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
#ifndef OBJECTSLISTMODEL_H
#define OBJECTSLISTMODEL_H

#include <QAbstractListModel>
#include <QMetaProperty>
#include "libp6core_global.h"

class QMetaObject;

/** Model for a list of same class QObjects, mapping their properties to roles,
 * and the QObject itself to an object role, in order to make the object and
 * its properties accessible to QML views.
 *
 * Only own QMetaObject properties are mapped, not the one inherited from
 * parents, for instance if used with the QMetaObject of class Foo which extends
 * class Bar which extends QObject, then only properties declared within class
 * Foo will be mapped, not the one declared within class Bar.
 *
 * Can work standalone but designed to be used with ObjectsStore.
 *
 * @see ObjectsStore
 */
class LIBPUMPKINSHARED_EXPORT ObjectsListModel : public QAbstractListModel {
  Q_OBJECT
  QList<QObject*> _objects;
  QHash<int, QByteArray> _roleNames;
  int _baseUserRole;
  QList<QMetaProperty> _storedProperties;

public:
  /** @param metaobject type of objects that will be stored
   * @param objectRoleName if empty, use snake cased class name of object
   */
  ObjectsListModel(QObject *parent, const QMetaObject *metaobject,
                   QByteArray objectRoleName = { }, QByteArray rolePrefix = { },
                   int baseUserRole = Qt::UserRole);
  ObjectsListModel(const QMetaObject *metaobject,
                   QByteArray objectRoleName = { }, QByteArray rolePrefix = { },
                   int baseUserRole = Qt::UserRole)
    : ObjectsListModel(nullptr, metaobject, objectRoleName, rolePrefix,
                       baseUserRole) { }
  int rowCount(const QModelIndex &parent = { }) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

public slots:
  /** notify changes to views, will append object if not already in the list
   * do not take ownership, but the object must remain valid until removed */
  void update(QObject *object);
  /** remove item from list, but do not delete the object itself */
  void remove(QObject *object);

private:
  /** do not take ownership, but the object must remain valid until removed */
  void append(QObject *object);
  /** do not take ownership, but the object must remain valid until removed */
  void append(QList<QObject *> objects);
};

#endif // OBJECTSLISTMODEL_H
