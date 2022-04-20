//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#pragma once
#include <string>
#include <cstring>
#include <chrono>
#include <vector>
#include <thread>
#include <dlfcn.h>
#include <sqlite3.h>

#ifdef DEBUG
#include <iostream>
#endif

using namespace std;

///This class is used to access the database
class DataAccess {
  public:
    DataAccess(const char *);
    ~DataAccess();

    virtual int datecheck(const char*, const char*);
    virtual void db_exec(const char*);
    virtual byte* db_exec(const char*, const char*, size_t*);
    virtual void db_exec(const char*, const char*, byte*, const size_t&);
    virtual void db_exec(const char*, vector<string>, vector<string>);
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
    typedef const void* (*t_handle)(sqlite3_stmt *, int);
    t_handle sqlite3_column_blob;
    typedef int (*y_handle)(sqlite3_stmt *, int);
    y_handle sqlite3_column_bytes;
    typedef int (*x_handle)(sqlite3 *, const char *, int(*)(void *, int, char **, char **), void *, char **);
    x_handle sqlite3_exec;
    typedef sqlite3_backup* (*i_handle)(sqlite3 *, const char *, sqlite3 *, const char *);
    i_handle sqlite3_backup_init;
    typedef int (*b_handle)(sqlite3_backup *, int);
    b_handle sqlite3_backup_step;
    typedef int (*n_handle)(sqlite3_backup *);
    n_handle sqlite3_backup_finish;
    typedef int (*r_handle)(sqlite3 *);
    r_handle sqlite3_errcode;
    typedef int (*d_handle)(sqlite3_stmt *, int, const void*, int, void(*)(void*));
    d_handle sqlite3_bind_blob;

    sqlite3 *db;
    char *fname;
    vector<byte*> bytes;
    virtual void db_copy(int);
    virtual byte* sql_read(sqlite3 *, const char*, size_t*);
    virtual void sql_write(sqlite3 *, const char*, byte*, const size_t&);
};

