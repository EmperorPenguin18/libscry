//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#pragma once
#include <string>
#include <cstring>
#include <curl/curl.h>

using namespace std;

class WebAccess {
  public:
    WebAccess();
    ~WebAccess();
    
    virtual char * api_call(string url);
  private:
    CURL *easyhandle;
};
