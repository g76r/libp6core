/* Copyright 2015-2023 Hallowyn, Gregoire Barbier and others.
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
#include "inmemorydatabasedocumentmanager.h"
#include <QSqlQuery>
#include <QSqlError>

static QRegularExpression _unallowedColumnCharsSequence {
  "(^[^a-zA-Z_]+)|([^a-zA-Z0-9_]+)" };

InMemoryDatabaseDocumentManager::InMemoryDatabaseDocumentManager(QObject *parent)
  : InMemorySharedUiItemDocumentManager(parent) {
}

InMemoryDatabaseDocumentManager::InMemoryDatabaseDocumentManager(
    QSqlDatabase db, QObject *parent)
  : InMemorySharedUiItemDocumentManager(parent), _db(db) {
}

bool InMemoryDatabaseDocumentManager::registerItemType(
    const Utf8String &qualifier, Setter setter, Creator creator, int id_section,
    QString *errorString) {
  QString reason;
  if (!errorString)
    errorString = &reason;
  InMemorySharedUiItemDocumentManager::registerItemType(
        qualifier, setter, creator);
  _ordered_qualifiers.append(qualifier);
  _id_sections.insert(qualifier, id_section);
  if (!createTableAndSelectData(qualifier, setter, creator, id_section,
                                errorString)) {
    *errorString = reason;
    qWarning() << "InMemoryDatabaseDocumentManager" << *errorString;
    return false;
  }
  return true;
}

bool InMemoryDatabaseDocumentManager::prepareChangeItem(
    SharedUiItemDocumentTransaction *transaction, const SharedUiItem &new_item,
    const SharedUiItem &old_item, const Utf8String &qualifier,
    QString *errorString) {
  Q_ASSERT(errorString != 0);
  if (!changeItemInDatabase(transaction, new_item, old_item, qualifier,
                            errorString, true)) {
    qDebug() << "InMemoryDatabaseDocumentManager::prepareChangeItem: "
                "test transaction failed:" << *errorString;
    return false;
  }
  storeItemChange(transaction, new_item, old_item, qualifier);
  return true;
}

void InMemoryDatabaseDocumentManager::commitChangeItem(
    const SharedUiItem &new_item, const SharedUiItem &old_item,
    const Utf8String &qualifier) {
  QString errorString;
  SharedUiItemDocumentTransaction transaction(this);
  if (!changeItemInDatabase(&transaction, new_item, old_item, qualifier,
                            &errorString, false)) {
    // this should only occur on severe technical error (filesystem full,
    // network connection to the database lost, etc.)
    qWarning() << "InMemoryDatabaseDocumentManager cannot write to database "
                  "prepared change:" << new_item << old_item << ":"
               << errorString;
  } else {
    //qDebug() << "InMemoryDatabaseDocumentManager::commitChangeItem"
    //         << newItem << oldItem;
    InMemorySharedUiItemDocumentManager::commitChangeItem(
          new_item, old_item, qualifier);
  }
}

bool InMemoryDatabaseDocumentManager::changeItemInDatabase(
    SharedUiItemDocumentTransaction *transaction, const SharedUiItem &new_item,
    const SharedUiItem &old_item, const Utf8String &qualifier,
    QString *errorString, bool dry_run) {
  Q_ASSERT(errorString != 0);
  Q_ASSERT(!new_item.isNull() || !old_item.isNull());
  if (!_db.transaction()) {
    *errorString = "database error: cannot start transaction "
        +_db.lastError().text();
    return false;
  }
  if (!old_item.isNull()) {
    QSqlQuery query(_db);
    query.prepare("delete from "+qualifier+" where "
                  +protectedColumnName(old_item.uiSectionName(
                                         _id_sections.value(qualifier)))
                  +" = ?");
    query.bindValue(0, old_item.id());
    if (!query.exec()) {
      *errorString = tr("database error: cannot delete from table %1 %2 %3 %4")
                     .arg(qualifier).arg(old_item.id())
                     .arg(query.lastError().text()).arg(query.executedQuery());
      goto failed;
    }
  }
  if (!new_item.isNull()
      && !insertItemInDatabase(transaction, new_item, errorString)) {
    if (!_db.rollback()) {
      qDebug() << "InMemoryDatabaseDocumentManager database error: cannot "
                  "rollback transaction" << _db.lastError().text();
    }
    goto failed;
  }
  if (dry_run) {
    if (!_db.rollback()) {
      qDebug() << "InMemoryDatabaseDocumentManager database error: cannot "
                  "rollback transaction" << _db.lastError().text();
      return false;
    }
  } else {
    if (!_db.commit()) {
      *errorString = "database error: cannot commit transaction "
          +_db.lastError().text();
      goto failed;
    }
  }
  return true;
failed:;
  _db.rollback();
  qWarning() << "InMemoryDatabaseDocumentManager" << *errorString;
  return false;
}

bool InMemoryDatabaseDocumentManager::insertItemInDatabase(
    SharedUiItemDocumentTransaction *transaction, const SharedUiItem &new_item,
    QString *errorString) {
  Q_UNUSED(transaction)
  Q_ASSERT(errorString != 0);
  Creator creator = _creators.value(new_item.qualifier());
  if (new_item.isNull() || !creator) {
    *errorString = "cannot insert into database item: "+new_item.qualifiedId()
        +" creator: "+(creator?"true":"false");
    return false;
  }
  QString qualifier = new_item.qualifier();
  QStringList columnNames, placeholders;
  for (int i = 0; i < new_item.uiSectionCount(); ++i) {
    columnNames << protectedColumnName(new_item.uiSectionName(i));
    placeholders << QStringLiteral("?");
  }
  QSqlQuery query(_db);
  query.prepare("insert into "+qualifier+" ("+columnNames.join(',')
                +") values ("+placeholders.join(',')+")");
  for (int i = 0; i < new_item.uiSectionCount(); ++i)
    query.bindValue(i, new_item.uiData(i, SharedUiItem::ExternalDataRole));
  if (!query.exec()) {
    *errorString = tr("database error: cannot insert into table %1 %2: %3")
                   .arg(qualifier).arg(new_item.id())
                   .arg(query.lastError().text());
    qDebug() << "InMemoryDatabaseDocumentManager" << *errorString;
    return false;
  }
  return true;
}

bool InMemoryDatabaseDocumentManager::setDatabase(
    QSqlDatabase db, QString *errorString) {
  _repository.clear();
  _db = db;
  emit dataReset();
  bool successful = true;
  QString reason;
  for (auto qualifier: _ordered_qualifiers) {
    QString oneReason;
    if (!createTableAndSelectData(
          qualifier, _setters.value(qualifier),
          _creators.value(qualifier), _id_sections.value(qualifier),
          &oneReason)) {
      successful = false;
      if (!reason.isEmpty())
        reason = reason+"\n"+oneReason;
      else
        reason = oneReason;
    }
  }
  if (!successful && errorString)
    *errorString = reason;
  return successful;
}

bool InMemoryDatabaseDocumentManager::createTableAndSelectData(
    const Utf8String &qualifier, Setter setter, Creator creator, int id_section,
    QString *errorString) {
  Q_ASSERT(errorString != 0);
  Q_UNUSED(id_section)
  Q_ASSERT_X((creator && setter),
             "InMemoryDatabaseDocumentManager::createTableAndSelectData",
             "invalid parameters");
  Utf8StringList columnNames;
  SharedUiItemDocumentTransaction transaction(this);
  SharedUiItem item = creator(&transaction, "dummy"_u8, errorString);
  if (!_db.isOpen())
    return true; // do nothing without a valid opened database
  if (item.isNull()) {
    qWarning() << "InMemoryDatabaseDocumentManager cannot create empty "
                  "item of type" << qualifier << ":" << *errorString;
    return false;
  }
  for (int i = 0; i < item.uiSectionCount(); ++i) {
    columnNames << protectedColumnName(item.uiSectionName(i));
  }
  QSqlQuery query(_db);
  query.exec("select count(*) from "+qualifier);
  if (query.lastError().type() != QSqlError::NoError) {
    QString q = "create table "+qualifier+" ( ";
    for (int i = 0; i < columnNames.size(); ++ i) {
      const QString &columnName = columnNames[i];
      if (i)
        q += ", ";
      q = q+columnName+" text"; // LATER use a more portable text data type
    }
    q += " )";
    query.exec(q);
    if (query.lastError().type() != QSqlError::NoError) {
      *errorString = tr("database error: cannot create table: %1: %2")
                     .arg(qualifier).arg(query.lastError().text());
      return false;
    }
    // TODO create unique index if not exists mytable_pk on mytable(idcolumn)
  }
  // TODO alter table, if needed, @see QSqlDatabase::record()
  /*
sqlite> alter table connection rename to foo;
sqlite> create table connection as select Id,Url,Login,Password,'' as Proxy_Id from foo;
sqlite> .h on
sqlite> select * from connection;
Id|URL|Login|Password|Proxy_Id
connection4||||
sqlite> drop table foo;
   */
  query.exec("select "+columnNames.join(',')+" from "+qualifier);
  if (query.lastError().type() != QSqlError::NoError) {
    *errorString = tr("database error: cannot select from table: %1: %2")
                   .arg(qualifier).arg(query.lastError().text());
    return false;
  }
  //qDebug() << "***** selected:" << query.executedQuery();
  while (query.next()) {
    item = creator(&transaction, "dummy"_u8, errorString);
    if (item.isNull()) {
      qWarning() << "InMemoryDatabaseDocumentManager cannot create empty "
                    "item of type" << qualifier << ":" << *errorString;
      break;
    }
    for (int i = 0; i < item.uiSectionCount(); ++i) {
      QString errorString;
      bool ok = setter(&item, i, query.value(i), &errorString, &transaction,
                       SharedUiItem::ExternalDataRole);
      if (!ok) {
        // TODO do not log this
        qDebug() << "InMemoryDatabaseDocumentManager cannot set value for item"
                 << item.qualifiedId() << errorString;
      }
    }
    //qDebug() << "  have item:" << item.qualifiedId();
    InMemorySharedUiItemDocumentManager::commitChangeItem(
          item, SharedUiItem(), qualifier);
  }
  return true;
}

QString InMemoryDatabaseDocumentManager::protectedColumnName(
    QString column_name) {
  return column_name.replace(_unallowedColumnCharsSequence, u"_"_s);
}
