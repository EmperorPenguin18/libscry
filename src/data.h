//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#pragma once
#include <string>
#include <cstring>
#include <sqlite3.h>

using namespace std;

///This class is used to access the database
class DataAccess {
  public:
    DataAccess();
    ~DataAccess();

    virtual void db_init(string table);
    virtual bool db_check(string table, string search);
    virtual string db_read(string table, string search, string column);
    virtual void db_write(string table, string key, string value);
    virtual void db_new(string table, string key, string value);
  private:
    sqlite3 *db;
    virtual string db_exec(string in);
};

