//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "web.h"

using namespace std;

WebAccess::WebAccess() {
  conn_per_thread = 10;
  construct();
}

WebAccess::WebAccess(vector<string> approved_urls) : approved_urls(approved_urls) {
  conn_per_thread = 10;
  construct();
}

WebAccess::WebAccess(long conn_per_thread) : conn_per_thread(conn_per_thread) {
  construct();
}

WebAccess::WebAccess(vector<string> approved_urls, long conn_per_thread) : approved_urls(approved_urls), conn_per_thread(conn_per_thread) {
  construct();
}

void WebAccess::construct() {
  curl_lib = dlopen("libcurl.so", RTLD_LAZY | RTLD_DEEPBIND);
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

  char *ptr = (char *)realloc(mem->response, mem->size + realsize + 1);
  if (!ptr) {
    /* out of memory */
    fprintf(stderr, "not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;

  return realsize;
}

CURL * WebAccess::add_transfer(string url, struct memory* chunk, int num) {
#ifdef DEBUG
  cout << "Adding url: " << url << endl;
#endif
  CURL *eh = curl_easy_init();
  curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(eh, CURLOPT_HTTPGET, 1);
  curl_easy_setopt(eh, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, cb);
  curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void *)chunk);
  curl_easy_setopt(eh, CURLOPT_URL, url.c_str());
  curl_easy_setopt(eh, CURLOPT_PRIVATE, num);
  return eh;
}

char * WebAccess::api_call(string url) {
  struct memory chunk = {0};
  CURL *eh = add_transfer(url, &chunk, 0);

  CURLcode res = curl_easy_perform(eh);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    exit(res);
  }
#ifdef DEBUG
  cout << "cURL response: " << chunk.response << endl;
#endif

  curl_easy_cleanup(eh);
  return chunk.response;
}

vector<string> WebAccess::api_call(vector<string> urls) {
#ifdef DEBUG
  cout << "All urls: " << endl;
  for (int i = 0; i < urls.size(); i++) cout << urls[i] << endl;
#endif
  CURLM *cm;
  CURLMsg *msg;
  unsigned int transfers = 0;
  int msgs_left = -1;
  int still_alive = 1;
  vector<string> output(urls.size());
  struct memory chunks[urls.size()];

  cm = curl_multi_init();
  curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, conn_per_thread);

  for (transfers = 0; transfers < conn_per_thread; transfers++) {
    chunks[transfers] = {0};
    curl_multi_add_handle(cm, add_transfer(urls[transfers], &chunks[transfers], transfers));
  }
#ifdef DEBUG
  cout << "Handles added" << endl;
#endif

  do {
    curl_multi_perform(cm, &still_alive);
#ifdef DEBUG
    cout << "Still alive: " << to_string(still_alive) << endl;
#endif

    while ((msg = curl_multi_info_read(cm, &msgs_left))) {
#ifdef DEBUG
      cout << "Msg ready" << endl;
#endif
      if (msg->msg == CURLMSG_DONE) {
	int num;
	CURL *e = msg->easy_handle;
	curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &num);
#ifdef DEBUG
	cout << "Transfer num: " << to_string(num) << endl;
#endif
	output[num].reserve(strlen(chunks[num].response));
	output[num].assign(chunks[num].response);
#ifdef DEBUG
	cout << "First 250 chars of response: " << output[num].substr(0, 250) << endl;
#endif
	curl_multi_remove_handle(cm, e);
	curl_easy_cleanup(e);
      }
      else {
        fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
      }
      if (transfers < urls.size()) {
	chunks[transfers] = {0};
        curl_multi_add_handle(cm, add_transfer(urls[transfers++], &chunks[transfers], transfers));
      }
    }
#ifdef DEBUG
    cout << "Transfers: " << to_string(transfers) << endl;
#endif
    if (still_alive)
      curl_multi_wait(cm, NULL, 0, 1000, NULL);

  } while (still_alive || (transfers < urls.size()));

  curl_multi_cleanup(cm);
  return output;
}
