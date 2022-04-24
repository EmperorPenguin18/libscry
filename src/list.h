//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#pragma once
#include <vector>
#include <regex>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "card.h"

#ifdef DEBUG
#include <iostream>
#endif

///This class is used to represent a list of cards (returned from a search for example)
class List {
  public:
    List(const char*);
    List(vector<string>);
    ~List();
  
    ///Returns a vector with all the cards on this page of the list. For cards on all pages see allcards().
    virtual vector<Card*> cards();
    ///Returns the string of the url for the next page of a search result
    virtual string nextPage();
    ///Gets the raw json text of the card provided by Scryfall.
    virtual string json();
  private:
    Document data;
    vector<Card*> content;
    string nextpage;
    void construct(const char*);
};

