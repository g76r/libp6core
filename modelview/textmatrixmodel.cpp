/* Copyright 2012-2017 Hallowyn, Gregoire Barbier and others.
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
#include "textmatrixmodel.h"
//#include "log/log.h"

TextMatrixModel::TextMatrixModel(QObject *parent)
  : QAbstractTableModel(parent), _rowsSortEnabled(false), _columnsSortEnabled(false) {
}

int TextMatrixModel::rowCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  //Log::fatal() << "rowCount " << _rowNames.size() << (parent.isValid() ? " true" : " false");
  return parent.isValid() ? 0 : _rowNames.size();
}

int TextMatrixModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  //Log::fatal() << "columnCount " << _columnNames.size();
  return _columnNames.size();
}

QVariant TextMatrixModel::data(const QModelIndex &index, int role) const {
  //Log::fatal() << "TextMatrixModel::data " << index.row() << " " << index.column() << " " << role;
  if (index.isValid() && index.row() >= 0 && index.row() < _rowNames.size()
      && index.column() >= 0 && index.column() < _columnNames.size()) {
    switch (role) {
    case Qt::DisplayRole:
      return _values.value(_rowNames.at(index.row()))
          .value(_columnNames.at(index.column()));
    }
  }
  return QVariant();
}

QVariant TextMatrixModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {
  //Log::fatal() << "headerData " << section << " " << orientation << " " << role;
  switch (orientation) {
  case Qt::Horizontal:
    if (section >= 0 && section < _columnNames.size()) {
      switch (role) {
      case Qt::DisplayRole:
        return _columnNames.at(section);
      }
    }
    break;
  case Qt::Vertical:
    if (section >= 0 && section < _rowNames.size()) {
      switch (role) {
      case Qt::DisplayRole:
        return _rowNames.at(section);
      }
    }
  }
  return QVariant();
}

QString TextMatrixModel::value(QString row, QString column) const {
  return _values.value(row).value(column);
}

/** @return -1 if found, or insertion point */
static inline int insertionPosition(QStringList &list, QString key,
                                    bool shouldSort) {
  int pos, len = list.size();
  for (pos = 0; pos < len; ++pos) {
    const QString &s = list[pos];
    // LATER parametrize comparison function
    int comparison = QString::compare(s, key);
    if (comparison == 0)
      return -1; // found it, already in the list
    else if (comparison > 0)
      break; // not in list, found next item in order
  }
  return shouldSort ? pos : len;
}

void TextMatrixModel::setCellValue(QString row, QString column, QString value) {
  //Log::fatal() << "TextMatrixModel::setCellValue " << row << " " << column << " " << value;
  // LATER optimize TextMatrixModel::setCellValue, complexity is O(4n) and should be at most O(log n)
  int pos = insertionPosition(_rowNames, row, _rowsSortEnabled);
  if (pos >= 0){
    //Log::fatal() << "insert row " << pos << " " << row;
    beginInsertRows(QModelIndex(), pos, pos);
    _rowNames.insert(pos, row);
    endInsertRows();
  }
  pos = insertionPosition(_columnNames, column, _columnsSortEnabled);
  if (pos >= 0) {
    //Log::fatal() << "insert column " << pos << " " << column;
    beginInsertColumns(QModelIndex(), pos, pos);
    _columnNames.insert(pos, column);
    endInsertColumns();
  }
  QHash<QString,QString> &values = _values[row];
  values.insert(column, value);
  QModelIndex i = index(_rowNames.indexOf(row),
                        _columnNames.indexOf(column));
  //Log::fatal() << "data changed " << i.row() << " " << i.column() << " " << value;
  emit dataChanged(i, i);
}

void TextMatrixModel::clear() {
  //Log::fatal() << "TextMatrixModel::clear";
  if (!_rowNames.isEmpty()) {
    beginRemoveRows(QModelIndex(), 0, _rowNames.size()-1);
    _rowNames.clear();
    _columnNames.clear();
    _values.clear();
    endRemoveRows();
  }
}

bool TextMatrixModel::removeColumn(QString name) {
  // LATER optimize
  for (int i = 0; i < columnCount(); ++i)
    if (_columnNames.at(i) == name)
      return QAbstractTableModel::removeColumn(i);
  return false;
}

bool TextMatrixModel::removeRow(QString name) {
  // LATER optimize
  for (int i = 0; i < rowCount(); ++i)
    if (_rowNames.at(i) == name)
      return QAbstractTableModel::removeRow(i);
  return false;
}
