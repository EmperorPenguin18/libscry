#include "libscry.h"
#include <iostream>
#include <curl/curl.h>
#include <sqlite3.h>
#include <string>
#include <json/json.h>
#include <sstream>

using namespace std;

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

static string *DownloadedResponse;

static size_t writer(char *data, size_t size, size_t nmemb, string *buffer_in)
{
    cout << "In writer callback" << endl;

    // Is there anything in the buffer?
    if (buffer_in != NULL)
    {
        cout << "Buffer not null" << endl;
        // Append the data to the buffer
        buffer_in->append(data, size * nmemb);
        cout <<" Buffer appended, seting response" << endl;

        DownloadedResponse = buffer_in;
        cout << "Set downloadedResponse" << endl;
        return size * nmemb;
    }

    return 0;
}

Card Scry::cards_named(string search)
{
  curl_easy_setopt(easyhandle, CURLOPT_URL, "https://api.scryfall.com/cards/named?fuzzy=" + search);
  curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, writer);
  string *data;
  curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, data);
  CURLcode success = curl_easy_perform(easyhandle);
  if (success != 0) {
    printf("Error");
    exit(1);
  }
  stringstream temp(&data);
  Json::Value root;
  temp >> root;
  Card card(root["name"]);
  return card
}

void Scry::cleanup()
{
  curl_easy_cleanup(easyhandle);
  curl_global_cleanup();
  sqlite3_close(db);
}

Card::Card(string name) { name(name) }

string Card::getName()
{
  return name;
}
