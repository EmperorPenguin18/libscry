//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#pragma once
#include <cstring>
#include <vector>
#include <future>
#include <mutex>
#include <cmath>
#include <dlfcn.h>
#include <curl/curl.h>

using namespace std;

class WebAccess {
  public:
    WebAccess();
    WebAccess(vector<const char*>);
    WebAccess(vector<const char*>, double);
    WebAccess(vector<const char*>, double, size_t);
    WebAccess(double);
    WebAccess(double, size_t);
    ~WebAccess();
    
    virtual byte* api_call(const char*, size_t*);
    virtual char* api_call(const char*);
    virtual vector<char*> api_call(char**, size_t);
  private:
    virtual void construct();
    void* curl_lib;
    typedef CURL* (*cgi_handle)(int);
    cgi_handle curl_global_init;
    typedef CURL* (*cei_handle)();
    cei_handle curl_easy_init;
    typedef CURLcode (*ces_handle)(CURL*, CURLoption, ...);
    ces_handle curl_easy_setopt;
    typedef CURLcode (*cep_handle)(CURL*);
    cep_handle curl_easy_perform;
    typedef void (*cec_handle)(CURL*);
    cec_handle curl_easy_cleanup;
    typedef void (*cgc_handle)();
    cgc_handle curl_global_cleanup;
    typedef const char* (*cee_handle)(CURLcode);
    cee_handle curl_easy_strerror;
    typedef CURLMcode (*cma_handle)(CURLM*, CURL*);
    cma_handle curl_multi_add_handle;
    typedef CURLM* (*cmi_handle)();
    cmi_handle curl_multi_init;
    typedef CURLMcode (*cms_handle)(CURLM*, CURLMoption, ...);
    cms_handle curl_multi_setopt;
    typedef CURLMcode (*cmp_handle)(CURLM*, int*);
    cmp_handle curl_multi_perform;
    typedef CURLMsg* (*cmn_handle)(CURLM*, int*);
    cmn_handle curl_multi_info_read;
    typedef CURLcode (*ceg_handle)(CURL*, CURLINFO, ...);
    ceg_handle curl_easy_getinfo;
    typedef CURLMcode (*cmr_handle)(CURLM*, CURL*);
    cmr_handle curl_multi_remove_handle;
    typedef CURLMcode (*cmw_handle)(CURLM*, struct curl_waitfd[], unsigned int, int, int*);
    cmw_handle curl_multi_wait;
    typedef CURLMcode (*cmc_handle)(CURLM*);
    cmc_handle curl_multi_cleanup;

    static size_t cb(void*, size_t, size_t, void*);
    struct memory {
      byte* response;
      size_t* size;
    };

    vector<const char*> approved_urls;
    virtual char* strremove(char*, const char*);
    virtual void checkurl(const char*);

    double delay;
    clock_t prev_time;
    size_t conn_per_thread;
    virtual CURL* add_transfer(const char*, struct memory*, int);
    mutex mtx;
    virtual CURL* add_transfer_multi(const char*, struct memory*, size_t);
    virtual struct memory* start_multi(char**, size_t);
};
