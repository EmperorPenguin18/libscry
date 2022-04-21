//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "scry.h"

#ifdef DEBUG
const char * signals[31] = {"SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABR", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGSTKFLT", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGIO", "SIGPWR", "SIGUNUSED"};

void print_stacktrace(int signum) {
  printf("\nReceived signal %d: %s\n", signum, signals[signum-1]);
  int nptrs;
  void *buffer[BT_BUF_SIZE];
  char **strings;

  nptrs = backtrace(buffer, BT_BUF_SIZE);
  printf("backtrace() returned %d addresses\n", nptrs);

  /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
     would produce similar output to the following: */

  strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
     perror("backtrace_symbols");
     exit(signum);
  }

  for (int j = 0; j < nptrs; j++)
      printf("%s\n", strings[j]);

  free(strings);
  exit(signum);
}
#endif

using namespace std;
using namespace rapidjson;

extern "C" Scry* create_object() {
  return new Scry;
}

extern "C" void destroy_object( Scry* object ) {
  delete object;
}

Scry::Scry() {
#ifdef DEBUG
  signal(SIGSEGV, print_stacktrace);
  signal(SIGABRT, print_stacktrace);
#endif
  vector<string> temp;
  temp.push_back("api.scryfall.com");
  wa = new WebAccess(temp, 50, 20);
  da = new DataAccess("libscry");
  da->db_exec("Cards");
  da->db_exec("Lists");
  da->db_exec("Autocompletes");
  da->db_exec("Images");
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
#ifdef DEBUG
  cerr << "URL: " << url << endl;
#endif
  List * list = new List((char*)wa->api_call(url).response);
#ifdef DEBUG
  cerr << "First card: " << list->cards()[0]->name() << endl;
#endif
  lists.push_back(list);
  List * full_list = allcards(list);
#ifdef DEBUG
  cerr << "Full first card: " << full_list->cards()[0]->name() << endl;
#endif
  return full_list;
}

List * Scry::cards_search_cache(string query) {
  query = urlformat(query);
  List * list;

  if (da->datecheck("Lists", query.c_str()) == 1) {
    list = cards_search(query);
    const char* str = cachecard(list).c_str();
    da->db_exec("Lists", query.c_str(), (byte*)str, strlen(str)+1);
  } else {
    cstring_t data; data.len = 0; data.max = 0; data.str = NULL;
    size_t size = 0;
    vector<string> names = explode((char*)da->db_exec("Lists", query.c_str(), &size), '\n');
    string_cat(&data, "{\"data\":[");
    for (size_t i = 0; i < names.size(); i++) {
      string_cat(&data, (char*)da->db_exec("Cards", names[i].c_str(), &size));
      string_cat(&data, ",");
    }
    data.len--;
    string_cat(&data, "],\"has_more\":false}");
#ifdef DEBUG
    string str(data.str);
    cerr << "New list (last 50 chars): " << str.substr(str.length()-50, 50) << endl;
#endif
    list = new List(data.str);
    lists.push_back(list);
    free(data.str);
  }

  return list;
}

Card * Scry::cards_named(string query) {
  query = urlformat(query);
  string url = "https://api.scryfall.com/cards/named?fuzzy=" + query;
  Card * card = new Card((char*)wa->api_call(url).response);
  cards.push_back(card);
  return card;
}

Card * Scry::cards_named_cache(string query) {
  query = urlformat(query);
  Card * card;
  query[0] = toupper(query[0]);
  string name = nameformat(query);

  if (da->datecheck("Cards", name.c_str()) == 1) {
    card = cards_named(query);
    const char* str = nameformat(card->json()).c_str();
    da->db_exec("Cards", name.c_str(), (byte*)str, strlen(str)+1);
  } else {
    size_t size = 0;
    card = new Card( (char*)da->db_exec("Cards", name.c_str(), &size) );
    cards.push_back(card);
  }

  return card;
}

byte * Scry::cards_named(string query, size_t *size) {
  query = urlformat(query);
  string url = "https://api.scryfall.com/cards/named?fuzzy=" + query + "&format=image&version=border_crop";
  struct WebAccess::memory mem = wa->api_call(url);
  *size = mem.size;
  return mem.response;
}

byte * Scry::cards_named_cache(string query, size_t *size) {
  query = urlformat(query);
  byte *image;
  query[0] = toupper(query[0]);
  string name = nameformat(query);

  if (da->datecheck("Images", name.c_str()) == 1) {
    image = cards_named(query, size);
    da->db_exec("Images", name.c_str(), image, *size);
  } else {
    image = da->db_exec("Images", name.c_str(), size);
  }

  return image;
}

vector<string> Scry::cards_autocomplete(string query) {
  query = urlformat(query);
  string url = "https://api.scryfall.com/cards/autocomplete?q=" + query;
  Document doc;
  doc.Parse((char*)wa->api_call(url).response);
  const Value& a = doc["data"];
  vector<string> output;
  for (auto& v : a.GetArray()) output.push_back(v.GetString());
  return output;
}

vector<string> Scry::cards_autocomplete_cache(string query) {
  query = urlformat(query);
  vector<string> names;

  if (da->datecheck("Autocompletes", query.c_str()) == 1) {
    names = cards_autocomplete(query);
    string namestr = implode(names, '\n');
    const char* str = nameformat(namestr).c_str();
    da->db_exec("Autocompletes", query.c_str(), (byte*)str, strlen(str)+1);
  } else {
    size_t size = 0;
    names = explode((char*)da->db_exec("Autocompletes", query.c_str(), &size), '\n');
  }

  return names;
}

Card * Scry::cards_random() {
  string url = "https://api.scryfall.com/cards/random";
  Card * card = new Card((char*)wa->api_call(url).response);
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
