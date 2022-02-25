//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#pragma once
#include <iostream>
#include "web.h"
#include "data.h"
#include "list.h"
#include "card.h"

using namespace std;

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
    ///Returns all cards from a search using the list representing the first page
    virtual vector<Card *> allcards(List * list, bool cache);
  private:
    WebAccess * wa;
    DataAccess * da;
    vector<Card *> cards;
    vector<List *> lists;
    virtual vector<string> explode(const string& str, const char& ch);
    virtual string implode(const vector<string>& strs, const char& ch);
    virtual string urlformat(string str);
    virtual string nameformat(string str);
    virtual string cachecard(List * list, bool recursive);
};
