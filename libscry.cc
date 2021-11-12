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

static size_t cb(void *data, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)userp;

  char *ptr = (char *)realloc(mem->response, mem->size + realsize + 1);
  if(ptr == NULL)
    return 0;  /* out of memory! */

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
}

Scry::~Scry() {
  curl_easy_cleanup(easyhandle);
  curl_global_cleanup();
  sqlite3_close(db);
  while (!cards.empty()) {
    delete cards.back();
    cards.pop_back();
  }
}

char * Scry::api_call(const char * url) {
  curl_easy_setopt(easyhandle, CURLOPT_URL, url);
  struct memory chunk = {0};
  curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, (void *)&chunk);
  CURLcode success = curl_easy_perform(easyhandle);
  if (success != 0) {
    fprintf(stderr, "Errored with CURLcode %i\n", success);
    exit(success);
  }
  return chunk.response;
}

string Scry::db_exec(const char * cmd)
{
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, cmd, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        throw string(sqlite3_errmsg(db));

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
        string errmsg(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw errmsg;
    }
    /*if (rc == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw string("customer not found");
    }*/

    string output;
    char temp[7] = ""; strncpy(temp, cmd, 6); temp[6] = '\0';
    if (strcmp(temp, "SELECT") == 0) output = string((char*)sqlite3_column_text(stmt, 0));

    sqlite3_finalize(stmt);
    return output;
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

vector<Card *> Scry::cards_search(string query) {
  string url = "https://api.scryfall.com/cards/search?q=" + query;
  char * rawjson;
  Document doc;
  vector<Card *> output;
  do {
    rawjson = api_call(url.c_str());
    doc.Parse(rawjson);
    for (int i = 0; i < doc["data"].Size(); i++) {
      StringBuffer buffer;
      Writer<StringBuffer> writer(buffer);
      doc["data"][i].Accept(writer);
      output.push_back(new Card(buffer.GetString()));
      cards.push_back(output.back());
    }
    if (!doc["has_more"].GetBool()) break;
    url = doc["next_page"].GetString();
  } while (true);
  return output;
}

Card * Scry::cards_named(string query) {
  string url = "https://api.scryfall.com/cards/named?fuzzy=" + query;
  Card * card = new Card(api_call(url.c_str()));
  cards.push_back(card);
  return card;
}

Card * Scry::cards_named_cache(string query) {
  Card * card;
  query[0] = toupper(query[0]);

  string str = "SELECT COUNT(1) FROM Cards WHERE Name='" + query + "';";
  if (db_exec(str.c_str()).compare("1") == 0) {
    str = "SELECT Updated FROM Cards WHERE Name='" + query + "';";
    if (datecheck( db_exec(str.c_str()) ) == 1) {
      card = cards_named(query);
      str = "UPDATE Cards SET Updated=datetime(), Data='";
      string temp = card->json();
      str.append(temp).append("' WHERE Name='").append(query).append("';");
      db_exec(str.c_str());
    } else {
      str = "SELECT Data FROM Cards WHERE Name='" + query + "';";
      card = new Card( db_exec(str.c_str()).c_str() );
      cards.push_back(card);
    }
  } else {
    card = cards_named(query);
    str = "INSERT INTO Cards VALUES ('" + query + "', datetime(), '";
    string temp = card->json();
    str.append(temp).append("');");
    db_exec(str.c_str());
  }

  return card;
}

vector<string> Scry::cards_autocomplete(string query) {
  string url = "https://api.scryfall.com/cards/autocomplete?q=" + query;
  Document doc;
  doc.Parse(api_call(url.c_str()));
  const Value& a = doc["data"];
  vector<string> output;
  for (auto& v : a.GetArray()) output.push_back(v.GetString());
  return output;
}

Card * Scry::cards_random() {
  string url = "https://api.scryfall.com/cards/random";
  Card * card = new Card(api_call(url.c_str()));
  cards.push_back(card);
  return card;
}

Card::Card(const char * rawjson) {
  data.Parse(rawjson);
}

string Card::name() {
  return data["name"].GetString();
}

string Card::json() {
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  data.Accept(writer);
  return buffer.GetString();
}
