//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "data.h"

using namespace std;

DataAccess::DataAccess() {
  char * cachedir = getenv("XDG_CACHE_HOME");
  int rc;
  if (cachedir != NULL) rc = sqlite3_open(strcat(cachedir, "/libscry.db"), &db);
  else rc = sqlite3_open(strcat(getenv("HOME"), "/.cache/libscry.db"), &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  }
}

DataAccess::~DataAccess() {
  sqlite3_close(db);
}

string DataAccess::db_exec(string in) {
  const char * cmd = in.c_str();
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, cmd, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "DB error: %s\n", sqlite3_errmsg(db));
    exit(1);
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    fprintf(stderr, "DB error: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    exit(1);
  }
  string output;
  string temp = in.substr(0, 6);
  if (temp == "SELECT") output = string((char*)sqlite3_column_text(stmt, 0));
  sqlite3_finalize(stmt);
  return output;
}

void DataAccess::db_init(string table) {
  db_exec("CREATE TABLE IF NOT EXISTS " + table + "(Key TEXT, Updated DATETIME, Value TEXT);");
}

bool DataAccess::db_check(string table, string search) {
  string cmd = "SELECT COUNT(1) FROM " + table + " WHERE Key='" + search + "';";
  if (db_exec(cmd).compare("1") == 0) return true;
  return false;
}

string DataAccess::db_read(string table, string search, string column) {
  string cmd = "SELECT " + column + " FROM " + table + " WHERE Key='" + search + "';";
  return db_exec(cmd);
}

void DataAccess::db_write(string table, string key, string value) {
  string cmd = "UPDATE " + table + " SET Updated=datetime(), Value='" + value + "' WHERE Key='" + key + "';";
  db_exec(cmd);
}

void DataAccess::db_new(string table, string key, string value) {
  string cmd = "INSERT INTO " + table + " VALUES ('" + key + "', datetime(), '" + value + "');";
  db_exec(cmd);
}

