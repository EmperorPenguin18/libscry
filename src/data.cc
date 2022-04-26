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
  sqlite3_column_blob = reinterpret_cast<t_handle>(dlsym(sqlite3_lib, "sqlite3_column_blob"));
  sqlite3_column_bytes = reinterpret_cast<y_handle>(dlsym(sqlite3_lib, "sqlite3_column_bytes"));
  sqlite3_exec = reinterpret_cast<x_handle>(dlsym(sqlite3_lib, "sqlite3_exec"));
  sqlite3_backup_init = reinterpret_cast<i_handle>(dlsym(sqlite3_lib, "sqlite3_backup_init"));
  sqlite3_backup_step = reinterpret_cast<b_handle>(dlsym(sqlite3_lib, "sqlite3_backup_step"));
  sqlite3_backup_finish = reinterpret_cast<n_handle>(dlsym(sqlite3_lib, "sqlite3_backup_finish"));
  sqlite3_errcode = reinterpret_cast<r_handle>(dlsym(sqlite3_lib, "sqlite3_errcode"));
  sqlite3_bind_blob = reinterpret_cast<d_handle>(dlsym(sqlite3_lib, "sqlite3_bind_blob"));

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
  sql_exec(db, "pragma journal_mode = WAL;");
  sql_exec(db, "pragma synchronous = normal;");
  sql_exec(db, "pragma temp_store = memory;");
  sql_exec(db, "pragma mmap_size = 30000000000;");
  sql_exec(db, "pragma locking_mode = exclusive;");
}

DataAccess::~DataAccess() {
  sql_exec(db, "pragma optimize;");
  db_copy(1);
  sqlite3_close(db);
  dlclose(sqlite3_lib);
  free(fname);
  while (!bytes.empty()) {
    free(bytes.back());
    bytes.pop_back();
  }
}

void DataAccess::db_copy(int isSave) {
  sqlite3 *pFile;
  int rc = sqlite3_open(fname, &pFile);
  if (rc == SQLITE_OK) {
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

byte* DataAccess::sql_read(sqlite3 *pDb, const char* cmd, size_t* size) {
#ifdef DEBUG
  fprintf(stderr, "Sql call: %s\n", cmd);
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
  byte *rawoutput = (byte*)sqlite3_column_blob(stmt, 0);
  if (!rawoutput) {
    sqlite3_finalize(stmt);
    return nullptr;
  }
  *size = sqlite3_column_bytes(stmt, 0);
#ifdef DEBUG
  fprintf(stderr, "Database returned (250 chars): ");
  if (*size < 250) for (size_t i = 0; i < *size; i++) fprintf(stderr, "%c", (char)rawoutput[i]);
  else for (size_t i = 0; i < 250; i++) fprintf(stderr, "%c", (char)rawoutput[i]);
  fprintf(stderr, "\n");
#endif
  byte* output = (byte*)malloc(sizeof(byte)*(*size) + 1);
  if (!output) {
    fprintf(stderr, "not enough memory: malloc returned null");
    exit(1);
  }
  memcpy(output, rawoutput, *size);
  bytes.push_back(output);
  sqlite3_finalize(stmt);
  return output;
}

void DataAccess::sql_write(sqlite3 *pDb, const char* cmd, byte* data, const size_t& size) {
#ifdef DEBUG
  fprintf(stderr, "Sql call: %s\n", cmd);
#endif
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(pDb, cmd, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL compile error: %s\n", sqlite3_errmsg(pDb));
    exit(1);
  }
#ifdef DEBUG
  fprintf(stderr, "Data input (250 chars): ");
  if (size < 250) for (size_t i = 0; i < size; i++) fprintf(stderr, "%c", (char)data[i]);
  else for (size_t i = 0; i < 250; i++) fprintf(stderr, "%c", (char)data[i]);
  fprintf(stderr, "\n");
#endif
  rc = sqlite3_bind_blob(stmt, 1, data, size, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    fprintf(stderr, "Write error: %s\n", sqlite3_errmsg(pDb));
    sqlite3_finalize(stmt);
    exit(1);
  }
#ifdef DEBUG
  if (rc == SQLITE_ROW)
    fprintf(stderr, "Write returned: %s\n", (char*)sqlite3_column_blob(stmt, 0));
#endif
  sqlite3_finalize(stmt);
}

void DataAccess::sql_exec(sqlite3 *pDb, const char* cmd) {
#ifdef DEBUG
  fprintf(stderr, "Sql call: %s\n", cmd);
#endif
  int rc = sqlite3_exec(pDb, cmd, NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL exec error: %s\n", sqlite3_errmsg(pDb));
    exit(1);
  }
}

int DataAccess::datecheck(const char* table, const char* search) {
  char cmd[200] = "";
  strcat(cmd, "SELECT Updated FROM ");
  strcat(cmd, table);
  strcat(cmd, " WHERE Key='");
  strcat(cmd, search);
  strcat(cmd, "';");
  size_t size = 0;
  char* rawdatetime = (char*)sql_read(db, cmd, &size);
  if (!rawdatetime) return 1;
  rawdatetime[size] = '\0';
  string datetime = string(rawdatetime);

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

void DataAccess::db_exec(const char* table) {
  string cmd = "CREATE TABLE IF NOT EXISTS " + string(table) + "(Key TEXT NOT NULL, Updated DATETIME NOT NULL, Value BLOB NOT NULL, PRIMARY KEY(Key));";
  sql_exec(db, cmd.c_str());
}

byte* DataAccess::db_exec(const char* table, const char* key, size_t* size) {
  char cmd[200] = "";
  strcat(cmd, "SELECT Value FROM ");
  strcat(cmd, table);
  strcat(cmd, " WHERE Key='");
  strcat(cmd, key);
  strcat(cmd, "';");
  return sql_read(db, cmd, size);
}

char* DataAccess::db_exec(const char* table, const char* key) {
  size_t size = 0;
  char* output = (char*)db_exec(table, key, &size);
  output[size] = '\0';
  return output;
}

void DataAccess::db_exec(const char* table, const char* key, byte* value, const size_t& size) {
  char cmd[200] = "";
  strcat(cmd, "REPLACE INTO ");
  strcat(cmd, table);
  strcat(cmd, " VALUES ('");
  strcat(cmd, key);
  strcat(cmd, "', datetime(), ?);");
  sql_write(db, cmd, value, size);
}

void DataAccess::db_exec(const char* table, vector<char*> keys, vector<char*> values) {
  sql_exec(db, "BEGIN TRANSACTION;");
  for (int i = 0; i < keys.size(); i++) {
    char cmd[200] = "";
    strcat(cmd, "REPLACE INTO ");
    strcat(cmd, table);
    strcat(cmd, " VALUES ('");
    strcat(cmd, keys[i]);
    strcat(cmd, "', datetime(), ?);");
    sql_write(db, cmd, (byte*)values[i], strlen(values[i]));
    free(keys[i]);
    delete[] values[i];
  }
  sql_exec(db, "END TRANSACTION;");
}
