#include "libscry.h"
#include <iostream>
#include <curl/curl.h>
#include <sqlite3.h>

using namespace std;

extern "C" MyClass* create_object()
{
  return new MyClass;
}

extern "C" void destroy_object( MyClass* object )
{
  curl_global_cleanup();
  delete object;
}

MyClass::MyClass()
{
  curl_global_init(CURL_GLOBAL_ALL);
  easyhandle = curl_easy_init();
}

void MyClass::DoSomething()
{
  curl_easy_setopt(easyhandle, CURLOPT_URL, "https://api.scryfall.com/cards/search?q=c%3Ared+pow%3D3");
  success = curl_easy_perform(easyhandle);
}
