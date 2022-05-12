//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#pragma once
#include <string>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using namespace std;
using namespace rapidjson;

///This class is used to represent a single card
class Card {
  public:
    Card(const char*);
  
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
    ///Get the starting loyalty of the card
    virtual string loyalty();
    ///Get the set name of the card
    virtual string set();
    ///Get the USD non-foil price of the most recent printing of the card
    virtual string price();
    ///Get the legalities (for most formats) of the card
    virtual vector<string> legality();
  private:
    Document data;
};
