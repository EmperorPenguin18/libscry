//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "scry.h"

using namespace std;
using namespace rapidjson;
using namespace std::chrono;

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

const year_month_day Scry::parse(string datetime) {
  string ys = datetime.substr(0,4);
  string ms = datetime.substr(5,2);
  string ds = datetime.substr(8,2);
  year y(stoi(ys));
  month m (stoi(ms));
  day d (stoi(ds));
  const year_month_day output(y, m, d);
  return output;
}

int Scry::datecheck(string datetime) {
  const time_point<system_clock> now{system_clock::now()};
  const year_month_day ymd{floor<days>(now)};
  const year_month_day old = parse(datetime);
  
  int output;
  if (static_cast<int>(ymd.year()) > static_cast<int>(old.year())) {
    output = 1;
  } else {
    if (static_cast<unsigned>(ymd.month()) > static_cast<unsigned>(old.month())) {
      output = 1;
    } else {
      if (static_cast<unsigned>(ymd.day()) > static_cast<unsigned>(old.day()) + 7) {
	output = 1;
      } else if (static_cast<unsigned>(ymd.day()) == static_cast<unsigned>(old.day()) + 7) {
	output = 0;
      } else {
	output = -1;
      }
    }
  }

  return output;
}

vector<string> Scry::explode(const string& str, const char& ch) {
  string next;
  vector<string> result;
  for (string::const_iterator it = str.begin(); it != str.end(); it++) {
    if (*it == ch) {
      if (!next.empty()) {
        result.push_back(next);
        next.clear();
      }
    } else next += *it;
  }
  if (!next.empty()) result.push_back(next);
  return result;
}

string Scry::implode(const vector<string>& strs, const char& ch) {
  string result = "";
  for (auto it = strs.begin(); it != strs.end(); it++) {
    result += (*it) + ch;
  }
  return result;
}

string Scry::urlformat(string str) {
  regex space(" ");
  regex colon(":");
  str = regex_replace(str, space, "%20");
  str = regex_replace(str, colon, "%3A");
  return str;
}

string Scry::nameformat(string str) {
  regex apos("'");
  str = regex_replace(str, apos, "''");
  return str;
}

List * Scry::cards_search(string query) {
  query = urlformat(query);
  string url = "https://api.scryfall.com/cards/search?q=" + query;
  List * list = new List(wa->api_call(url));
  lists.push_back(list);
  return list;
}

string Scry::cachecard(List * list, bool recursive) {
  string names = "";
  vector <Card *> cards = allcards(list, true);
  if (recursive) {
    for (int i = 0; i < cards.size(); i++) {
      string name = nameformat(cards[i]->name());
      names += name + "\n";
      string temp = nameformat(cards[i]->json());
      if (i < list->cards().size()) {
        if (da->db_check("Cards", name)) {
          da->db_write("Cards", name, temp);
        } else da->db_new("Cards", name, temp);
      }
    }
  } else {
    for (int i = 0; i < cards.size(); i++) {
      string name = nameformat(cards[i]->name());
      names += name + "\n";
      string temp = nameformat(cards[i]->json());
      if (da->db_check("Cards", name)) {
        da->db_write("Cards", name, temp);
      } else da->db_new("Cards", name, temp);
    }
  }
  names.pop_back();
  return names;
}

List * Scry::cards_search_cache(string query) {
  query = urlformat(query);
  List * list;

  regex pages(".*&p"); smatch sm; regex_search(query, sm, pages);
  string search = string(sm[0]).substr(0, sm[0].length()-2);
  if (size(search) < 1) search = query;
  if (da->db_check("Lists", search)) {
    if (datecheck( da->db_read("Lists", search, "Updated") ) == 1) {
      string url = "https://api.scryfall.com/cards/search?q=" + query;
      list = new List(wa->api_call(url));
      lists.push_back(list);
      da->db_write("Lists", search, cachecard(list, true));
    } else {
      vector<string> strvec = explode(da->db_read("Lists", search, "Value"), '\n');
      vector<Card *> content;
      for (int i = 0; i < strvec.size(); i++)
	content.push_back( new Card( da->db_read("Cards", nameformat(strvec[i]), "Value").c_str() ) );
      list = new List( content );
      lists.push_back(list);
    }
  } else {
    string url = "https://api.scryfall.com/cards/search?q=" + query;
    list = new List(wa->api_call(url));
    lists.push_back(list);
    string names = cachecard(list, false);
    if (da->db_check("Lists", search)) {
      string temp = nameformat( da->db_read("Lists", search, "Value") );
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
    if (datecheck( da->db_read("Cards", name, "Updated") ) == 1) {
      card = cards_named(query);
      da->db_write("Cards", name, nameformat(card->json()));
    } else {
      card = new Card( da->db_read("Cards", name, "Value").c_str() );
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
    if (datecheck( da->db_read("Autocompletes", query, "Updated") ) == 1) {
      names = cards_autocomplete(query);
      string namestr = implode(names, '\n');
      da->db_write("Autocompletes", query, nameformat(namestr));
    } else {
      names = explode(da->db_read("Autocompletes", query, "Value"), '\n');
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
