#include "libscry.h"
#include <iostream>
#include <curl/curl.h>
#include <sqlite3.h>
#include <string>
#include <rapidjson/document.h>
#include <vector>

using namespace std;
using namespace rapidjson;

extern "C" Scry* create_object()
{
  return new Scry;
}

extern "C" void destroy_object( Scry* object )
{
  delete object;
}

Scry::Scry()
{
  curl_global_init(CURL_GLOBAL_ALL);
  easyhandle = curl_easy_init();
  rc = sqlite3_open("/home/sebastien/test.db", &db);
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

Card * Scry::cards_named(string search)
{
  string url = "https://api.scryfall.com/cards/named?fuzzy=";
  url.append(search);
  curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, WriteCallback);
  string data;
  curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &data);
  curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(easyhandle, CURLOPT_HTTPGET, 1);
  CURLcode success = curl_easy_perform(easyhandle);
  if (success != 0) {
    printf("Errored with CURLcode %i\n", success);
    exit(success);
  }
  Document document;
  document.Parse(data.c_str());
  Card * card = new Card(document["name"].GetString());
  cards.push_back(card);
  return card;
}

void Scry::cleanup()
{
  curl_easy_cleanup(easyhandle);
  curl_global_cleanup();
  sqlite3_close(db);
  while (!cards.empty()) {
    delete cards.back();
    cards.pop_back();
  }
}

Card::Card(const string& name) : name(name) {}

string& Card::getName()
{
  return name;
}
