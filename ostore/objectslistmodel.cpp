/* Copyright 2017-2023 Hallowyn, Gregoire Barbier and others.
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
#include "objectslistmodel.h"
#include "format/stringutils.h"
#include <QMetaProperty>

ObjectsListModel::ObjectsListModel(
    QObject *parent, const QMetaObject *metaobject, QByteArray objectRoleName,
    QByteArray rolePrefix, int baseUserRole)
  : QAbstractListModel(parent), _baseUserRole(baseUserRole) {
  Q_ASSERT(metaobject);
  if (objectRoleName.isEmpty())
    objectRoleName = StringUtils::toSnakeCase(metaobject->className());
  //qDebug() << "objectRoleName:" << objectRoleName
  //<< toSnakeCase(metaobject->className());
  _roleNames = QAbstractListModel::roleNames();
  _roleNames.insert(baseUserRole, objectRoleName);
  int first = metaobject->propertyOffset();
  int count = metaobject->propertyCount();
  for (int i = first, j = 0; i < count; ++i) {
    QMetaProperty prop = metaobject->property(i);
    if (!prop.isStored()) // MAYDO maybe there are other props to ignore
      continue;
    //qDebug() << "  stored property:" << j << prop.name() << baseUserRole+1+j
    //<< rolePrefix+prop.name();
    _storedProperties.append(prop);
    _roleNames.insert(baseUserRole+1+j, rolePrefix+prop.name());
    ++j;
  }
}

int ObjectsListModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : _objects.size();
}

QVariant ObjectsListModel::data(const QModelIndex &index, int role) const {
  QObject *object = _objects.value(index.row());
  if (!object || !index.isValid())
    return QVariant();
  if (role == _baseUserRole)
    return QVariant::fromValue<QObject*>(object);
  if (role > _baseUserRole)
    return _storedProperties.value(role-1-_baseUserRole).read(object);
  if (role == Qt::DisplayRole || role == Qt::EditRole)
    return _storedProperties.value(index.column()).read(object);
  return QVariant();
}

QVariant ObjectsListModel::headerData(
    int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal
      && (role == Qt::DisplayRole || role == Qt::EditRole))
    return _storedProperties.value(section).name();
  return QVariant();
}

QHash<int, QByteArray> ObjectsListModel::roleNames() const {
  return _roleNames;
}

void ObjectsListModel::append(QObject *object) {
  beginInsertRows(QModelIndex(), _objects.size(), _objects.size());
  _objects.append(object);
  endInsertRows();
}

void ObjectsListModel::append(QList<QObject*> objects) {
  beginInsertRows(QModelIndex(), _objects.size(),
                  _objects.size()+objects.size());
  for (QObject *object : objects)
    _objects.append(object);
  endInsertRows();
}

void ObjectsListModel::update(QObject *object) {
  int n = _objects.size();
  for (int i = 0; i < n; ++i) {
    if (_objects.value(i) == object) {
      QModelIndex index = this->index(i);
      emit dataChanged(index, index);
      return;
    }
  }
  append(object);
  //qDebug() << "ObjectsListModel::update" << _objects.size();
}

void ObjectsListModel::remove(QObject *object) {
  //qDebug() << "ObjectsListModel::remove" << object;
  int n = _objects.size();
  for (int i = 0; i < n; ++i) {
    if (_objects.value(i) == object) {
      beginRemoveRows(QModelIndex(), i, i);
      _objects.removeAt(i);
      //qDebug() << "  " << i;
      endRemoveRows();
      --i;
    }
  }
}
