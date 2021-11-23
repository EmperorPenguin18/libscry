//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#pragma once
#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include <chrono>
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

///This class is used to represent a single card
class Card {
  public:
    Card(const char * rawjson);
  
    ///Get the name (Storm Crow) of the card.
    virtual string name();
    ///Get the mana cost ({1}{U}) of the card.
    virtual string mana_cost();
    ///Get the type line (Creature - Bird) of the card.
    virtual string type_line();
    ///Get the oracle text (Flying) of the card.
    virtual string oracle_text();
    ///Get the power (1) of the card.
    virtual string power();
    ///Get the toughness (2) of the card.
    virtual string toughness();
    ///Returns true if the card has two faces, false otherwise.
    virtual bool dual_sided();
    ///Get the raw json text of the card provided by Scryfall.
    virtual string json();
  private:
    Document data;
};

///This class is used to represent a list of cards (returned from a search for example)
class List {
  public:
    List(Scry * scry, const char * rawjson, bool cache);
    List(vector<Card *> input);
    ~List();
  
    ///Returns a vector with all the cards on this page of the list. For cards on all pages see allcards().
    virtual vector<Card *> cards();
    ///Cumulates cards from all pages of the list and returns them as a vector.
    virtual vector<Card *> allcards();
  private:
    vector<Card *> content;
    List * nextpage;
};

///This class is the main interface to Scryfall. It handles all the API requests and caching.
class Scry {
  public:
    Scry();
    ~Scry();

    ///Put in a query using Scryfall syntax (commander:g+type:legendary) as the argument.
    virtual List * cards_search(string query);
    ///Cached version of cards_search.
    virtual List * cards_search_cache(string query);
    ///Put in the name of a card as the argument.
    virtual Card * cards_named(string query);
    ///Cached version of cards_named.
    virtual Card * cards_named_cache(string query);
    ///Put in part of a card's name as the argument.
    virtual vector<string> cards_autocomplete(string query);
    ///Cached version of cards_autocomplete.
    virtual vector<string> cards_autocomplete_cache(string query);
    ///Returns a randomly selected card.
    virtual Card * cards_random();
    ///Splits a card into it's multiple faces. Useful because some cards won't have certain data in the conglomerate.
    virtual vector<Card *> split(Card * card);
  private:
    CURL *easyhandle;
    sqlite3 *db;
    vector<Card *> cards;
    vector<List *> lists;
    virtual char * api_call(string url);
    virtual string db_exec(string in);
    virtual void db_init(string table);
    virtual bool db_check(string table, string search);
    virtual string db_read(string table, string search, string column);
    virtual void db_write(string table, string key, string value);
    virtual void db_new(string table, string key, string value);
    virtual int datecheck(string datetime);
    virtual const year_month_day parse(string datetime);
    virtual vector<string> explode(const string& str, const char& ch);
    virtual string implode(const vector<string>& strs, const char& ch);
    virtual string urlformat(string str);
    virtual string nameformat(string str);
    virtual string cachecard(List * list, bool recursive);
};
