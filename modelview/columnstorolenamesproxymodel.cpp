/* Copyright 2016-2023 Hallowyn, Gregoire Barbier and others.
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
#include "columnstorolenamesproxymodel.h"

void ColumnsToRolenamesProxyModel::setFirstMappedRole(int role) {
  if (role < Qt::UserRole)
    qWarning() << "ColumnsToRolenamesProxyModel::setFirstMappedRole called with"
                  " a role number < Qt::UserRole: " << role;
  _firstMappedRole = role;
}

void ColumnsToRolenamesProxyModel::setSourceModel(
    QAbstractItemModel *sourceModel) {
  auto *oldModel = this->sourceModel();
  if (oldModel)
    disconnect(oldModel, &QAbstractItemModel::modelReset,
               this, &ColumnsToRolenamesProxyModel::refreshRolenamesFromColumnHeaders);
  QIdentityProxyModel::setSourceModel(sourceModel);
  if (sourceModel) {
    connect(sourceModel, &QAbstractItemModel::modelReset,
            this, &ColumnsToRolenamesProxyModel::refreshRolenamesFromColumnHeaders);
    refreshRolenamesFromColumnHeaders();
  }
}

void ColumnsToRolenamesProxyModel::refreshRolenamesFromColumnHeaders() {
  auto *model = sourceModel();
  if (!model) {
    _roles.clear();
    _reverseRoles.clear();
    return;
  }
  // keep default rolenames (provided no column header name overrides them)
  _roles = model->roleNames();
  for (const int &key : _roles.keys())
    _reverseRoles.insert(QString::fromUtf8(_roles.value(key)), key);
  // use column names as QML role names and map them to QtWidgets user roles,
  // force lower case names
  int count = columnCount();
  for (int i = 0; i < count; ++i) {
    // LATER make case a parameter
    QString roleName = _rolenamesPrefix
        +model->headerData(i, Qt::Horizontal).toString().toLower();
    _roles[_firstMappedRole+i] = roleName.toUtf8();
    _reverseRoles[roleName] = _firstMappedRole+i;
  }
}

QHash<int, QByteArray> ColumnsToRolenamesProxyModel::roleNames() const {
  return _roles;
}

QVariant ColumnsToRolenamesProxyModel::data(
    const QModelIndex &index, int role) const {
  auto *model = sourceModel();
  if (!model)
    return QVariant();
  // map QtWidgets user roles to columns, using display roles, to make it
  // possible to access columns by their names from QML
  if (role >= _firstMappedRole)
    return model->data(
          model->index(index.row(), role-_firstMappedRole, index.parent()),
          Qt::DisplayRole);
  else
    return model->data(index, role);
}

bool ColumnsToRolenamesProxyModel::setData(
    const QModelIndex &index, const QVariant &value, int role) {
  auto *model = sourceModel();
  if (!model)
    return false;
  //qDebug() << "setData" << index << value << role;
  // map QtWidgets user roles to columns, using display roles, to make it
  // possible to access columns by their names from QML
  if (role >= _firstMappedRole)
    return model->setData(
          model->index(index.row(), role-_firstMappedRole, index.parent()),
          value, _setDataRole);
  else
    return model->setData(index, value, role);
}

bool ColumnsToRolenamesProxyModel::setData(
    int row, const QVariant &value, const QString &roleName) {
  auto *model = sourceModel();
  //qDebug() << "setData (by name)" << row << value << roleName
  //         << _reverseRoles.value(roleName, -1);
  if (!model)
    return false;
  int role = _reverseRoles.value(roleName, -1);
  if (role == -1)
    return false;
  return model->setData(
        model->index(row, role-_firstMappedRole, QModelIndex()),
        value, _setDataRole);
}

bool ColumnsToRolenamesProxyModel::insertRow(
    int row, const QModelIndex &parent) {
  return QIdentityProxyModel::insertRows(row, 1, parent);
}

bool ColumnsToRolenamesProxyModel::insertRows(
    int row, int count, const QModelIndex &parent) {
  return QIdentityProxyModel::insertRows(row, count, parent);
}

bool ColumnsToRolenamesProxyModel::removeRow(
    int row, const QModelIndex &parent) {
  return QIdentityProxyModel::removeRows(row, 1, parent);
}

bool ColumnsToRolenamesProxyModel::removeRows(
    int row, int count, const QModelIndex &parent) {
  return QIdentityProxyModel::removeRows(row, count, parent);
}

bool ColumnsToRolenamesProxyModel::insertColumn(
    int column, const QModelIndex &parent) {
  return QIdentityProxyModel::insertColumns(column, 1, parent);
}

bool ColumnsToRolenamesProxyModel::insertColumns(
    int column, int count, const QModelIndex &parent) {
  return QIdentityProxyModel::insertColumns(column, count, parent);
}

bool ColumnsToRolenamesProxyModel::removeColumn(
    int column, const QModelIndex &parent) {
  return QIdentityProxyModel::removeColumns(column, 1, parent);
}

bool ColumnsToRolenamesProxyModel::removeColumns(
    int column, int count, const QModelIndex &parent) {
  return QIdentityProxyModel::removeColumns(column, count, parent);
}
