#include <QString>
#include <QtDebug>

#include "database.hpp"

#include "admin.hpp"
#include "worker.hpp"

void usage (const char* file) {
  qWarning ()
    << "Usage: " << file << " database_config [--worker | --admin]";
}

int main (int argc, char** argv) {
  if (argc != 3) {
    usage (argv[0]);
    return 1;
  }

  Database db;
  if (!db.ProcessSettingsFile (argv[1]) || !db.Connect ()) {
    return 1;
  }

 if (QString (argv[2]) == "--admin") {
    Admin admin (db);
    admin.Run ();
    return 0;
  }

  if (QString (argv[2]) == "--worker") {
    Worker worker (db);
    worker.Run ();
    return 0;
  }
}
