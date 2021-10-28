#include <dlfcn.h>
#include <iostream>
#include <libscry.h>

using namespace std;

int main(int argc, char **argv)
{
  /* This block is what allows the library to be dynamic. */
  void* handle = dlopen("/usr/lib/libscry.so", RTLD_LAZY);
  Scry* (*create)();
  void (*destroy)(Scry*);
  create = (Scry* (*)())dlsym(handle, "create_object");
  destroy = (void (*)(Scry*))dlsym(handle, "destroy_object");

  /* Create an instance of the Scry class.
     This class handles all of the API calls and cacheing. */
  Scry* scry = (Scry*)create();
  /* Use the cards_named function as an example.
     Takes the name of a card and returns its data. */
  Card * island = scry->cards_named("island");
  /* Output the name of the returned card (will be "Island"). */
  cout << island->name() << endl;
  /* Prevents memory leaks.
     Without this line std::bad_alloc will be thrown. */
  destroy(scry);
}

/* Compiled with `g++ simple.cc -ldl -o simple`. */
