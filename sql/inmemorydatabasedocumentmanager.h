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
#ifndef INMEMORYDATABASEDOCUMENTMANAGER_H
#define INMEMORYDATABASEDOCUMENTMANAGER_H

#include "modelview/inmemoryshareduiitemdocumentmanager.h"
#include <QHash>
#include <QList>
#include <QSqlDatabase>

/** Simple generic implementation of SharedUiItemDocumentManager holding in
 * memory a repository of items by idQualifier and id, with database
 * persistence.
 *
 * Database persistence is inefficient for large number of items (all mapped
 * objects stay in memory, database schema is auto-created with no index at all,
 * etc.) but quite easy to set up since you only need to have a database
 * (event a Sqlite one-file database) and InMemoryDatabaseDocumentManager will
 * create one table per registred item type and will manage (insert and delete)
 * one row per item.
 *
 * All items must support SharedUiItem::ExternalDataRole role in their
 * uiData() and setUiData() implementation.
 *
 * To enable holding items, registerItemType() must be called for every
 * idQualifier, in such a way:
 *   dm->registerItemType(
 *         "foobar", static_cast<InMemorySharedUiItemDocumentManager::Setter>(
 *         &Foobar::setUiData),
 *         [](QString id) -> SharedUiItem { return Foobar(id); },
 *         0);
 *
 * A file database in user home directory can easily be set up that way:
 *   QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
 *   db.setDatabaseName(QDir::homePath()+"/.foo.db");
 *   db.open();
 *   InMemoryDatabaseDocumentManager *configureDm
 *        = new InMemoryDatabaseDocumentManager(db, this);
 */
class LIBQTSSUSHARED_EXPORT InMemoryDatabaseDocumentManager
    : public InMemorySharedUiItemDocumentManager {
  Q_OBJECT
  Q_DISABLE_COPY(InMemoryDatabaseDocumentManager)
  QSqlDatabase _db;
  QHash<QString,int> _idSections;

public:
  InMemoryDatabaseDocumentManager(QObject *parent = 0);
  InMemoryDatabaseDocumentManager(QSqlDatabase db, QObject *parent = 0);
  bool setDatabase(QSqlDatabase db, QString *errorString = 0);
  bool registerItemType(QString idQualifier, Setter setter, Creator creator,
                        int idSection, QString *errorString = 0);
  bool changeItem(SharedUiItem newItem, SharedUiItem oldItem,
                  QString idQualifier, QString *errorString = 0) override;
  // TODO add a way to notify user of database errors, such as a signal

private:
  bool createTableAndSelectData(
      QString idQualifier, Setter setter, Creator creator,
      int idSection, QString *errorString);
  static inline QString protectedColumnName(QString columnName);
  bool insertItem(SharedUiItem newItem, QString *errorString);
};

#endif // INMEMORYDATABASEDOCUMENTMANAGER_H
