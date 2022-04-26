//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "web.h"

using namespace std;
using namespace std::chrono;

WebAccess::WebAccess() {
  delay = duration<long, ratio<1,1000>>(50);
  conn_per_thread = 10;
  construct();
}

WebAccess::WebAccess(vector<const char*> approved_urls) : approved_urls(approved_urls) {
  delay = duration<long, ratio<1,1000>>(50);
  conn_per_thread = 10;
  construct();
}

WebAccess::WebAccess(vector<const char*> approved_urls, long delay) : approved_urls(approved_urls), delay(delay) {
  conn_per_thread = 10;
  construct();
}

WebAccess::WebAccess(vector<const char*> approved_urls, long delay, size_t conn_per_thread) : approved_urls(approved_urls), delay(delay), conn_per_thread(conn_per_thread) {
  construct();
}

WebAccess::WebAccess(long delay) : delay(delay) {
  conn_per_thread = 10;
  construct();
}

WebAccess::WebAccess(long delay, size_t conn_per_thread) : delay(delay), conn_per_thread(conn_per_thread) {
  construct();
}

void WebAccess::construct() {
  curl_lib = dlopen("libcurl.so", RTLD_LAZY);
  if (!curl_lib) {
    fprintf(stderr, "%s\n", dlerror());
    exit(EXIT_FAILURE);
  }
  curl_global_init = reinterpret_cast<cgi_handle>(dlsym(curl_lib, "curl_global_init"));
  curl_easy_init = reinterpret_cast<cei_handle>(dlsym(curl_lib, "curl_easy_init"));
  curl_easy_setopt = reinterpret_cast<ces_handle>(dlsym(curl_lib, "curl_easy_setopt"));
  curl_easy_perform = reinterpret_cast<cep_handle>(dlsym(curl_lib, "curl_easy_perform"));
  curl_easy_cleanup = reinterpret_cast<cec_handle>(dlsym(curl_lib, "curl_easy_cleanup"));
  curl_global_cleanup = reinterpret_cast<cgc_handle>(dlsym(curl_lib, "curl_global_cleanup"));
  curl_easy_strerror = reinterpret_cast<cee_handle>(dlsym(curl_lib, "curl_easy_strerror"));
  curl_multi_add_handle = reinterpret_cast<cma_handle>(dlsym(curl_lib, "curl_multi_add_handle"));
  curl_multi_init = reinterpret_cast<cmi_handle>(dlsym(curl_lib, "curl_multi_init"));
  curl_multi_setopt = reinterpret_cast<cms_handle>(dlsym(curl_lib, "curl_multi_setopt"));
  curl_multi_perform = reinterpret_cast<cmp_handle>(dlsym(curl_lib, "curl_multi_perform"));
  curl_multi_info_read = reinterpret_cast<cmn_handle>(dlsym(curl_lib, "curl_multi_info_read"));
  curl_easy_getinfo = reinterpret_cast<ceg_handle>(dlsym(curl_lib, "curl_easy_getinfo"));
  curl_multi_remove_handle = reinterpret_cast<cmr_handle>(dlsym(curl_lib, "curl_multi_remove_handle"));
  curl_multi_wait = reinterpret_cast<cmw_handle>(dlsym(curl_lib, "curl_multi_wait"));
  curl_multi_cleanup = reinterpret_cast<cmc_handle>(dlsym(curl_lib, "curl_multi_cleanup"));

  curl_global_init(CURL_GLOBAL_ALL);
}

WebAccess::~WebAccess() {
  curl_global_cleanup();
  dlclose(curl_lib);
}

size_t WebAccess::cb(void *data, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)userp;
  byte *ptr = (byte *)realloc(mem->response, *(mem->size) + realsize + 1);
  if (!ptr) {
    /* out of memory */
    fprintf(stderr, "not enough memory (realloc returned NULL)\n");
    return 0;
  }
  mem->response = ptr;
  memcpy(&(mem->response[*(mem->size)]), data, realsize);
  *(mem->size) += realsize;
  //mem->response[mem->size] = (byte)0;
  return realsize;
}

CURL * WebAccess::add_transfer(const char* url, struct memory* chunk, int num) {
#ifdef DEBUG
  fprintf(stderr, "Adding url: %s\n", url);
#endif
  *(chunk->size) = 0;
  CURL *eh = curl_easy_init();
  curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(eh, CURLOPT_HTTPGET, 1);
  curl_easy_setopt(eh, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, cb);
  curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void *)chunk);
  curl_easy_setopt(eh, CURLOPT_URL, url);
  curl_easy_setopt(eh, CURLOPT_PRIVATE, num);
  return eh;
}

void WebAccess::replace(char* str, const char* s) {
#ifdef DEBUG
  fprintf(stderr, "Replacing: %s\n", str);
#endif
  size_t length = strlen(str);
  size_t length_s = strlen(s);
  for (size_t i = 0; i < length-length_s; i++) {
    if (strncmp(str+i, s, length_s) == 0) {
      for (size_t j = i; j < length-length_s; j++) {
	str[j] = str[j+length_s];
      }
      length -= length_s;
    }
  }
#ifdef DEBUG
  fprintf(stderr, "Replaced with: %s\n", str);
#endif
}

void WebAccess::checkurl(const char* url) {
#ifdef DEBUG
  mtx.lock();
  fprintf(stderr, "Checking URL: %s\n", url);
  mtx.unlock();
#endif
  char* temp = (char*)malloc(sizeof(char)*strlen(url)+1);
  strcpy(temp, url);
  replace(temp, "https://");
  char* temp2 = (char*)calloc(strlen(temp), sizeof(char)*strlen(temp)+1);
  strncpy(temp2, temp, strchr(temp, '/')-temp);
#ifdef DEBUG
  mtx.lock();
  fprintf(stderr, "Relevant section: %s\n", temp2);
  mtx.unlock();
#endif
  size_t i;
  for (i = 0; i < approved_urls.size(); i++) if (strcmp(approved_urls[i], temp2) == 0) break;
  if (i == approved_urls.size()) {
    fprintf(stderr, "Attempted to use unapproved url: %s\n", temp2);
    exit(EXIT_FAILURE);
  }
  free(temp);
  free(temp2);
}

byte* WebAccess::api_call(const char* url, size_t* size) {
  checkurl(url);

  struct memory chunk = {0};
  chunk.size = size;
  CURL *eh = add_transfer(url, &chunk, 0);

  CURLcode res = curl_easy_perform(eh);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    exit(res);
  }
#ifdef DEBUG
  fprintf(stderr, "First 250 chars of response: ");
  if (*(chunk.size) < 250) for (size_t i = 0; i < *(chunk.size); i++) fprintf(stderr, "%c", (char)chunk.response[i]);
  else for (size_t i = 0; i < 250; i++) fprintf(stderr, "%c", (char)chunk.response[i]);
  fprintf(stderr, "\n");
#endif

  curl_easy_cleanup(eh);
  return chunk.response;
}

char* WebAccess::api_call(const char* url) {
  size_t size = 0;
  char* output = (char*)api_call(url, &size);
  output[size] = '\0';
  return output;
}

vector<string> WebAccess::start_multi(vector<string> urls) {
#ifdef DEBUG
  mtx.lock();
  fprintf(stderr, "All urls: \n");
  for (int i = 0; i < urls.size(); i++) fprintf(stderr, "%s\n", urls[i].c_str());
  mtx.unlock();
#endif
  CURLM *cm;
  CURLMsg *msg;
  unsigned int transfers = 0;
  int msgs_left = -1;
  int still_alive = 1;
  unsigned int conns = min(urls.size(), conn_per_thread);
  vector<string> output(conns);
  struct memory chunks[conns];
  size_t sizes[conns];

  cm = curl_multi_init();
  curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, conn_per_thread);

  for (transfers = 0; transfers < conns; transfers++) {
    checkurl(urls[transfers].c_str());
    chunks[transfers] = {0};
    sizes[transfers] = 0;
    chunks[transfers].size = &sizes[transfers];
    curl_multi_add_handle(cm, add_transfer(urls[transfers].c_str(), &chunks[transfers], transfers));
  }
#ifdef DEBUG
  mtx.lock();
  fprintf(stderr, "Handles added\n");
  mtx.unlock();
#endif

  do {
    duration<long, ratio<1,1000>> time_span = duration_cast<duration<long, ratio<1,1000>>>(steady_clock::now() - prev_time);
    if (time_span > delay) {
      curl_multi_perform(cm, &still_alive);
      mtx.lock();
      prev_time = steady_clock::now();
      mtx.unlock();
    }

    while ((msg = curl_multi_info_read(cm, &msgs_left))) {
#ifdef DEBUG
      mtx.lock();
      fprintf(stderr, "Msg ready\n");
      mtx.unlock();
#endif
      if (msg->msg == CURLMSG_DONE) {
	int num;
	CURL *e = msg->easy_handle;
	curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &num);
#ifdef DEBUG
        mtx.lock();
	fprintf(stderr, "Transfer num: %d\n", num);
        mtx.unlock();
#endif
	chunks[num].response[*(chunks[num].size)] = (byte)'\0';
	output[num].reserve(strlen((char*)chunks[num].response));
	output[num].assign((char*)chunks[num].response);
#ifdef DEBUG
        mtx.lock();
	fprintf(stderr, "First 250 chars of response: %.250s\n", output[num].c_str());
        mtx.unlock();
#endif
	curl_multi_remove_handle(cm, e);
	curl_easy_cleanup(e);
      }
      else {
        fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
      }
    }
    if (still_alive)
      curl_multi_wait(cm, NULL, 0, 1000, NULL);

  } while (still_alive || (transfers < urls.size()));

  curl_multi_cleanup(cm);
  return output;
}

vector<string> WebAccess::api_call(vector<string> urls) {
  unsigned available_threads = thread::hardware_concurrency()-1; //Not actual max, but keeps things reasonable
  unsigned used_threads = static_cast<unsigned>(ceil(
    static_cast<float>(urls.size()) / static_cast<float>(conn_per_thread)
  ));
#ifdef DEBUG
  fprintf(stderr, "Available threads: %d\nUsed threads: %d\n", available_threads, used_threads);
#endif
  if ( (available_threads > 0) && (available_threads >= used_threads) ) {
    vector<string> output;
    vector<future<vector<string>>> threads(used_threads);
    for (int i = 0; i < used_threads; i++) {
      vector<string> data( min(conn_per_thread, urls.size()-i*conn_per_thread) );
      for (int j = 0; j < data.size(); j++) data.at(j) = urls.at(i*conn_per_thread+j);
#ifdef DEBUG
      mtx.lock();
      fprintf(stderr, "Thread %d is getting: \n", i);
      for (int j = 0; j < data.size(); j++) fprintf(stderr, "%s\n", data.at(j).c_str());
      mtx.unlock();
#endif
      threads.at(i) = async(&WebAccess::start_multi, this, data);
    }
    for (int i = 0; i < used_threads; i++) {
      vector<string> newdata = threads[i].get();
      for (int j = 0; j < newdata.size(); j++) {
#ifdef DEBUG
	mtx.lock();
	fprintf(stderr, "Adding to output: %.10s\n", newdata[j].c_str());
	mtx.unlock();
#endif
	output.push_back(newdata[j]);
      }
    }
    return output;
  }
  start_multi(urls);
  return urls;
}
