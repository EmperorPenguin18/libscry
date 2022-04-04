//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "data.h"

using namespace std;
using namespace std::chrono;

DataAccess::DataAccess(const char *name) {
  sqlite3_lib = dlopen("libsqlite3.so", RTLD_LAZY | RTLD_DEEPBIND);
  if (!sqlite3_lib) {
    fprintf(stderr, "%s\n", dlerror());
    exit(EXIT_FAILURE);
  }
  sqlite3_open = reinterpret_cast<o_handle>(dlsym(sqlite3_lib, "sqlite3_open"));
  sqlite3_errmsg = reinterpret_cast<e_handle>(dlsym(sqlite3_lib, "sqlite3_errmsg"));
  sqlite3_close = reinterpret_cast<c_handle>(dlsym(sqlite3_lib, "sqlite3_close"));
  sqlite3_prepare_v2 = reinterpret_cast<p_handle>(dlsym(sqlite3_lib, "sqlite3_prepare_v2"));
  sqlite3_step = reinterpret_cast<s_handle>(dlsym(sqlite3_lib, "sqlite3_step"));
  sqlite3_finalize = reinterpret_cast<f_handle>(dlsym(sqlite3_lib, "sqlite3_finalize"));
  sqlite3_column_text = reinterpret_cast<t_handle>(dlsym(sqlite3_lib, "sqlite3_column_text"));

  char * cachedir = getenv("XDG_DATA_HOME");
  char fname[100] = "";
  int rc;
  if (cachedir != NULL) {
    strcat(fname, cachedir);
    strcat(fname, "/");
  } else {
    strcat(fname, getenv("HOME"));
    strcat(fname, "/.local/share/");
  }
  strcat(fname, name);
  strcat(fname, ".db");
#ifdef DEBUG
  fprintf(stderr, "Database filepath: %s\n", fname);
#endif
  rc = sqlite3_open(fname, &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  }
}

DataAccess::~DataAccess() {
  sqlite3_close(db);
  dlclose(sqlite3_lib);
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

int DataAccess::datecheck(string table, string search) {
  string cmd = "SELECT Updated FROM " + table + " WHERE Key='" + search + "';";
  string datetime = db_exec(cmd);

  const time_point<system_clock> now{system_clock::now()};
  const year_month_day ymd{floor<days>(now)};
  const year_month_day old(
		  static_cast<year>(stoi(datetime.substr(0,4))),
		  static_cast<month>(stoi(datetime.substr(5,2))),
		  static_cast<day>(stoi(datetime.substr(8,2))));
  
  int output;
  if (static_cast<int>(ymd.year()) > static_cast<int>(old.year())) {
    output = 1;
  } else {
    if (static_cast<unsigned>(ymd.month()) > static_cast<unsigned>(old.month())) {
      output = 1;
    } else {
      if (static_cast<unsigned>(ymd.day()) > static_cast<unsigned>(old.day()) + 7) {
	output = 1;
      } else if (static_cast<unsigned>(ymd.day()) == static_cast<unsigned>(old.day()) + 7) {
	output = 0;
      } else {
	output = -1;
      }
    }
  }

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

string DataAccess::db_read(string table, string search) {
  string cmd = "SELECT Value FROM " + table + " WHERE Key='" + search + "';";
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

