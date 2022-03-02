//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#pragma once
#include <string>
#include <cstring>
#include <chrono>
#include <dlfcn.h>
#include <sqlite3.h>

using namespace std;

///This class is used to access the database
class DataAccess {
  public:
    DataAccess();
    ~DataAccess();

    virtual void db_init(string table);
    virtual bool db_check(string table, string search);
    virtual string db_read(string table, string search);
    virtual void db_write(string table, string key, string value);
    virtual void db_new(string table, string key, string value);
    virtual int datecheck(string table, string search);
  private:
    void * sqlite3_lib;
    typedef int (*o_handle)(const char *, sqlite3 **);
    o_handle sqlite3_open;
    typedef const char* (*e_handle)(sqlite3 *);
    e_handle sqlite3_errmsg;
    typedef int (*c_handle)(sqlite3 *);
    c_handle sqlite3_close;
    typedef int (*p_handle)(sqlite3 *, const char *, int, sqlite3_stmt **, const char **);
    p_handle sqlite3_prepare_v2;
    typedef int (*s_handle)(sqlite3_stmt *);
    s_handle sqlite3_step;
    typedef int (*f_handle)(sqlite3_stmt *);
    f_handle sqlite3_finalize;
    typedef const unsigned char* (*t_handle)(sqlite3_stmt *, int);
    t_handle sqlite3_column_text;

    sqlite3 *db;
    virtual string db_exec(string in);
};

