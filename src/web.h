//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#pragma once
#include <string>
#include <cstring>
#include <vector>
#include <dlfcn.h>
#include <curl/curl.h>

using namespace std;

class WebAccess {
  public:
    WebAccess();
    ~WebAccess();
    
    virtual char * api_call(string url);
  private:
    void * curl_lib;
    typedef CURL* (*cgi_handle)(int);
    cgi_handle curl_global_init;
    typedef CURL* (*cei_handle)();
    cei_handle curl_easy_init;
    typedef CURLcode (*ces_handle)(CURL *, CURLoption, ...);
    ces_handle curl_easy_setopt;
    typedef CURLcode (*cep_handle)(CURL *);
    cep_handle curl_easy_perform;
    typedef void (*cec_handle)(CURL *);
    cec_handle curl_easy_cleanup;
    typedef void (*cgc_handle)();
    cgc_handle curl_global_cleanup;

    unsigned int conn_per_thread;
    vector<string> approved_urls;
    CURL *easyhandle;
    struct memory {
      char *response;
      size_t size;
    };
    static size_t cb(void *data, size_t size, size_t nmemb, void *userp);
};
