/* Copyright 2014 Hallowyn and others.
 * This file is part of qron, see <http://qron.hallowyn.com/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#include "shareduiitemstablemodel.h"

SharedUiItemsTableModel::SharedUiItemsTableModel(QObject *parent)
  : SharedUiItemsModel(parent) {
}

int SharedUiItemsTableModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : _items.size();
}

QModelIndex SharedUiItemsTableModel::index(int row, int column,
                                           const QModelIndex &parent) const {
  if (hasIndex(row, column, parent))
    return createIndex(row, column);
  return QModelIndex();
}

QModelIndex SharedUiItemsTableModel::parent(const QModelIndex &child) const {
  Q_UNUSED(child)
  return QModelIndex();
}

void SharedUiItemsTableModel::setItems(QList<SharedUiItem> items) {
  if (!_items.isEmpty()) {
    beginRemoveRows(QModelIndex(), 0, _items.size()-1);
    _items.clear();
    endRemoveRows();
  }
  if (!items.isEmpty()) {
    beginInsertRows(QModelIndex(), 0, items.size()-1);
    _items = items;
    endInsertRows();
  }
}

void SharedUiItemsTableModel::updateItem(SharedUiItem item) {
  // LATER improve performance
  for (int row = 0; row < _items.size(); ++row) {
    if (_items[row].id() == item.id()) {
      _items[row] = item;
      emit dataChanged(index(row, 0), index(row, columnCount(QModelIndex())-1));
    }
  }
}

void SharedUiItemsTableModel::renameItem(SharedUiItem item, QString oldId) {
  // LATER improve performance
  for (int row = 0; row < _items.size(); ++row) {
    if (_items[row].id() == oldId) {
      _items[row] = item;
      emit dataChanged(index(row, 0), index(row, columnCount(QModelIndex())-1));
    }
  }
}

SharedUiItem SharedUiItemsTableModel::itemAt(const QModelIndex &index) const {
  if (index.isValid())
    return _items.value(index.row());
  return SharedUiItem();
}
