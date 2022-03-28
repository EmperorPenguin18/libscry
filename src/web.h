//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#pragma once
#include <string>
#include <cstring>
#include <vector>
#include <chrono>
#include <ratio>
#include <algorithm>
#include <regex>
#include <dlfcn.h>
#include <curl/curl.h>

#ifdef DEBUG
#include <iostream>
#endif

using namespace std;
using namespace std::chrono;

class WebAccess {
  public:
    WebAccess();
    WebAccess(vector<string>);
    WebAccess(vector<string>, long);
    WebAccess(vector<string>, long, long);
    WebAccess(long);
    WebAccess(long, long);
    ~WebAccess();
    
    virtual char * api_call(string url);
    virtual vector<string> api_call(vector<string>);
  private:
    void construct();
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
    typedef const char* (*cee_handle)(CURLcode);
    cee_handle curl_easy_strerror;
    typedef CURLMcode (*cma_handle)(CURLM *, CURL *);
    cma_handle curl_multi_add_handle;
    typedef CURLM* (*cmi_handle)();
    cmi_handle curl_multi_init;
    typedef CURLMcode (*cms_handle)(CURLM *, CURLMoption, ...);
    cms_handle curl_multi_setopt;
    typedef CURLMcode (*cmp_handle)(CURLM *, int *);
    cmp_handle curl_multi_perform;
    typedef CURLMsg* (*cmn_handle)(CURLM *, int *);
    cmn_handle curl_multi_info_read;
    typedef CURLcode (*ceg_handle)(CURL *, CURLINFO, ...);
    ceg_handle curl_easy_getinfo;
    typedef CURLMcode (*cmr_handle)(CURLM *, CURL *);
    cmr_handle curl_multi_remove_handle;
    typedef CURLMcode (*cmw_handle)(CURLM *, struct curl_waitfd[], unsigned int, int, int *);
    cmw_handle curl_multi_wait;
    typedef CURLMcode (*cmc_handle)(CURLM *);
    cmc_handle curl_multi_cleanup;

    static size_t cb(void *data, size_t size, size_t nmemb, void *userp);

    vector<string> approved_urls;
    void checkurl(string);

    duration<long, ratio<1,1000>> delay;
    steady_clock::time_point prev_time;
    long conn_per_thread;
    struct memory {
      char *response;
      size_t size;
    };
    CURL * add_transfer(string, struct memory *, int);
};
