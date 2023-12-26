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
#ifndef SQLUTILS_H
#define SQLUTILS_H

#include "util/utf8string.h"

class PfNode;
class ParamsProvider;
class ParamSet;

class LIBP6CORESHARED_EXPORT SqlUtils {
  SqlUtils() = delete;

public:
  /** Configure QSqlDatabases from PfNode children given a child name.
   * e.g. with configureSqlDatabasesFromChildren("sqldb")
   * (parent
   *   (sqldb foodb "QPSQL:user:s3cr3t@host:5439/foo?connect_timeout=2")
   *   (sqldb localdb "QPSQL:/bar")
   *  )
   *  ":password", "hostname", ":port" and "?options" can be omitted
   *  "user@" can be omitted (if password is omitted)
   *  database name can be omitted
   *  leading "name driver:" is mandatory
   *  "/" is mandatory
   *  password must not contain / or @ (deal with it)
   *  hostname must not contain /
   *  dbname and options must not contain ?
   *
   *  @param context if not null, connection string is %-evaluated within it
   */
  static void configureSqlDatabasesFromChildren(
      const PfNode &config, const Utf8String &childname,
      const ParamsProvider *context = 0);
  static inline void configureSqlDatabasesFromChildren(
      const PfNode &config, const Utf8String &childname,
      const ParamsProvider &context) {
    configureSqlDatabasesFromChildren(config, childname, &context);
  }
  /** calls ParamSet::setValuesFromSqlDb for each child with a given name,
   *  using its children "db", "sql" and "bindings" as parameters.
   *  e.g.
   *  (parent
   *    (sqlparams (db foodb)(sql select 2+2, 2*2)(bindings four eight))
   *    (sqlparams (db orders)(sql name from customers)(bindings customers))
   *  )
   */
  static void setSqlParamsFromChildren(
      const PfNode &config, ParamSet *params, const Utf8String &childname);
};

#endif // SQLUTILS_H
