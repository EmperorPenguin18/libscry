#ifndef __SCRY_H__
#define __SCRY_H__
#include <curl/curl.h>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <rapidjson/document.h>
#include <cstring>

using namespace std;
using namespace rapidjson;

class Card
{
  public:
    Card(const char * rawjson);
  
    virtual string name();
  private:
    Document data;
};

class Scry
{
  public:
    Scry();
    ~Scry();

    /* use virtual otherwise linker will try to perform static linkage */
    virtual vector<Card *> cards_search(string query);
    virtual Card * cards_named(string query);
    virtual vector<string> cards_autocomplete(string query);
    virtual Card * cards_random();

  private:
    CURL *easyhandle;
    sqlite3 *db;
    int rc;
    vector<Card *> cards;
    virtual char * api_call(const char * url);
};

#endif
