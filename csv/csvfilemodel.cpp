/* Copyright 2014-2017 Hallowyn, Gregoire Barbier and others.
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
#include "csvfilemodel.h"

CsvFileModel::CsvFileModel(QObject *parent)
  : QAbstractTableModel(parent), _csvFile(0) {
}

CsvFileModel::CsvFileModel(CsvFile *csvFile, QObject *parent)
  : QAbstractTableModel(parent), _csvFile(csvFile) {
}


int CsvFileModel::columnCount(const QModelIndex &parent) const {
  return _csvFile && !parent.isValid() ? _csvFile->columnCount() : 0;
}

int CsvFileModel::rowCount(const QModelIndex &parent) const {
  return _csvFile && !parent.isValid() ? _csvFile->rowCount() : 0;
}

QVariant CsvFileModel::data(const QModelIndex &index, int role) const {
  if (_csvFile && role == Qt::DisplayRole && index.isValid())
    return _csvFile->row(index.row()).value(index.column());
  return QVariant();
}

QVariant CsvFileModel::headerData(int section, Qt::Orientation orientation,
                                  int role) const {
  if (_csvFile && role == Qt::DisplayRole) {
    switch(orientation) {
    case Qt::Horizontal:
      return _csvFile->headers().value(section);
    case Qt::Vertical:
      return QString::number(section);
    }
  }
  return QVariant();
}

CsvFile *CsvFileModel::csvFile() const {
  return _csvFile;
}

void CsvFileModel::setCsvFile(CsvFile *csvFile) {
  beginResetModel();
  _csvFile = csvFile;
  endResetModel();
}
