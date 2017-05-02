/* Copyright 2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef SQLOBJECTSSTORE_H
#define SQLOBJECTSSTORE_H

#include "objectsstore.h"
#include <QSqlDatabase>
#include <QMetaMethod>

/** RDBMS implementation for ObjectsStore.
 * Currently only SQLite is supported for real.
 * @see ObjectsStore
 */
class LIBPUMPKINSHARED_EXPORT SqlObjectsStore : public ObjectsStore {
  Q_OBJECT
  QSqlDatabase _db;
  QByteArray _tableName, _pkPropName;
  QHash<QString,QObject*> _byPk;
  QList<QMetaProperty> _storedProperties;
  QHash<QByteArray,QMetaProperty> _storedPropertiesByName;
  QMetaMethod _persistSenderSlot;

public:
  /** @param metaobject type of objects that will be stored
   * @param db database to use
   * @param tableName if empty, use snake case object class name + 's'
   * @param pkPropName, if not "id"
   */
  SqlObjectsStore(
      const QMetaObject *metaobject, QSqlDatabase db,
      QByteArray tableName = QByteArray(), QByteArray pkPropName = "id",
      QObject *parent = 0);
  SqlObjectsStore(
      const QMetaObject *metaobject, QSqlDatabase db, QByteArray tableName,
      QObject *parent)
    : SqlObjectsStore(metaobject, db, tableName, "id", parent) { }
  SqlObjectsStore(
      const QMetaObject *metaobject, QSqlDatabase db, QObject *parent)
    : SqlObjectsStore(metaobject, db, QByteArray(), "id", parent) { }
  Result create(const QHash<QString, QVariant> &params) override;
  Result fetch() override;
  Result persist(QObject *object) override;
  Result dispose(QObject *object, bool shouldDelete = true) override;
  long apply(std::function<void(QObject*,ObjectsStore*,long)> f) override;
  using ObjectsStore::apply;

private:
  inline QObject *mapToObject(const QSqlRecord &r);
  Q_INVOKABLE void persistSenderSlot();
};

#endif // SQLOBJECTSSTORE_H
