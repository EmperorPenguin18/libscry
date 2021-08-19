#include "myclass.h"
#include <iostream>

using namespace std;

extern "C" MyClass* create_object()
{
  return new MyClass;
}

extern "C" void destroy_object( MyClass* object )
{
  delete object;
}

MyClass::MyClass()
{
  x = 20;
}

void MyClass::DoSomething()
{
  cout<<x<<endl;
}
