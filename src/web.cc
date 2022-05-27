//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "web.h"

using namespace std;

WebAccess::WebAccess() {
  delay = 50;
  conn_per_thread = 10;
  construct();
}

WebAccess::WebAccess(vector<const char*> approved_urls) : approved_urls(approved_urls) {
  delay = 50;
  conn_per_thread = 10;
  construct();
}

WebAccess::WebAccess(vector<const char*> approved_urls, double delay) : approved_urls(approved_urls), delay(delay) {
  conn_per_thread = 10;
  construct();
}

WebAccess::WebAccess(vector<const char*> approved_urls, double delay, size_t conn_per_thread) : approved_urls(approved_urls), delay(delay), conn_per_thread(conn_per_thread) {
  construct();
}

WebAccess::WebAccess(double delay) : delay(delay) {
  conn_per_thread = 10;
  construct();
}

WebAccess::WebAccess(double delay, size_t conn_per_thread) : delay(delay), conn_per_thread(conn_per_thread) {
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
  memory* mem = (memory*)userp;
  byte* ptr = (byte*)realloc(mem->response, *(mem->size) + realsize + 1);
  if (!ptr) {
    /* out of memory */
    fprintf(stderr, "not enough memory (realloc returned NULL)\n");
    return 0;
  }
  mem->response = ptr;
  memcpy(&(mem->response[*(mem->size)]), data, realsize);
  *(mem->size) += realsize;
  return realsize;
}

CURL* WebAccess::add_transfer(const char* url, memory* chunk, int num) {
#ifdef DEBUG
  fprintf(stderr, "Adding url: %s\n", url);
#endif
  *(chunk->size) = 0;
  CURL *eh = curl_easy_init();
  curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(eh, CURLOPT_HTTPGET, 1);
  curl_easy_setopt(eh, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, cb);
  curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void*)chunk);
  curl_easy_setopt(eh, CURLOPT_URL, url);
#ifdef DEBUG
  curl_easy_setopt(eh, CURLOPT_PRIVATE, num);
#endif
  return eh;
}

char* WebAccess::strremove(char *str, const char *sub) {
  char *p, *q, *r;
  if (*sub && (q = r = strstr(str, sub)) != NULL) {
    size_t len = strlen(sub);
    while ((r = strstr(p = r + len, sub)) != NULL) {
      memmove(q, p, r - p);
      q += r - p;
    }
    memmove(q, p, strlen(p) + 1);
  }
  return str;
}

void WebAccess::checkurl(const char* url) {
#ifdef DEBUG
  mtx.lock();
  fprintf(stderr, "Checking URL: %s\n", url);
  mtx.unlock();
#endif
  char temp[strlen(url)+1] = "";
  strcat(temp, url);
  strremove(temp, "https://");
  char temp2[strlen(temp)+1] = "";
  strncat(temp2, temp, strchr(temp, '/')-temp);
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
}

byte* WebAccess::api_call(const char* url, size_t* size) {
  checkurl(url);

  memory chunk;
  chunk.response = (byte*)malloc(sizeof(byte));
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
  size_t size;
  char* output = (char*)api_call(url, &size);
  output[size] = '\0';
  return output;
}

CURL* WebAccess::add_transfer_multi(const char* url, struct memory* chunk, size_t num) {
  checkurl(url);
  chunk->response = (byte*)malloc(sizeof(byte));
  chunk->size = (size_t*)malloc(sizeof(size_t));
  return add_transfer(url, chunk, num);
}

WebAccess::memory* WebAccess::start_multi(char** urls, size_t conns) {
#ifdef DEBUG
  mtx.lock();
  fprintf(stderr, "All urls: \n");
  for (int i = 0; i < conns; i++) fprintf(stderr, "%s\n", urls[i]);
  mtx.unlock();
#endif
  CURLM *cm;
  CURLMsg *msg;
  size_t transfers = 0;
  int msgs_left = -1;
  int still_alive = 1;
  memory* chunks = (memory*)malloc(sizeof(memory)*conns);

  cm = curl_multi_init();
  curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, conn_per_thread);

  size_t start = (conns > conn_per_thread) ? conn_per_thread : conns;
  for (transfers = 0; transfers < start; transfers++) {
    curl_multi_add_handle(cm, add_transfer_multi(urls[transfers], &chunks[transfers], transfers));
  }
#ifdef DEBUG
  mtx.lock();
  fprintf(stderr, "Handles added\n");
  mtx.unlock();
#endif

  do {
    double time_span = (((double)(clock()-prev_time))/CLOCKS_PER_SEC)*1000;
    if (time_span > delay) {
      curl_multi_perform(cm, &still_alive);
      mtx.lock();
      prev_time = clock();
      mtx.unlock();
    }

    while ((msg = curl_multi_info_read(cm, &msgs_left))) {
#ifdef DEBUG
      mtx.lock();
      fprintf(stderr, "Msg ready\n");
      mtx.unlock();
#endif
      if (msg->msg == CURLMSG_DONE) {
	CURL *e = msg->easy_handle;
#ifdef DEBUG
	int num;
	curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &num);
        chunks[num].response[*(chunks[num].size)] = (byte)'\0';
        mtx.lock();
	fprintf(stderr, "Transfer num: %d\nSize: %d\nFirst 250 chars of response: %.250s\n", num, *(chunks[num].size), (char*)chunks[num].response);
        mtx.unlock();
#endif
	curl_multi_remove_handle(cm, e);
	curl_easy_cleanup(e);
      }
      else {
        fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
      }
      if (transfers < conns) {
        curl_multi_add_handle(cm, add_transfer_multi(urls[transfers], &chunks[transfers], transfers));
	transfers++;
      }
    }
    if (still_alive)
      curl_multi_wait(cm, NULL, 0, 1000, NULL);

  } while (still_alive || (transfers < conns));

  curl_multi_cleanup(cm);
  return chunks;
}

vector<char*> WebAccess::api_call(char** urls, size_t size) {
  unsigned available_threads = thread::hardware_concurrency()-1;
  unsigned used_threads = static_cast<unsigned>(ceil(
    static_cast<float>(size) / static_cast<float>(conn_per_thread)
  ));
#ifdef DEBUG
  fprintf(stderr, "Available threads: %d\nUsed threads: %d\n", available_threads, used_threads);
#endif
  vector<char*> output;
  if ( (available_threads > 0) && (available_threads >= used_threads) ) {
    vector<future<memory*>> threads(used_threads);
    size_t sizes[used_threads];
    for (int i = 0; i < used_threads; i++) {
      sizes[i] = 0;
      for (int j = 0; j < conn_per_thread; j++) {
	if (i*conn_per_thread+j < size) {
	  sizes[i]++;
	}
      }
#ifdef DEBUG
      mtx.lock();
      fprintf(stderr, "Thread %d is getting: \n", i);
      for (int j = 0; j < sizes[i]; j++) fprintf(stderr, "%s\n", *(urls+(i*conn_per_thread)+j));
      mtx.unlock();
#endif
      threads.at(i) = async(&WebAccess::start_multi, this, urls+(i*conn_per_thread), sizes[i]);
    }
    for (int i = 0; i < used_threads; i++) {
      memory* newdata = threads[i].get();
      for (int j = 0; j < sizes[i]; j++) {
        newdata[j].response[*(newdata[j].size)] = (byte)'\0';
#ifdef DEBUG
	mtx.lock();
        fprintf(stderr, "Adding to output: %.10s\n", (char*)newdata[j].response);
	mtx.unlock();
#endif
	output.push_back((char*)newdata[j].response);
      }
    }
    return output;
  }
  memory* newdata = start_multi(urls, size);
  for (int j = 0; j < size; j++) {
    newdata[j].response[*(newdata[j].size)] = (byte)'\0';
#ifdef DEBUG
    mtx.lock();
    fprintf(stderr, "Adding to output: %.10s\n", (char*)newdata[j].response);
    mtx.unlock();
#endif
    output.push_back((char*)newdata[j].response);
  }
  return output;
}
