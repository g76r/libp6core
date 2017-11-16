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
#include "stringhashmodel.h"

StringHashModel::StringHashModel(QObject *parent)
  : QAbstractTableModel(parent) {
}

void StringHashModel::clear() {
  if (!_rowNames.isEmpty()) {
    beginRemoveRows(QModelIndex(), 0, _rowNames.size()-1);
    _rowNames.clear();
    _values.clear();
    endRemoveRows();
  }
}

void StringHashModel::setValues(const QHash<QString,QString> &values) {
  clear();
  if (!values.isEmpty()) {
    auto keys = values.keys();
    beginInsertRows(QModelIndex(), 0, keys.size()-1);
    for (const QString &key : keys)
      _rowNames.append(key);
    _values = values;
    endInsertRows();
  }
}

int StringHashModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : _rowNames.size();
}

int StringHashModel::columnCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : 2;
}

QVariant StringHashModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= _rowNames.size()
      || role != Qt::DisplayRole)
    return QVariant();
  switch (index.column()) {
  case 0:
    return _rowNames.value(index.row());
  case 1:
    return _values.value(_rowNames.value(index.row()));
  }
  return QVariant();
}

QVariant StringHashModel::headerData(
    int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    switch(section) {
    case 0:
      return tr("Key");
    case 1:
      return tr("Value");
    }
  return QVariant();
}

Qt::ItemFlags StringHashModel::flags(const QModelIndex &index) const {
  Qt::ItemFlags flags = QAbstractTableModel::flags(index);
  if (flags.testFlag(Qt::ItemIsEnabled))
    flags |= Qt::ItemIsEditable;
  return flags;
}

bool StringHashModel::setData(const QModelIndex &index, const QVariant &value,
                              int role) {
  if (!index.isValid() || index.row() < 0 || index.row() >= _rowNames.size()
      || role != Qt::EditRole)
    return false;
  auto string = value.toString();
  switch (index.column()) {
  case 0: {
    QString oldKey = _rowNames.value(index.row());
    if (oldKey == string) // nothing to do
      return true;
    if (string.isEmpty()) // reject empty keys
      return false;
    QString oldValue = _values.value(oldKey);
    int changedRow = index.row();
    QModelIndex changedIndex = index;
    int removedRow = _rowNames.indexOf(string);
    if (removedRow >= 0) { // must remove another row with same key
      beginRemoveRows(QModelIndex(), removedRow, removedRow);
      _rowNames.removeAt(removedRow);
      endRemoveRows();
      if (removedRow < changedRow) {
        --changedRow;
        changedIndex = this->index(changedRow, index.column());
      }
    }
    _rowNames[changedRow] = string;
    _values.remove(oldKey);
    _values.insert(string, oldValue);
    emit dataChanged(changedIndex, changedIndex);
    emit valuesChanged(_values);
    return true;
  }
  case 1:
    _values.insert(_rowNames.value(index.row()), string);
    emit dataChanged(index, index);
    emit valuesChanged(_values);
    return true;
  }
  return false;
}

int StringHashModel::setValue(const QString &key, const QString &value) {
  int row = _rowNames.indexOf(key);
  if (row >= 0) { // key already exists
    if (_values.value(key) == value) // nothing to do
      return row;
    _values.insert(key, value);
    QModelIndex changedIndex = index(row, 1);
    emit dataChanged(changedIndex, changedIndex);
    emit valuesChanged(_values);
  } else if (key.isEmpty()) { // reject empty keys
    return -1;
  } else { // key does not exist, append new row
    row = _rowNames.size();
    beginInsertRows(QModelIndex(), row, row);
    _rowNames.append(key);
    _values.insert(key, value);
    endInsertRows();
    emit valuesChanged(_values);
  }
  return row;
}

void StringHashModel::removeValue(const QString &key) {
  int row = _rowNames.indexOf(key);
  if (row < 0)
    return; // nothing to do
  beginRemoveRows(QModelIndex(), row, row);
  _rowNames.removeAt(row);
  _values.remove(key);
  endRemoveRows();
  emit valuesChanged(_values);
}

int StringHashModel::addNewKey() {
  QString key;
  for (int i = 1; i < 20; ++i) {
    key = tr("New Key ")+QString::number(i);
    if (!_values.contains(key))
      break;
  }
  while (_values.contains(key))
    key = tr("New Key ")+QString::number(qrand());
  return setValue(key, QString());
}

bool StringHashModel::removeRows(
    int row, int count, const QModelIndex &parent) {
  int last = row+count-1;
  if (row < 0 || last >= _rowNames.size() || parent.isValid())
    return false;
  beginRemoveRows(parent, row, last);
  for (int i = 0; i < count; ++i) {
    _values.remove(_rowNames.value(row));
    _rowNames.removeAt(row);
  }
  endRemoveRows();
  emit valuesChanged(_values);
  return true;
}
