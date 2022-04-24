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

#ifdef DEBUG
void print_stacktrace(int signum) {
  const char * signals[31] = {"SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABR", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGSTKFLT", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGIO", "SIGPWR", "SIGUNUSED"};
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

List* Scry::cards_search(string in) {
  string query = string(urlformat(in));
  string url = "https://api.scryfall.com/cards/search?q=" + query;
#ifdef DEBUG
  cerr << "URL: " << url << endl;
#endif
  size_t size = 0;
  List* list;
  try {
    list = new List(wa->api_call(url));
  } catch(const char* e) {
    return nullptr;
  }
#ifdef DEBUG
  cerr << "First card: " << list->cards()[0]->name() << endl;
#endif
  lists.push_back(list);
  List* full_list = allcards(list);
#ifdef DEBUG
  cerr << "Full first card: " << full_list->cards()[0]->name() << endl;
#endif
  return full_list;
}

List* Scry::cards_search_cache(string in) {
  char* query = urlformat(in);
  List* list;

  if (da->datecheck("Lists", query) == 1) {
    list = cards_search(in);
    if (!list) return list;
    char* str = cachecard(list);
    da->db_exec("Lists", query, (byte*)str, strlen(str));
    free(str);
  } else {
    cstring_t data; data.len = 0; data.max = 0; data.str = NULL;
    vector<char*> names = explode(da->db_exec("Lists", query), '\n');
    string_cat(&data, "{\"object\":\"list\",\"data\":[");
    for (size_t i = 0; i < names.size(); i++) {
      string_cat(&data, da->db_exec("Cards", names[i]));
      string_cat(&data, ",");
    }
    data.len--;
    string_cat(&data, "],\"has_more\":false}");
#ifdef DEBUG
    cerr << "New list (last 50 chars): " << &(data.str[strlen(data.str)-50]) << endl;
#endif
    list = new List(data.str);
    lists.push_back(list);
    free(data.str);
  }

  free(query);
  return list;
}

Card* Scry::cards_named(string in) {
  string query = string(urlformat(in));
  string url = "https://api.scryfall.com/cards/named?fuzzy=" + query;
  Card* card;
  try {
    card = new Card(wa->api_call(url));
  } catch (const char* e) {
    return nullptr;
  }
  cards.push_back(card);
  return card;
}

Card* Scry::cards_named_cache(string in) {
  Card* card;
  char* name = nameformat(in);

  if (da->datecheck("Cards", name) == 1) {
    card = cards_named(in);
    if (!card) return card;
    string str = card->json();
    char* cstr = new char[str.length()+1];
    strcpy(cstr, str.c_str());
    da->db_exec("Cards", name, (byte*)cstr, str.length());
  } else {
    card = new Card( da->db_exec("Cards", name) );
    cards.push_back(card);
  }

  free(name);
  return card;
}

byte* Scry::cards_named(string in, size_t *size) {
  string query = string(urlformat(in));
  string url = "https://api.scryfall.com/cards/named?fuzzy=" + query + "&format=image&version=border_crop";
  byte* image = wa->api_call(url, size);
  return image;
}

byte* Scry::cards_named_cache(string in, size_t *size) {
  byte* image;
  char* name = nameformat(in);

  if (da->datecheck("Images", name) == 1) {
    image = cards_named(in, size);
    da->db_exec("Images", name, image, *size);
  } else {
    image = da->db_exec("Images", name, size);
  }

  free(name);
  return image;
}

vector<string> Scry::cards_autocomplete(string in) {
  string query = string(urlformat(in));
  string url = "https://api.scryfall.com/cards/autocomplete?q=" + query;
  Document doc;
  doc.Parse(wa->api_call(url));
  const Value& a = doc["data"];
  vector<string> output;
  for (auto& v : a.GetArray()) output.push_back(v.GetString());
  return output;
}

vector<string> Scry::cards_autocomplete_cache(string in) {
  char* query = urlformat(in);
  vector<string> names;

  if (da->datecheck("Autocompletes", query) == 1) {
    names = cards_autocomplete(in);
    string namestr = implode(names, '\n');
    const char* str = namestr.c_str();
    da->db_exec("Autocompletes", query, (byte*)str, strlen(str));
  } else {
    vector<char*> rawnames = explode(da->db_exec("Autocompletes", query), '\n');
    for (size_t i = 0; i < rawnames.size(); i++) names.push_back(string(rawnames[i]));
  }

  free(query);
  return names;
}

Card* Scry::cards_random() {
  string url = "https://api.scryfall.com/cards/random";
  Card* card = new Card(wa->api_call(url));
  cards.push_back(card);
  return card;
}

vector<Card*> Scry::split(Card* card) {
  Document doc; doc.Parse(card->json().c_str());
  vector<Card*> output;
  for (int i = 0; i < doc["card_faces"].Size(); i++) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc["card_faces"][i].Accept(writer);
    Card * card = new Card(buffer.GetString());
    output.push_back(card); cards.push_back(card);
  }
  return output;
}
