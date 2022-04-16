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
  sqlite3_exec = reinterpret_cast<x_handle>(dlsym(sqlite3_lib, "sqlite3_exec"));
  sqlite3_backup_init = reinterpret_cast<i_handle>(dlsym(sqlite3_lib, "sqlite3_backup_init"));
  sqlite3_backup_step = reinterpret_cast<b_handle>(dlsym(sqlite3_lib, "sqlite3_backup_step"));
  sqlite3_backup_finish = reinterpret_cast<n_handle>(dlsym(sqlite3_lib, "sqlite3_backup_finish"));
  sqlite3_errcode = reinterpret_cast<r_handle>(dlsym(sqlite3_lib, "sqlite3_errcode"));

  char * cachedir = getenv("XDG_DATA_HOME");
  fname = (char*)calloc(100, sizeof(char)*100);
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
  rc = sqlite3_open(":memory:", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  }
  db_copy(0);
  sql_write(db, "pragma journal_mode = WAL;");
  sql_write(db, "pragma synchronous = normal;");
  sql_write(db, "pragma temp_store = memory;");
  sql_write(db, "pragma mmap_size = 30000000000;");
  sql_write(db, "pragma locking_mode = exclusive;");
}

DataAccess::~DataAccess() {
  sql_write(db, "pragma optimize;");
  db_copy(1);
  sqlite3_close(db);
  dlclose(sqlite3_lib);
  free(fname);
}

void DataAccess::db_copy(int isSave) {
  sqlite3 *pFile;
  int rc = sqlite3_open(fname, &pFile);
  if (rc == SQLITE_OK) {
    string page_size = (isSave ? sql_read(db, "PRAGMA PAGE_SIZE;") : sql_read(pFile, "PRAGMA PAGE_SIZE;") );
    if (isSave) sql_write(pFile, "pragma page_size = " + page_size + ";");
    else sql_write(db, "pragma page_size = " + page_size + ";");
    sqlite3 *pFrom = (isSave ? db     : pFile);
    sqlite3 *pTo   = (isSave ? pFile  :    db);
    sqlite3_backup *pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
    if (pBackup) {
      (void)sqlite3_backup_step(pBackup, -1);
      (void)sqlite3_backup_finish(pBackup);
    }
    rc = sqlite3_errcode(pTo);
    if (rc != SQLITE_OK) {
      fprintf(stderr, "Copy error: %s\n", sqlite3_errmsg(db));
      exit(1);
    }
  }
  (void)sqlite3_close(pFile);
}

string DataAccess::sql_read(sqlite3 *pDb, string in) {
  const char * cmd = in.c_str();
#ifdef DEBUG
  cerr << "Sql call: " << cmd << endl;
#endif
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(pDb, cmd, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL compile error: %s\n", sqlite3_errmsg(pDb));
    exit(1);
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    fprintf(stderr, "Read error: %s\n", sqlite3_errmsg(pDb));
    sqlite3_finalize(stmt);
    exit(1);
  }
  const char *rawoutput = (char*)sqlite3_column_text(stmt, 0);
  if (!rawoutput) return "";
  string output = string(rawoutput);
#ifdef DEBUG
  cerr << "Database returned: " << output << endl;
#endif
  sqlite3_finalize(stmt);
  return output;
}

void DataAccess::sql_write(sqlite3 *pDb, string in) {
  const char * cmd = in.c_str();
#ifdef DEBUG
  cerr << "Sql call: " << cmd << endl;
#endif
  int rc = sqlite3_exec(pDb, cmd, NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Write error: %s\n", sqlite3_errmsg(pDb));
    exit(1);
  }
}

int DataAccess::datecheck(string table, string search) {
  string cmd = "SELECT Updated FROM " + table + " WHERE Key='" + search + "';";
  string datetime = sql_read(db, cmd);
  if (datetime.compare("") == 0) return 1;

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

void DataAccess::db_exec(string table) {
#ifdef DEBUG
  cerr << "Creating table: " << table << endl;
#endif
  sql_write(db, "CREATE TABLE IF NOT EXISTS " + table + "(Key TEXT NOT NULL, Updated DATETIME NOT NULL, Value TEXT NOT NULL, PRIMARY KEY(Key));");
}

string DataAccess::db_exec(string table, string key) {
  string cmd = "SELECT Value FROM " + table + " WHERE Key='" + key + "';";
  return sql_read(db, cmd);
}

void DataAccess::db_exec(string table, string key, string value) {
  string cmd = "REPLACE INTO " + table + " VALUES ('" + key + "', datetime(), '" + value + "');";
  sql_write(db, cmd);
}

void DataAccess::db_exec(string table, vector<string> key, vector<string> value) {
  sql_write(db, "BEGIN TRANSACTION");
  for (int i = 0; i < key.size(); i++) {
    string cmd = "REPLACE INTO " + table + " VALUES ('" + key[i] + "', datetime(), '" + value[i] + "');";
    sql_write(db, cmd);
  }
  sql_write(db, "END TRANSACTION");
}
