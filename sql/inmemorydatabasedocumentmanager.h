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
class LIBP6CORESHARED_EXPORT InMemoryDatabaseDocumentManager
    : public InMemorySharedUiItemDocumentManager {
  Q_OBJECT
  Q_DISABLE_COPY(InMemoryDatabaseDocumentManager)
  QSqlDatabase _db;
  QHash<QString,int> _idSections;
  Utf8StringList _orderedIdQualifiers; // in order of registration

public:
  InMemoryDatabaseDocumentManager(QObject *parent = 0);
  InMemoryDatabaseDocumentManager(QSqlDatabase db, QObject *parent = 0);
  bool setDatabase(QSqlDatabase db, QString *errorString = 0);
  bool isDatabaseOpen() const { return _db.isOpen(); }
  /** As compared to base class, registerItemType also need section number to
   * be used to store item id (which is recommended to be 0). */
  bool registerItemType(Utf8String idQualifier, Setter setter, Creator creator,
                        int idSection, QString *errorString = 0);
  /** Convenience method. */
  bool registerItemType(Utf8String idQualifier, Setter setter,
                        SimplestCreator creator,
                        int idSection, QString *errorString = 0) {
    return registerItemType(idQualifier, setter, [creator](
                            SharedUiItemDocumentTransaction *,
                            Utf8String id, QString *) {
      return creator(id);
    }, idSection, errorString);
  }
  /** Convenience method. */
  template <class T>
  void registerItemType(Utf8String idQualifier, MemberSetter<T> setter,
                        Creator creator, int idSection,
                        QString *errorString = 0) {
    registerItemType(idQualifier, [setter](SharedUiItem *item, int section,
                     const QVariant &value, QString *errorString,
                     SharedUiItemDocumentTransaction *transaction, int role ){
      return (item->*static_cast<MemberSetter<SharedUiItem>>(setter))(
            section, value, errorString, transaction, role);
    }, creator, idSection, errorString);
  }
  /** Convenience method. */
  template <class T>
  void registerItemType(Utf8String idQualifier, MemberSetter<T> setter,
                        SimplestCreator creator, int idSection,
                        QString *errorString = 0) {
    registerItemType(idQualifier, [setter](SharedUiItem *item, int section,
                     const QVariant &value, QString *errorString,
                     SharedUiItemDocumentTransaction *transaction, int role ){
      return (item->*static_cast<MemberSetter<SharedUiItem>>(setter))(
            section, value, errorString, transaction, role);
    }, [creator](SharedUiItemDocumentTransaction *, Utf8String id, QString *) {
      return creator(id);
    }, idSection, errorString);
  }
  bool prepareChangeItem(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
      SharedUiItem oldItem, Utf8String idQualifier,
      QString *errorString) override;
  void commitChangeItem(SharedUiItem newItem, SharedUiItem oldItem,
                        Utf8String idQualifier) override;
  // TODO add a way to notify user of database errors, such as a signal

private:
  bool createTableAndSelectData(
      Utf8String idQualifier, Setter setter, Creator creator,
      int idSection, QString *errorString);
  static inline Utf8String protectedColumnName(Utf8String columnName);
  bool insertItemInDatabase(SharedUiItemDocumentTransaction *transaction,
                            SharedUiItem newItem, QString *errorString);
  bool changeItemInDatabase(SharedUiItemDocumentTransaction *transaction,
      SharedUiItem newItem, SharedUiItem oldItem, Utf8String idQualifier,
      QString *errorString, bool dryRun);
  using InMemorySharedUiItemDocumentManager::registerItemType; // hide
};

#endif // INMEMORYDATABASEDOCUMENTMANAGER_H
