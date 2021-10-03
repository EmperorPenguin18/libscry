#ifndef __SCRY_H__
#define __SCRY_H__
#include <curl/curl.h>
#include <sqlite3.h>
#include <string>

class Card
{
  public:
    Card(string name);
  
    string getName();
  private:
    string name;
}

class Scry
{
  public:
    Scry();

    /* use virtual otherwise linker will try to perform static linkage */
    virtual Card cards_named(string search);
    virtual void cleanup();

  private:
    CURL easyhandle;
    sqlite3 *db;
    int rc;
};

#endif
