#pragma once
#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include <chrono>
#include <sstream>
#include <curl/curl.h>
#include <sqlite3.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using namespace std;
using namespace rapidjson;
using namespace std::chrono;

class Card
{
  public:
    Card(const char * rawjson);
  
    virtual string name();
    virtual string json();
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
    virtual Card * cards_named_cache(string query);
    virtual vector<string> cards_autocomplete(string query);
    virtual Card * cards_random();

  private:
    CURL *easyhandle;
    sqlite3 *db;
    vector<Card *> cards;
    virtual char * api_call(const char * url);
    virtual string db_exec(const char * cmd);
    virtual int datecheck(string datetime);
    virtual const year_month_day parse(string datetime);
};
