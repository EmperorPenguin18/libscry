#include <dlfcn.h>
#include <iostream>
#include "scry.h"

using namespace std;

int main(int argc, char **argv)
{
  void* handle = dlopen("/usr/lib/libscry.so", RTLD_LAZY);

  MyClass* (*create)();
  void (*destroy)(MyClass*);

  create = (MyClass* (*)())dlsym(handle, "create_object");
  destroy = (void (*)(MyClass*))dlsym(handle, "destroy_object");

  MyClass* myClass = (MyClass*)create();
  myClass->DoSomething();
  destroy( myClass );
}
