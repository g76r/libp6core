/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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
#ifndef TEXTMATRIXMODEL_H
#define TEXTMATRIXMODEL_H

#include "libp6core_global.h"
#include <QAbstractTableModel>

/** Kind of 2-dimensions QStringListModel.
 *
 * Can optionaly sort columns and/or rows on the fly, by header names order.
 */
class LIBP6CORESHARED_EXPORT TextMatrixModel : public QAbstractTableModel {
  Q_OBJECT
  Q_DISABLE_COPY(TextMatrixModel)

  QStringList _columnNames, _rowNames;
  QHash<QString,QHash<QString,QString> > _values;
  bool _rowsSortEnabled, _columnsSortEnabled;

public:
  explicit TextMatrixModel(QObject *parent = 0);
  /** Should sort rows. Default: false. */
  void enableRowsSort(bool enabled = true) { _rowsSortEnabled = enabled; }
  bool rowsSortEnabled() const { return _rowsSortEnabled; }
  /** Should sort columns. Default: false. */
  void enableColumnsSort(bool enabled = true) { _columnsSortEnabled = enabled; }
  bool columnsSortEnabled() const { return _columnsSortEnabled; }
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  QString value(QString row, QString column) const;
  /** Set a cell value.
   * Row and/or column will be added if they do not yet exist. */
  void setCellValue(QString row, QString column, QString value);
  /** Remove any data. */
  void clear();
  using QAbstractTableModel::removeColumn;
  /** Remove a column, knowing its name. */
  bool removeColumn(QString name);
  using QAbstractTableModel::removeRow;
  /** Remove a row, knowing its name. */
  bool removeRow(QString name);
};

#endif // TEXTMATRIXMODEL_H
