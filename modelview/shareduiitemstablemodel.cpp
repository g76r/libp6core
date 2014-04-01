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

SharedUiItemsTableModel::SharedUiItemsTableModel(
    SharedUiItem templateItem, QObject *parent)
  : QAbstractTableModel(parent), _templateItem(templateItem) {
}

SharedUiItemsTableModel::SharedUiItemsTableModel(QObject *parent)
  : QAbstractTableModel(parent) {
}

int SharedUiItemsTableModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : _items.size();
}

int SharedUiItemsTableModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  return _templateItem.uiDataCount();
}

QVariant SharedUiItemsTableModel::data(const QModelIndex &index, int role) const {
  return index.isValid()
      ? _items.value(index.row()).uiData(index.column(), role)
      : QVariant();
}

QVariant SharedUiItemsTableModel::headerData(int section, Qt::Orientation orientation,
                                int role) const {
  return orientation == Qt::Horizontal
      ? _templateItem.uiHeaderData(section, role) : QVariant();
}

void SharedUiItemsTableModel::resetItems(QList<SharedUiItem> items) {
  beginResetModel();
  _items = items;
  endResetModel();
}
