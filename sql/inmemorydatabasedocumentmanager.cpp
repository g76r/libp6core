/* Copyright 2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "inmemorydatabasedocumentmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <QtDebug>

InMemoryDatabaseDocumentManager::InMemoryDatabaseDocumentManager(QObject *parent)
  : InMemorySharedUiItemDocumentManager(parent) {
}

InMemoryDatabaseDocumentManager::InMemoryDatabaseDocumentManager(
    QSqlDatabase db, QObject *parent)
  : InMemorySharedUiItemDocumentManager(parent), _db(db) {
}

InMemorySharedUiItemDocumentManager &
InMemoryDatabaseDocumentManager::registerItemType(
    QString idQualifier, Setter setter, Creator creator, int idSection) {
  InMemorySharedUiItemDocumentManager::registerItemType(
        idQualifier, setter, creator);
  _idSections.insert(idQualifier, idSection);
  createTableAndSelectData(idQualifier, setter, creator, idSection);
  return *this;
}

SharedUiItem InMemoryDatabaseDocumentManager::createNewItem(QString idQualifier) {
  SharedUiItem newItem =
      InMemorySharedUiItemDocumentManager::createNewItem(idQualifier);
  if (!insertItem(newItem)) {
    InMemorySharedUiItemDocumentManager::changeItem(SharedUiItem(), newItem);
    return SharedUiItem();
  }
  return newItem;
}

bool InMemoryDatabaseDocumentManager::changeItem(
    SharedUiItem newItem, SharedUiItem oldItem) {
  if (!_db.transaction()) {
    qDebug() << "InMemoryDatabaseDocumentManager cannot start transaction"
             << _db.lastError().text();
    goto failed;
  }
  // resolve oldItem from repository, to handle case where oldItem is just a
  // GenericSharedUiItem placeholder with ids but no data and to handle case
  // where oldItem does not actualy exist
  oldItem = itemById(oldItem.idQualifier(), oldItem.id());
  if (!oldItem.isNull()) {
    QString idQualifier = oldItem.idQualifier();
    QSqlQuery query(_db);
    query.prepare("delete from "+idQualifier+" where "
                  +protectedColumnName(oldItem.uiHeaderString(
                                         _idSections.value(idQualifier)))
                  +" = ?");
    query.bindValue(0, oldItem.id());
    if (!query.exec()) {
      qDebug() << "InMemoryDatabaseDocumentManager cannot delete from table"
               << idQualifier << oldItem.id() << query.lastError().text()
               << query.executedQuery();
      goto failed;
    }
  } else if (newItem.isNull()) {
    goto failed; // called with null,null : should never happen
  }
  if (!newItem.isNull() && !insertItem(newItem)) {
    if (!_db.rollback()) {
      qDebug() << "InMemoryDatabaseDocumentManager cannot rollback transaction"
               << _db.lastError().text();
    }
    goto failed;
  }
  if (!_db.commit()) {
    qDebug() << "InMemoryDatabaseDocumentManager cannot commit transaction"
             << _db.lastError().text();
    goto failed;
  }
  InMemorySharedUiItemDocumentManager::changeItem(newItem, oldItem);
  return true;
failed:;
  _db.rollback();
  return false;
}

bool InMemoryDatabaseDocumentManager::insertItem(SharedUiItem newItem) {
  Creator creator = _creators.value(newItem.idQualifier());
  if (newItem.isNull() || !creator)
    return false;
  QString idQualifier = newItem.idQualifier();
  QStringList columnNames, protectedColumnNames, placeholders;
  SharedUiItem item = creator(QStringLiteral("dummy"));
  for (int i = 0; i < item.uiSectionCount(); ++i) {
    QString headerName = item.uiHeaderString(i);
    columnNames << headerName;
    protectedColumnNames << protectedColumnName(headerName);
    placeholders << QStringLiteral("?");
  }
  QSqlQuery query(_db);
  query.prepare("insert into "+idQualifier+" ("+protectedColumnNames.join(',')
                +") values ("+placeholders.join(',')+") ");
  for (int i = 0; i < newItem.uiSectionCount(); ++i)
    query.bindValue(i, newItem.uiData(i, SharedUiItem::ExternalDataRole));
  if (!query.exec()) {
    qDebug() << "InMemoryDatabaseDocumentManager cannot insert into table"
             << idQualifier << newItem.id() << query.lastError().text();
    return false;
  }
  return true;
}

InMemoryDatabaseDocumentManager &InMemoryDatabaseDocumentManager::setDatabase(
    QSqlDatabase db) {
  _repository.clear();
  _db = db;
  foreach (const QString &idQualifier, _idSections.keys())
    createTableAndSelectData(idQualifier, _setters.value(idQualifier),
                             _creators.value(idQualifier),
                             _idSections.value(idQualifier));
  return *this;
}

void InMemoryDatabaseDocumentManager::createTableAndSelectData(
    QString idQualifier, Setter setter, Creator creator, int idSection) {
  Q_UNUSED(idSection)
  if (!creator || !setter) // should never happen
    return;
  QStringList columnNames, protectedColumnNames;
  SharedUiItem item = creator(QStringLiteral("dummy"));
  for (int i = 0; i < item.uiSectionCount(); ++i) {
    QString headerName = item.uiHeaderString(i);
    columnNames << headerName;
    protectedColumnNames << protectedColumnName(headerName);
  }
  QSqlQuery query(_db);
  query.exec("select count(*) from "+idQualifier);
  if (query.lastError().type() != QSqlError::NoError) {
    QString q = "create table "+idQualifier+" ( ";
    for (int i = 0; i < protectedColumnNames.size(); ++ i) {
      const QString &columnName = protectedColumnNames[i];
      if (i)
        q += ", ";
      q = q+columnName+" text"; // LATER use a more portable text data type
    }
    q += " )";
    query.exec(q);
    if (query.lastError().type() != QSqlError::NoError) {
      qWarning() << "InMemoryDatabaseDocumentManager cannot create table"
                 << idQualifier << query.lastError().text();
      return;
    }
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
  query.exec("select "+protectedColumnNames.join(',')+" from "+idQualifier);
  if (query.lastError().type() != QSqlError::NoError) {
    qWarning() << "InMemoryDatabaseDocumentManager cannot select from table"
               << idQualifier << query.lastError().text();
    return;
  }
  //qDebug() << "***** selected:" << query.executedQuery();
  while (query.next()) {
    item = creator(QStringLiteral("dummy"));
    for (int i = 0; i < item.uiSectionCount(); ++i) {
      QString errorString;
      bool ok = (item.*setter)(i, query.value(i), &errorString,
                               SharedUiItem::ExternalDataRole, this);
      if (!ok) {
        // TODO do not log this
        qDebug() << "InMemoryDatabaseDocumentManager cannot set value for item"
                 << item.qualifiedId() << errorString;
      }
    }
    //qDebug() << "  have item:" << item.qualifiedId();
    InMemorySharedUiItemDocumentManager::changeItem(item, SharedUiItem());
  }
}

static QRegularExpression unallowedColumnCharsSequence {
  "(^[^a-zA-Z_]+)|([^a-zA-Z0-9_]+)" };

QString InMemoryDatabaseDocumentManager::protectedColumnName(QString columnName) {
  return columnName.replace(unallowedColumnCharsSequence,
                            QStringLiteral("_"));
}
