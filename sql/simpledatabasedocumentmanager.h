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
#ifndef SIMPLEDATABASEDOCUMENTMANAGER_H
#define SIMPLEDATABASEDOCUMENTMANAGER_H

#include "modelview/simpleshareduiitemdocumentmanager.h"
#include <QHash>
#include <QList>
#include <QSqlDatabase>

/**
 * inefficient for large databases (all mapped objects in memory, auto create un-indexed schemas...)
 */
/** Simple generic implementation of SharedUiItemDocumentManager holding in
 * memory a repository of items by idQualifier and id, with database
 * persistence.
 *
 * Database persistence is inefficient for large number of items (all mapped
 * objects stay in memory, database schema is auto-created with no index at all,
 * etc.) but quite easy to set up since you only need to have a database
 * (event a Sqlite one-file database) and SimpleDatabaseDocumentManager will
 * create one table per registred item type and will manage (insert and delete)
 * one row per item.
 *
 * To enable holding items, registerItemType() must be called for every
 * idQualifier, in such a way:
 *   dm->registerItemType(
 *         "foobar", static_cast<SimpleSharedUiItemDocumentManager::Setter>(
 *         &Foobar::setUiData),
 *         [](QString id) -> SharedUiItem { return Foobar(id); },
 *         0);
 *
 * A file database in user home directory can easily be set up that way:
 *   QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
 *   db.setDatabaseName(QDir::homePath()+"/.foo.db");
 *   db.open();
 *   SimpleDatabaseDocumentManager *configureDm
 *        = new SimpleDatabaseDocumentManager(db, this);
 */
class LIBQTSSUSHARED_EXPORT SimpleDatabaseDocumentManager
    : public SimpleSharedUiItemDocumentManager {
  Q_OBJECT
  Q_DISABLE_COPY(SimpleDatabaseDocumentManager)
  QSqlDatabase _db;
  QHash<QString,int> _idSections;

public:
  SimpleDatabaseDocumentManager(QObject *parent = 0);
  SimpleDatabaseDocumentManager(QSqlDatabase db, QObject *parent = 0);
  /** Does not take ownership of the QSqlDatabse. */
  SimpleDatabaseDocumentManager &setDatabase(QSqlDatabase db);
  SimpleSharedUiItemDocumentManager &registerItemType(
      QString idQualifier, Setter setter, Creator creator,
      int idSection);
  SharedUiItem createNewItem(QString idQualifier) override;
  bool changeItem(SharedUiItem newItem, SharedUiItem oldItem) override;
  // TODO add a way to notify user of database errors, such as a signal

private:
  void createTableAndSelectData(
      QString idQualifier, Setter setter, Creator creator,
      int idSection);
  static inline QString protectedColumnName(QString columnName);
  bool insertItem(SharedUiItem newItem);
};

#endif // SIMPLEDATABASEDOCUMENTMANAGER_H
