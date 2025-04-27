/* Copyright 2015-2025 Hallowyn, Gregoire Barbier and others.
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
#include <QSqlDatabase>

/** Simple generic implementation of SharedUiItemDocumentManager holding in
 * memory a repository of items by qualifier and id, with database
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
 * qualifier, in such a way:
 *   dm->registerItemType(
 *         "foobar", static_cast<InMemorySharedUiItemDocumentManager::Setter>(
 *         &Foobar::setUiData),
 *         [](QString id) static -> SharedUiItem { return Foobar(id); },
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
  QHash<Utf8String,int> _id_sections; // qualifier -> section containing id
  Utf8StringList _ordered_qualifiers; // in order of registration

public:
  InMemoryDatabaseDocumentManager(QObject *parent = 0);
  InMemoryDatabaseDocumentManager(QSqlDatabase db, QObject *parent = 0);
  bool setDatabase(QSqlDatabase db, QString *errorString = 0);
  bool isDatabaseOpen() const { return _db.isOpen(); }
  /** As compared to base class, registerItemType also need section number to
   * be used to store item id (which is recommended to be 0). */
  bool registerItemType(
      const Utf8String &qualifier, Setter setter, Creator creator,
      int id_section, QString *errorString = 0);
  /** Convenience method. */
  inline bool registerItemType(
      const Utf8String &qualifier, Setter setter, SimplestCreator creator,
      int id_section, QString *errorString = 0) {
    return registerItemType(qualifier, setter, [creator](
                            SharedUiItemDocumentTransaction *,
                            Utf8String id, QString *) {
      return creator(id);
    }, id_section, errorString);
  }
  /** Convenience method. */
  template <class T>
  inline void registerItemType(
      const Utf8String &qualifier, MemberSetter<T> setter,
      Creator creator, int id_section, QString *errorString = 0) {
    registerItemType(qualifier, [setter](SharedUiItem *item, int section,
                     const QVariant &value, QString *errorString,
                     SharedUiItemDocumentTransaction *transaction, int role ){
      return (item->*static_cast<MemberSetter<SharedUiItem>>(setter))(
            section, value, errorString, transaction, role);
    }, creator, id_section, errorString);
  }
  /** Convenience method. */
  template <class T>
  inline void registerItemType(
      const Utf8String &qualifier, MemberSetter<T> setter,
      SimplestCreator creator, int id_section, QString *errorString = 0) {
    registerItemType(qualifier, [setter](SharedUiItem *item, int section,
                     const QVariant &value, QString *errorString,
                     SharedUiItemDocumentTransaction *transaction, int role ){
      return (item->*static_cast<MemberSetter<SharedUiItem>>(setter))(
            section, value, errorString, transaction, role);
    }, [creator](SharedUiItemDocumentTransaction *, Utf8String id, QString *) {
      return creator(id);
    }, id_section, errorString);
  }
  using InMemorySharedUiItemDocumentManager::prepareChangeItem;
  bool prepareChangeItem(
      SharedUiItemDocumentTransaction *transaction,
      const SharedUiItem &new_item, const SharedUiItem &old_item,
      const Utf8String &qualifier, QString *errorString) override;
  using InMemorySharedUiItemDocumentManager::commitChangeItem;
  void commitChangeItem(
      const SharedUiItem &new_item, const SharedUiItem &old_item,
      const Utf8String &qualifier) override;
  // TODO add a way to notify user of database errors, such as a signal

private:
  bool createTableAndSelectData(
      const Utf8String &qualifier, Setter setter, Creator creator,
      int id_section, QString *errorString);
  bool insertItemInDatabase(
      SharedUiItemDocumentTransaction *transaction,
      const SharedUiItem &new_item, QString *errorString);
  bool changeItemInDatabase(
      SharedUiItemDocumentTransaction *transaction,
      const SharedUiItem &new_item, const SharedUiItem &old_item,
      const Utf8String &qualifier, QString *errorString, bool dry_run);
  using InMemorySharedUiItemDocumentManager::registerItemType; // hide
};

#endif // INMEMORYDATABASEDOCUMENTMANAGER_H
