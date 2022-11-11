/* Copyright 2022 Hallowyn, Gregoire Barbier and others.
 * This file is part of libpumpkin, see <http://libpumpkin.g76r.eu/>.
 * Libpumpkin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libpumpkin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libpumpkin.  If not, see <http://www.gnu.org/licenses/>.
 */
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
  PfNode config, QString childname, ParamsProvider *context) {
  for (auto sqldb: config.childrenByName(childname)) {
    auto spec = sqldb.contentAsString();
    if (context)
      spec = context->evaluate(spec);
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

