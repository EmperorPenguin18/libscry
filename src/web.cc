//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "web.h"

using namespace std;

WebAccess::WebAccess() {
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

  curl_global_init(CURL_GLOBAL_ALL);
  easyhandle = curl_easy_init();
  curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, cb);
  curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(easyhandle, CURLOPT_HTTPGET, 1);
}

WebAccess::~WebAccess() {
  curl_easy_cleanup(easyhandle);
  curl_global_cleanup();
  dlclose(curl_lib);
}

size_t WebAccess::cb(void *data, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)userp;

  char *ptr = (char *)realloc(mem->response, mem->size + realsize + 1);
  if (ptr == NULL) return 0; // out of memory!

  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;

  return realsize;
}

char * WebAccess::api_call(string url) {
  curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
  struct memory chunk = {0};
  curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, (void *)&chunk);
  CURLcode success = curl_easy_perform(easyhandle);
  if (success != 0) {
    fprintf(stderr, "Errored with CURLcode %i\n", success);
    exit(success);
  }
  return chunk.response;
}

