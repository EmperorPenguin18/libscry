#include "libscry.h"

using namespace std;
using namespace rapidjson;
using namespace std::chrono;

extern "C" Scry* create_object() {
  return new Scry;
}

extern "C" void destroy_object( Scry* object ) {
  delete object;
}

struct memory {
  char *response;
  size_t size;
};

static size_t cb(void *data, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)userp;

  char *ptr = (char *)realloc(mem->response, mem->size + realsize + 1);
  if (ptr == NULL) return 0; // out of memory!

  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;

  return realsize;
}

Scry::Scry() {
  curl_global_init(CURL_GLOBAL_ALL);
  easyhandle = curl_easy_init();
  curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, cb);
  curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(easyhandle, CURLOPT_HTTPGET, 1);
  char * cachedir = getenv("XDG_CACHE_HOME");
  int rc;
  if (cachedir != NULL) rc = sqlite3_open(strcat(cachedir, "/libscry.db"), &db);
  else rc = sqlite3_open(strcat(getenv("HOME"), "/.cache/libscry.db"), &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  }
  db_exec("CREATE TABLE IF NOT EXISTS Cards(Name TEXT, Updated DATETIME, Data TEXT);");
  db_exec("CREATE TABLE IF NOT EXISTS Lists(Query TEXT, Updated DATETIME, Names TEXT);");
  db_exec("CREATE TABLE IF NOT EXISTS Autocompletes(Str TEXT, Updated DATETIME, Names TEXT);");
}

Scry::~Scry() {
  curl_easy_cleanup(easyhandle);
  curl_global_cleanup();
  sqlite3_close(db);
  while (!cards.empty()) {
    delete cards.back();
    cards.pop_back();
  }
  while (!lists.empty()) {
    delete lists.back();
    lists.pop_back();
  }
}

char * Scry::api_call(string url) {
  curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
  struct memory chunk = {0};
  curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, (void *)&chunk);
  CURLcode success = curl_easy_perform(easyhandle);
  if (success != 0) {
    fprintf(stderr, "Errored with CURLcode %i\n", success);
    exit(success);
  }
  return chunk.response;
}

string Scry::db_exec(string in) {
  const char * cmd = in.c_str();
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, cmd, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "DB error: %s\n", sqlite3_errmsg(db));
    exit(1);
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    fprintf(stderr, "DB error: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    exit(1);
  }
  string output;
  char temp[7] = ""; strncpy(temp, cmd, 6); temp[6] = '\0';
  if (strcmp(temp, "SELECT") == 0) output = string((char*)sqlite3_column_text(stmt, 0));
  sqlite3_finalize(stmt);
  return output;
}

bool Scry::db_check(string table, string search) {
  string cmd;
  if (table == "Cards") cmd = "SELECT COUNT(1) FROM Cards WHERE Name='" + search + "';";
  else if (table == "Lists") cmd = "SELECT COUNT(1) FROM Lists WHERE Query='" + search + "';";
  else cmd = "SELECT COUNT(1) FROM Autocompletes WHERE Str='" + search + "';";
  if (db_exec(cmd).compare("1") == 0) return true;
  return false;
}

string Scry::db_read(string table, string search, string column) {
  string cmd;
  if (table == "Cards") cmd = "SELECT " + column + " FROM Cards WHERE Name='" + search + "';";
  else if (table == "Lists") cmd = "SELECT " + column + " FROM Lists WHERE Query='" + search + "';";
  else cmd = "SELECT " + column + " FROM Autocompletes WHERE Str='" + search + "';";
  return db_exec(cmd);
}

void Scry::db_write(string table, string key, string value) {
  string cmd;
  if (table == "Cards") cmd = "UPDATE Cards SET Updated=datetime(), Data='" + value + "' WHERE Name='" + key + "';";
  else if (table == "Lists") cmd = "UPDATE Lists SET Updated=datetime(), Names='" + value + "' WHERE Query='" + key + "';";
  else cmd = "UPDATE Autocompletes SET Updated=datetime(), Names='" + value + "' WHERE Str='" + key + "';";
  db_exec(cmd);
}

void Scry::db_new(string table, string key, string value) {
  string cmd;
  if (table == "Cards") cmd = "INSERT INTO Cards VALUES ('" + key + "', datetime(), '" + value + "');";
  else if (table == "Lists") cmd = "INSERT INTO Lists VALUES ('" + key + "', datetime(), '" + value + "');";
  else cmd = "INSERT INTO Autocompletes VALUES ('" + key + "', datetime(), '" + value + "');";
  db_exec(cmd);
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
  List * list = new List(this, api_call(url), false);
  lists.push_back(list);
  return list;
}

string Scry::cachecard(List * list, bool recursive) {
  string names = "";
  if (recursive) {
    for (int i = 0; i < list->allcards().size(); i++) {
      string name = nameformat(list->allcards()[i]->name());
      names += name + "\n";
      string temp = nameformat(list->allcards()[i]->json());
      if (i < list->cards().size()) {
        if (db_check("Cards", name)) {
          db_write("Cards", name, temp);
        } else db_new("Cards", name, temp);
      }
    }
  } else {
    for (int i = 0; i < list->cards().size(); i++) {
      string name = nameformat(list->cards()[i]->name());
      names += name + "\n";
      string temp = nameformat(list->cards()[i]->json());
      if (db_check("Cards", name)) {
        db_write("Cards", name, temp);
      } else db_new("Cards", name, temp);
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
  if (db_check("Lists", search)) {
    if (datecheck( db_read("Lists", search, "Updated") ) == 1) {
      string url = "https://api.scryfall.com/cards/search?q=" + query;
      list = new List(this, api_call(url), true);
      lists.push_back(list);
      db_write("Lists", search, cachecard(list, true));
    } else {
      vector<string> strvec = explode(db_read("Lists", search, "Names"), '\n');
      vector<Card *> content;
      for (int i = 0; i < strvec.size(); i++)
	content.push_back( new Card( db_read("Cards", nameformat(strvec[i]), "Data").c_str() ) );
      list = new List( content );
      lists.push_back(list);
    }
  } else {
    string url = "https://api.scryfall.com/cards/search?q=" + query;
    list = new List(this, api_call(url), true);
    lists.push_back(list);
    string names = cachecard(list, false);
    if (db_check("Lists", search)) {
      string temp = nameformat( db_read("Lists", search, "Names") );
      db_write("Lists", search, names + "\n" + temp);
    } else db_new("Lists", search, names);
  }

  return list;
}

Card * Scry::cards_named(string query) {
  query = urlformat(query);
  string url = "https://api.scryfall.com/cards/named?fuzzy=" + query;
  Card * card = new Card(api_call(url));
  cards.push_back(card);
  return card;
}

Card * Scry::cards_named_cache(string query) {
  query = urlformat(query);
  Card * card;
  query[0] = toupper(query[0]);
  string name = nameformat(query);

  if (db_check("Cards", name)) {
    if (datecheck( db_read("Cards", name, "Updated") ) == 1) {
      card = cards_named(query);
      db_write("Cards", name, nameformat(card->json()));
    } else {
      card = new Card( db_read("Cards", name, "Data").c_str() );
      cards.push_back(card);
    }
  } else {
    card = cards_named(query);
    db_new("Cards", name, nameformat(card->json()));
  }

  return card;
}

vector<string> Scry::cards_autocomplete(string query) {
  query = urlformat(query);
  string url = "https://api.scryfall.com/cards/autocomplete?q=" + query;
  Document doc;
  doc.Parse(api_call(url));
  const Value& a = doc["data"];
  vector<string> output;
  for (auto& v : a.GetArray()) output.push_back(v.GetString());
  return output;
}

vector<string> Scry::cards_autocomplete_cache(string query) {
  query = urlformat(query);
  vector<string> names;

  if (db_check("Autocompletes", query)) {
    if (datecheck( db_read("Autocompletes", query, "Updated") ) == 1) {
      names = cards_autocomplete(query);
      string namestr = implode(names, '\n');
      db_write("Autocompletes", query, nameformat(namestr));
    } else {
      names = explode(db_read("Autocompletes", query, "Names"), '\n');
    }
  } else {
    names = cards_autocomplete(query);
    string namestr = implode(names, '\n');
    db_new("Autocompletes", query, nameformat(namestr));
  }

  return names;
}

Card * Scry::cards_random() {
  string url = "https://api.scryfall.com/cards/random";
  Card * card = new Card(api_call(url));
  cards.push_back(card);
  return card;
}

vector<Card *> Scry::split(Card * card) {
  Document doc; doc.Parse(card->json().c_str());
  vector<Card *> output;
  StringBuffer buffer1;
  Writer<StringBuffer> writer1(buffer1);
  doc["card_faces"][0].Accept(writer1);
  Card * card1 = new Card(buffer1.GetString());
  output.push_back(card1); cards.push_back(card1);
  StringBuffer buffer2;
  Writer<StringBuffer> writer2(buffer2);
  doc["card_faces"][1].Accept(writer2);
  Card * card2 = new Card(buffer2.GetString());
  output.push_back(card2); cards.push_back(card2);
  return output;
}

Card::Card(const char * rawjson) {
  data.Parse(rawjson);
}

string Card::name() {
  return data["name"].GetString();
}

string Card::mana_cost() {
  if (!data.HasMember("mana_cost")) return "";
  return data["mana_cost"].GetString();
}

string Card::type_line() {
  return data["type_line"].GetString();
}

string Card::oracle_text() {
  if (!data.HasMember("oracle_text")) return "";
  return data["oracle_text"].GetString();
}

string Card::power() {
  if (!data.HasMember("power")) return "";
  return data["power"].GetString();
}

string Card::toughness() {
  if (!data.HasMember("toughness")) return "";
  return data["toughness"].GetString();
}

bool Card::dual_sided() {
  return data.HasMember("card_faces");
}

string Card::json() {
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  data.Accept(writer);
  return buffer.GetString();
}

List::List(Scry * scry, const char * rawjson, bool cache) {
  Document doc;
  doc.Parse(rawjson);
  for (int i = 0; i < doc["data"].Size(); i++) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc["data"][i].Accept(writer);
    content.push_back(new Card(buffer.GetString()));
  }
  if (doc["has_more"].GetBool()) {
    string url = doc["next_page"].GetString();
    regex q("q=.*&");
    regex page("page=.*&q");
    smatch sm1; regex_search(url, sm1, q);
    smatch sm2; regex_search(url, sm2, page);
    string query = string(sm1[0]).substr(2, sm1[0].length()-2) + string(sm2[0]).substr(0, sm2[0].length()-2);
    if (cache) nextpage = scry->cards_search_cache(query);
    else nextpage = scry->cards_search(query);
  } else nextpage = nullptr;
}

List::List(vector<Card *> input) {
  content = input;
  nextpage = nullptr;
}

List::~List() {
  while (!content.empty()) {
    delete content.back();
    content.pop_back();
  }
}

vector<Card *> List::cards() {
  return content;
}

vector<Card *> List::allcards() {
  vector<Card *> output = cards();
  if (nextpage != nullptr) {
    vector<Card *> append = nextpage->allcards();
    output.insert(output.end(), append.begin(), append.end());
  }
  return output;
}
