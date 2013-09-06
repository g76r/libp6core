/* Copyright 2012-2013 Hallowyn and others.
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file to you under
 * the Apache License, Version 2.0 (the "License"); you may not use this
 * file except in compliance with the License. You may obtain a copy of the
 * License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
#include "pfsqlarrayfiller.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QVariant>

PfSqlArrayFiller::PfSqlArrayFiller(QSqlDatabase *db)
  : _db(db) {
}

PfArray PfSqlArrayFiller::buildArray(QString query, QString *errorString) {
  PfArray array;
  if (!_db) {
    if (errorString)
      *errorString = QObject::tr("cannot prepare or execute query on null db");
    return array;
  }
  QSqlQuery q(*_db);
  if (!q.prepare(query) || !q.exec()) {
    if (errorString)
      *errorString =
        QObject::tr("cannot prepare or execute query: %1, query was: %2")
          .arg(_db->lastError().text()).arg(query);
    return array;
  }
  return buildArray(&q, errorString);
}

PfArray PfSqlArrayFiller::buildArray(QSqlQuery *query, QString *errorString) {
  Q_UNUSED(errorString);
  PfArray array;
  if (query->first()) {
    QSqlRecord record = query->record();
    int columns = record.count();
    for (int i = 0; i < columns; ++i)
      array.appendHeader(record.fieldName(i));
    do {
      array.appendRow();
      for (int i = 0; i < columns; ++i)
        array.appendCell(query->value(i).toString());
    } while (query->next());
  }
  return array;
}
