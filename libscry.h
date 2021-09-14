#ifndef __SCRY_H__
#define __SCRY_H__
#include <curl/curl.h>

class MyClass
{
public:
  MyClass();

  /* use virtual otherwise linker will try to perform static linkage */
  virtual void DoSomething();

private:
  CURL easyhandle;
  CURLcode success;
};

#endif
