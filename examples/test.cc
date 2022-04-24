//Test all functions example
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include <dlfcn.h>
#include <time.h>
#include <iostream>
#include <scry/scry.h>

using namespace std;

int main(int argc, char **argv)
{
  char fname[50] = "";
  strcat(fname, getenv("HOME"));
  strcat(fname, "/.local/share/libscry.db");
  remove(fname);

  void* handle = dlopen("libscry.so", RTLD_LAZY);
  Scry* (*create)();
  void (*destroy)(Scry*);
  create = (Scry* (*)())dlsym(handle, "create_object");
  destroy = (void (*)(Scry*))dlsym(handle, "destroy_object");

  Scry* scry = (Scry*)create();

  cout << "cards_search(\"cmc>3\")" << endl;
  List* stuff = scry->cards_search("cmc>3");
  vector<Card*> cards = stuff->cards();
  size_t i;
  for (i = 0; i < 10; i++)
    cout << cards[i]->name() << endl;
  cout << ".." << endl;
  for (i = cards.size()-10; i < cards.size(); i++)
    cout << cards[i]->name() << endl;
  cout << endl;

  cout << "cards_search_cache(\"commander:wubrg\")" << endl;
  clock_t start = clock();
  stuff = scry->cards_search_cache("commander:wubrg");
  cout << "Time taken: " << ((double) (clock() - start)) / CLOCKS_PER_SEC << endl;
  cards = stuff->cards();
  for (i = 0; i < 10; i++)
    cout << cards[i]->name() << endl;
  cout << ".." << endl;
  for (i = cards.size()-10; i < cards.size(); i++)
    cout << cards[i]->name() << endl;
  cout << endl;

  cout << "cards_search_cache(\"commander:wubrg\")" << endl;
  start = clock();
  stuff = scry->cards_search_cache("commander:wubrg");
  cout << "Time taken: " << ((double) (clock() - start)) / CLOCKS_PER_SEC << endl;
  cards = stuff->cards();
  for (i = 0; i < 10; i++)
    cout << cards[i]->name() << endl;
  cout << ".." << endl;
  for (i = cards.size()-10; i < cards.size(); i++)
    cout << cards[i]->name() << endl;
  cout << endl;

  cout << "cards_named(\"vannifar\")" << endl;
  Card* card = scry->cards_named("vannifar");
  cout << card->name() << endl;
  cout << endl;

  cout << "cards_named_cache(\"agadeem's awakening\")" << endl;
  start = clock();
  card = scry->cards_named_cache("agadeem's awakening");
  cout << "Time taken: " << ((double) (clock() - start)) / CLOCKS_PER_SEC << endl;
  cout << "split(card)" << endl;
  cards = scry->split(card);
  cout << cards[1]->name() << endl;
  cout << endl;

  cout << "cards_named_cache(\"agadeem's awakening\")" << endl;
  start = clock();
  card = scry->cards_named_cache("agadeem's awakening");
  cout << "Time taken: " << ((double) (clock() - start)) / CLOCKS_PER_SEC << endl;
  cout << card->name() << endl;
  cout << card->mana_cost() << endl;
  cout << card->type_line() << endl;
  cout << card->oracle_text() << endl;
  cout << card->power() << endl;
  cout << card->toughness() << endl;
  cout << card->dual_sided() << endl;
  cout << card->json() << endl;
  cout << card->loyalty() << endl;
  cout << endl;

  cout << "cards_named(\"vannifar\", &img_size)" << endl;
  size_t img_size = 0;
  byte* image = scry->cards_named("vannifar", &img_size);
  FILE* fp = fopen("test0.jpg", "wb");
  fwrite((FILE*)image, sizeof(byte), img_size/sizeof(byte), fp);
  fclose(fp);
  free(image);
  cout << endl;

  cout << "cards_named_cache(\"agadeem's awakening\", &img_size)" << endl;
  start = clock();
  image = scry->cards_named_cache("agadeem's awakening", &img_size);
  cout << "Time taken: " << ((double) (clock() - start)) / CLOCKS_PER_SEC << endl;
  fp = fopen("test1.jpg", "wb");
  fwrite((FILE*)image, sizeof(byte), img_size/sizeof(byte), fp);
  fclose(fp);
  free(image);
  cout << endl;

  cout << "cards_named_cache(\"agadeem's awakening\", &img_size)" << endl;
  start = clock();
  image = scry->cards_named_cache("agadeem's awakening", &img_size);
  cout << "Time taken: " << ((double) (clock() - start)) / CLOCKS_PER_SEC << endl;
  fp = fopen("test2.jpg", "wb");
  fwrite((FILE*)image, sizeof(byte), img_size/sizeof(byte), fp);
  fclose(fp);
  cout << endl;

  cout << "cards_autocomplete(\"oko\")" << endl;
  vector<string> complete = scry->cards_autocomplete("oko");
  for (i = 0; i < complete.size(); i++)
    cout << complete[i] << endl;
  cout << endl;

  cout << "cards_autocomplete_cache(\"rain\")" << endl;
  start = clock();
  complete = scry->cards_autocomplete_cache("rain");
  cout << "Time taken: " << ((double) (clock() - start)) / CLOCKS_PER_SEC << endl;
  for (i = 0; i < complete.size(); i++)
    cout << complete[i] << endl;
  cout << endl;

  cout << "cards_autocomplete_cache(\"rain\")" << endl;
  start = clock();
  complete = scry->cards_autocomplete_cache("rain");
  cout << "Time taken: " << ((double) (clock() - start)) / CLOCKS_PER_SEC << endl;
  for (i = 0; i < complete.size(); i++)
    cout << complete[i] << endl;
  cout << endl;

  cout << "cards_random()" << endl;
  card = scry->cards_random();
  cout << card->name() << endl;
  cout << endl;

  cout << "Test no card" << endl;
  card = scry->cards_named("");
  if (!card) cout << "Worked" << endl;
  cout << endl;

  cout << "Test no results" << endl;
  stuff = scry->cards_search("");
  if (!stuff) cout << "Worked" << endl;
  cout << endl;

  destroy(scry);
}

/* Compiled with `g++ test.cc -o test`. */
