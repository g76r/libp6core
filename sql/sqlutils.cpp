#include "sqlutils.h"
#include "pf/pfnode.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QRegularExpression>
#include "log/log.h"
#include "util/paramset.h"

static QRegularExpression _sqldbspec
  { "\\s*(?<name>\\w+)\\s+(?<driver>\\w+):"
    "(?:(?<username>\\w+)(?::(?<password>[^@/]*))?@)?"
    "(?<hostname>[^/:]*|\\[[^\\]]+\\])(?::(?<port>\\d+))?/"
    "(?<dbname>[^?]*)(?:\\?(?<options>[^?]*))?\\s*" };


void SqlUtils::configureSqlDatabasesFromChildren(
  PfNode config, QString childname) {
  for (auto sqldb: config.childrenByName(childname)) {
    auto spec = sqldb.contentAsString();
    auto m = _sqldbspec.match(spec);
    if (!m.hasMatch()) {
      Log::warning() << "cannot parse SQL database specification: " << spec;
      continue;
    }
    auto name = m.captured("name");
    QSqlDatabase db = QSqlDatabase::addDatabase(m.captured("driver"), name);
    db.setUserName(m.captured("username"));
    db.setPassword(m.captured("password"));
    db.setHostName(m.captured("hostname"));
    auto port = m.captured("port").toInt();
    if (port > 0 && port < 65536)
      db.setPort(port);
    db.setDatabaseName(m.captured("dbname"));
    db.setConnectOptions(m.captured("options"));
    if (!db.open()) {
      QSqlError error = db.lastError();
      Log::warning() << "failure to open SQL database " << name
                     << " error: " << error.nativeErrorCode() << " "
                     << error.driverText() << " " << error.databaseText();
      continue;
    }
    //Log::debug() << "configured SQL database " << name;
  }

}

void SqlUtils::setSqlParamsFromChildren(
  PfNode config, ParamSet *params, QString childname) {
  if (!params)
    return;
  for (auto sqlparams: config.childrenByName(childname))
    params->setValuesFromSqlDb(sqlparams.attribute("db"),
                               sqlparams.attribute("sql"),
                               sqlparams.childrenByName("bindings").value(0)
                                 .contentAsStringList());
}

