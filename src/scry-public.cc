//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "scry.h"

using namespace std;
using namespace rapidjson;

extern "C" Scry* create_object() {
  return new Scry;
}

extern "C" void destroy_object( Scry* object ) {
  delete object;
}

Scry::Scry() {
  wa = new WebAccess();
  da = new DataAccess();
  da->db_init("Cards");
  da->db_init("Lists");
  da->db_init("Autocompletes");
}

Scry::~Scry() {
  delete(wa);
  delete(da);
  while (!cards.empty()) {
    delete cards.back();
    cards.pop_back();
  }
  while (!lists.empty()) {
    delete lists.back();
    lists.pop_back();
  }
}

List * Scry::cards_search(string query) {
  query = urlformat(query);
  string url = "https://api.scryfall.com/cards/search?q=" + query;
  List * list = new List(wa->api_call(url));
  lists.push_back(list);
  return list;
}

List * Scry::cards_search_cache(string query) {
  query = urlformat(query);
  List * list;

  regex pages(".*&p"); smatch sm; regex_search(query, sm, pages);
  string search = string(sm[0]).substr(0, sm[0].length()-2);
  if (size(search) < 1) search = query;
  if (da->db_check("Lists", search)) {
    if (da->datecheck("Lists", search) == 1) {
      string url = "https://api.scryfall.com/cards/search?q=" + query;
      list = new List(wa->api_call(url));
      lists.push_back(list);
      da->db_write("Lists", search, cachecard(list, true));
    } else {
      vector<string> strvec = explode(da->db_read("Lists", search), '\n');
      vector<Card *> content;
      for (int i = 0; i < strvec.size(); i++)
	content.push_back( new Card( da->db_read("Cards", nameformat(strvec[i])).c_str() ) );
      list = new List( content );
      lists.push_back(list);
    }
  } else {
    string url = "https://api.scryfall.com/cards/search?q=" + query;
    list = new List(wa->api_call(url));
    lists.push_back(list);
    string names = cachecard(list, false);
    if (da->db_check("Lists", search)) {
      string temp = nameformat( da->db_read("Lists", search) );
      da->db_write("Lists", search, names + "\n" + temp);
    } else da->db_new("Lists", search, names);
  }

  return list;
}

Card * Scry::cards_named(string query) {
  query = urlformat(query);
  string url = "https://api.scryfall.com/cards/named?fuzzy=" + query;
  Card * card = new Card(wa->api_call(url));
  cards.push_back(card);
  return card;
}

Card * Scry::cards_named_cache(string query) {
  query = urlformat(query);
  Card * card;
  query[0] = toupper(query[0]);
  string name = nameformat(query);

  if (da->db_check("Cards", name)) {
    if (da->datecheck("Cards", name) == 1) {
      card = cards_named(query);
      da->db_write("Cards", name, nameformat(card->json()));
    } else {
      card = new Card( da->db_read("Cards", name).c_str() );
      cards.push_back(card);
    }
  } else {
    card = cards_named(query);
    da->db_new("Cards", name, nameformat(card->json()));
  }

  return card;
}

vector<string> Scry::cards_autocomplete(string query) {
  query = urlformat(query);
  string url = "https://api.scryfall.com/cards/autocomplete?q=" + query;
  Document doc;
  doc.Parse(wa->api_call(url));
  const Value& a = doc["data"];
  vector<string> output;
  for (auto& v : a.GetArray()) output.push_back(v.GetString());
  return output;
}

vector<string> Scry::cards_autocomplete_cache(string query) {
  query = urlformat(query);
  vector<string> names;

  if (da->db_check("Autocompletes", query)) {
    if (da->datecheck("Autocompletes", query) == 1) {
      names = cards_autocomplete(query);
      string namestr = implode(names, '\n');
      da->db_write("Autocompletes", query, nameformat(namestr));
    } else {
      names = explode(da->db_read("Autocompletes", query), '\n');
    }
  } else {
    names = cards_autocomplete(query);
    string namestr = implode(names, '\n');
    da->db_new("Autocompletes", query, nameformat(namestr));
  }

  return names;
}

Card * Scry::cards_random() {
  string url = "https://api.scryfall.com/cards/random";
  Card * card = new Card(wa->api_call(url));
  cards.push_back(card);
  return card;
}

vector<Card *> Scry::split(Card * card) {
  Document doc; doc.Parse(card->json().c_str());
  vector<Card *> output;
  for (int i = 0; i < doc["card_faces"].Size(); i++) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc["card_faces"][i].Accept(writer);
    Card * card = new Card(buffer.GetString());
    output.push_back(card); cards.push_back(card);
  }
  return output;
}

vector<Card *> Scry::allcards(List * list, bool cache) {
  vector<Card *> output = list->cards();
  if (list->nextPage() != "") {
    vector<Card *> append;
    if (cache) append = allcards(cards_search_cache(list->nextPage()), true);
    else append = allcards(cards_search(list->nextPage()), false);
    output.insert(output.end(), append.begin(), append.end());
  }
  return output;
}
