#include "libscry.h"
#include <iostream>
#include <curl/curl.h>
#include <sqlite3.h>
#include <string>
#include <rapidjson/document.h>
#include <vector>
#include <cstring>

using namespace std;
using namespace rapidjson;

extern "C" Scry* create_object() {
  return new Scry;
}

extern "C" void destroy_object( Scry* object ) {
  delete object;
}

struct memory {
  char *response;
  size_t size;
};
 
static size_t cb(void *data, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)userp;

  char *ptr = (char *)realloc(mem->response, mem->size + realsize + 1);
  if(ptr == NULL)
    return 0;  /* out of memory! */

  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;

  return realsize;
}

Scry::Scry() {
  curl_global_init(CURL_GLOBAL_ALL);
  easyhandle = curl_easy_init();
  curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, cb);
  curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(easyhandle, CURLOPT_HTTPGET, 1);
  char * cachedir = getenv("XDG_CACHE_HOME");
  if (cachedir != NULL) rc = sqlite3_open(strcat(cachedir, "/libscry.db"), &db);
  else rc = sqlite3_open(strcat(getenv("HOME"), "/.cache/libscry.db"), &db);
}

Scry::~Scry() {
  curl_easy_cleanup(easyhandle);
  curl_global_cleanup();
  sqlite3_close(db);
  while (!cards.empty()) {
    delete cards.back();
    cards.pop_back();
  }
}

char * Scry::api_call(const char * url) {
  curl_easy_setopt(easyhandle, CURLOPT_URL, url);
  struct memory chunk = {0};
  curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, (void *)&chunk);
  CURLcode success = curl_easy_perform(easyhandle);
  if (success != 0) {
    printf("Errored with CURLcode %i\n", success);
    exit(success);
  }
  return chunk.response;
}

Card * Scry::cards_named(string query) {
  string url = "https://api.scryfall.com/cards/named?fuzzy=" + query;
  Card * card = new Card(api_call(url.c_str()));
  cards.push_back(card);
  return card;
}

Card::Card(const char * rawjson) {
  data.Parse(rawjson);
}

string Card::name() {
  return data["name"].GetString();
}
