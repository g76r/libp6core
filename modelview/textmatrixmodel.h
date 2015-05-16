/* Copyright 2012-2015 Hallowyn and others.
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
#ifndef TEXTMATRIXMODEL_H
#define TEXTMATRIXMODEL_H

#include "libqtssu_global.h"
#include <QAbstractTableModel>
#include <QStringList>
#include <QHash>

/** Kind of 2-dimensions QStringListModel. */
class LIBQTSSUSHARED_EXPORT TextMatrixModel : public QAbstractTableModel {
  Q_OBJECT
  QStringList _columnNames, _rowNames;
  QHash<QString,QHash<QString,QString> > _values;

public:
  explicit TextMatrixModel(QObject *parent = 0);
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  QString value(QString row, QString column) const;

public slots:
  /** Set a cell value.
   * Row and/or column will be added if they do not yet exist. */
  void setCellValue(QString row, QString column, QString value);
  /** Remove any data. */
  void clear();
};

#endif // TEXTMATRIXMODEL_H
