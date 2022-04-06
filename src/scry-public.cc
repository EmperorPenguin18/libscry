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
  List * list = new List(wa->api_call(url));
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

  if (da->datecheck("Lists", query) == 1) {
    list = cards_search(query);
    da->db_exec("Lists", query, cachecard(list));
  } else {
    vector<string> strvec = explode(da->db_exec("Lists", query), '\n');
    vector<Card *> content;
    for (int i = 0; i < strvec.size(); i++)
      content.push_back( new Card( da->db_exec("Cards", nameformat(strvec[i])).c_str() ) );
    list = new List( content );
    lists.push_back(list);
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

  if (da->datecheck("Cards", name) == 1) {
    card = cards_named(query);
    da->db_exec("Cards", name, nameformat(card->json()));
  } else {
    card = new Card( da->db_exec("Cards", name).c_str() );
    cards.push_back(card);
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

  if (da->datecheck("Autocompletes", query) == 1) {
    names = cards_autocomplete(query);
    string namestr = implode(names, '\n');
    da->db_exec("Autocompletes", query, nameformat(namestr));
  } else {
    names = explode(da->db_exec("Autocompletes", query), '\n');
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
