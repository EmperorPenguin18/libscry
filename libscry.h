#pragma once
#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include <chrono>
#include <sstream>
#include <regex>
#include <curl/curl.h>
#include <sqlite3.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using namespace std;
using namespace rapidjson;
using namespace std::chrono;

class Card;
class List;
class Scry;

class Card {
  public:
    Card(const char * rawjson);
  
    virtual string name();
    virtual string json();
  private:
    Document data;
};

class List {
  public:
    List(Scry * scry, const char * rawjson, bool cache);
    List(vector<Card *> input);
    ~List();
  
    virtual vector<Card *> cards();
    virtual vector<Card *> allcards();
  private:
    vector<Card *> content;
    List * nextpage;
};

class Scry {
  public:
    Scry();
    ~Scry();

    /* use virtual otherwise linker will try to perform static linkage */
    virtual List * cards_search(string query);
    virtual List * cards_search_cache(string query);
    virtual Card * cards_named(string query);
    virtual Card * cards_named_cache(string query);
    virtual vector<string> cards_autocomplete(string query);
    virtual Card * cards_random();

  private:
    CURL *easyhandle;
    sqlite3 *db;
    vector<Card *> cards;
    vector<List *> lists;
    virtual char * api_call(string url);
    virtual string db_exec(string in);
    virtual bool db_check(string table, string search);
    virtual string db_read(string table, string search, string column);
    virtual void db_write(string table, string key, string value);
    virtual void db_new(string table, string key, string value);
    virtual int datecheck(string datetime);
    virtual const year_month_day parse(string datetime);
    virtual vector<string> explode(const string& str, const char& ch);
    virtual string urlformat(string str);
    virtual string nameformat(string str);
    virtual string cachecard(List * list, bool recursive);
};
