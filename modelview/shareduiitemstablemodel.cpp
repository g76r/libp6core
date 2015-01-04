/* Copyright 2014-2015 Hallowyn and others.
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

void SharedUiItemsTableModel::insertItemAt(int row, SharedUiItem item) {
  if (row < 0 || row > rowCount())
    return;
  beginInsertRows(QModelIndex(), row, row);
  _items.insert(row, item);
  endInsertRows();
  //emit itemChanged(item, SharedUiItem());
}

void SharedUiItemsTableModel::removeItems(int first, int last) {
  if (first < 0 || last >= rowCount() || last < first)
    return;
  beginRemoveRows(QModelIndex(), first, last);
  while (first <= last--) {
    //emit itemChanged(SharedUiItem(), _items.value(first));
    _items.removeAt(first);
  }
  endRemoveRows();
}

SharedUiItem SharedUiItemsTableModel::itemAt(const QModelIndex &index) const {
  if (index.isValid())
    return _items.value(index.row());
  return SharedUiItem();
}

SharedUiItem SharedUiItemsTableModel::itemAt(int row) const {
  if (row > 0 && row < _items.size())
    return _items.value(row);
  return SharedUiItem();
}

void SharedUiItemsTableModel::changeItem(SharedUiItem newItem,
                                         SharedUiItem oldItem) {
  QModelIndex oldIndex = indexOf(oldItem);
  if (newItem.isNull()) {
    if (oldIndex.isValid())
      removeItems(oldIndex.row(), oldIndex.row());
  } else if (oldItem.isNull() || !oldIndex.isValid()) {
    insertItemAt(rowCount(), newItem);
  } else {
    _items[oldIndex.row()] = newItem;
    emit dataChanged(oldIndex, index(oldIndex.row(), columnCount()-1));
  }
}

QModelIndex SharedUiItemsTableModel::indexOf(SharedUiItem item) const {
  // MAYDO add index to improve lookup performance
  if (!item.isNull())
    for (int row = 0; row < _items.size(); ++row)
      if (_items[row] == item)
        return createIndex(row, 0);
  return QModelIndex();
}
