/* Copyright 2012-2013 Hallowyn and others.
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
#include "textmatrixmodel.h"
//#include "log/log.h"

TextMatrixModel::TextMatrixModel(QObject *parent)
  : QAbstractTableModel(parent) {
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

QString TextMatrixModel::value(const QString row, const QString column) const {
  return _values.value(row).value(column);
}

void TextMatrixModel::setCellValue(const QString row, const QString column,
                               const QString value) {
  //Log::fatal() << "TextMatrixModel::setCellValue " << row << " " << column << " " << value;
  if (!_rowNames.contains(row)) {
    int pos = _rowNames.size();
    //Log::fatal() << "insert row " << pos << " " << row;
    beginInsertRows(QModelIndex(), pos, pos);
    _rowNames.append(row);
    endInsertRows();
  }
  if (!_columnNames.contains(column)) {
    int pos = _columnNames.size();
    //Log::fatal() << "insert column " << pos << " " << column;
    beginInsertColumns(QModelIndex(), pos, pos);
    _columnNames.append(column);
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
  beginResetModel();
  _rowNames.clear();
  _columnNames.clear();
  _values.clear();
  endResetModel();
}
