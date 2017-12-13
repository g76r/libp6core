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
#ifndef STRINGLISTDIFFMODEL_H
#define STRINGLISTDIFFMODEL_H

#include "libp6core_global.h"
#include <QAbstractTableModel>
#include <QStringList>

/** Model displaying two QString list side by side with diff-like decoration
 * (background colors).
 */
class LIBPUMPKINSHARED_EXPORT StringListDiffModel : public QAbstractTableModel {
  Q_OBJECT
  Q_DISABLE_COPY(StringListDiffModel)

public:
  enum Status { NoChange, Added, Removed, Modified };
  Q_ENUM(Status)

private:
  QStringList _beforeValues, _afterValues;
  QList<Status> _rowStatuses;

public:
  StringListDiffModel(QObject *parent = 0);
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(
      const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  void setValues(const QList<QString> &beforeValues,
                 const QList<QString> &afterValues);
  void clear();

protected:
  Status rowStatus(int row) const { return _rowStatuses.value(row); }
};


#endif // STRINGLISTDIFFMODEL_H
