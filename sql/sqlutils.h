#ifndef SQLUTILS_H
#define SQLUTILS_H

#include "libp6core_global.h"

class PfNode;
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
   */
  static void configureSqlDatabasesFromChildren(
    PfNode config, QString childname);
  /** calls ParamSet::setValuesFromSqlDb for each child with a given name,
   *  using its children "db", "sql" and "bindings" as parameters.
   *  e.g.
   *  (parent
   *    (sqlparams (db foodb)(sql select 2+2, 2*2)(bindings four eight))
   *    (sqlparams (db orders)(sql name from customers)(bindings customers))
   *  )
   */
  static void setSqlParamsFromChildren(
    PfNode config, ParamSet *params, QString childname);
};

#endif // SQLUTILS_H
