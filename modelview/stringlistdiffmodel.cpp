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
#include "stringlistdiffmodel.h"

StringListDiffModel::StringListDiffModel(QObject *parent)
  : QAbstractTableModel(parent) {
}

int StringListDiffModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : _rowStatuses.size();
}

int StringListDiffModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  return 3;
}

QVariant StringListDiffModel::data(
    const QModelIndex &index, int role) const {
  switch(role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    switch(index.column()) {
    case 0:
      return _beforeValues.value(index.row());
    case 1:
      return _afterValues.value(index.row());
    case 2:
      return _rowStatuses.value(index.row()); // FIXME toString
    }
    break;
  }
  return QVariant();
}

QVariant StringListDiffModel::headerData(
    int section, Qt::Orientation orientation, int role) const {
  switch(orientation) {
  case Qt::Horizontal:
    switch(role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      switch(section) {
      case 0:
        return tr("Before");
      case 1:
        return tr("After");
      case 2:
        return tr("Status");
      }
      break;
    }
    break;
  default:
    ;
  }
  return QVariant();
}

void StringListDiffModel::setValues(const QList<QString> &beforeValues,
                                    const QList<QString> &afterValues) {
  clear();
  int n = std::max(beforeValues.size(), afterValues.size());
  beginInsertRows(QModelIndex(), 0, n);
  for (int i = 0; i < n; ++i) {
    auto before = beforeValues.value(i);
    auto after = afterValues.value(i);
    if (before == after)
      _rowStatuses.append(NoChange);
    else if (before.isEmpty())
      _rowStatuses.append(Added);
    else if (after.isEmpty())
      _rowStatuses.append(Removed);
    else
      _rowStatuses.append(Modified);
  }
  _beforeValues = beforeValues;
  _afterValues = afterValues;
  endInsertRows();
}

void StringListDiffModel::clear() {
  beginRemoveRows(QModelIndex(), 0, rowCount()-1);
  _beforeValues.clear();
  _afterValues.clear();
  _rowStatuses.clear();
  endRemoveRows();
}
