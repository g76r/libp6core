/* Copyright 2014-2022 Hallowyn, Gregoire Barbier and others.
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
#ifndef CSVFILEMODEL_H
#define CSVFILEMODEL_H

#include <QAbstractTableModel>
#include "csvfile.h"

// TODO add signals and slots communication with CsvFile to make this class work for real

/** Wrap a CsvFile object to the Model-View framework. */
class LIBP6CORESHARED_EXPORT CsvFileModel : public QAbstractTableModel {
  Q_OBJECT
  Q_DISABLE_COPY(CsvFileModel)
  CsvFile *_csvFile;

public:
  explicit CsvFileModel(QObject *parent = 0);
  explicit CsvFileModel(CsvFile *csvFile, QObject *parent = 0);
  ~CsvFileModel();
  int columnCount(const QModelIndex &parent) const;
  int rowCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  CsvFile *csvFile() const;
  /** Do not take object ownership (won't delete it). */
  void setCsvFile(CsvFile *csvFile);
};

#endif // CSVFILEMODEL_H
