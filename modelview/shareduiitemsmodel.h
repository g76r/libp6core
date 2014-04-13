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
#ifndef SHAREDUIITEMSMODEL_H
#define SHAREDUIITEMSMODEL_H

#include <QAbstractItemModel>
#include "shareduiitem.h"
#include "libqtssu_global.h"

/** Base class for model holding AbstractUiItems. */
class LIBQTSSUSHARED_EXPORT SharedUiItemsModel : public QAbstractItemModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsModel)
  int _columnsCount;
  QHash<int,QHash<int,QVariant> > _mapRoleSectionHeader;
  QVariant _decorationAtColumn0;

public:
  explicit SharedUiItemsModel(QObject *parent = 0);
  QVariant data(const QModelIndex &index, int role) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  /** Set header according to what template item returns.
   * Also set columns count. */
  void setHeaderDataFromTemplate(const SharedUiItem &templateItem,
                              int role = Qt::DisplayRole);
  void setColumnsCount(int columnsCount) { _columnsCount = columnsCount; }
  /** Data returned for column 0 with Qt::DecorationRole */
  QVariant decorationAtColumn0() const { return _decorationAtColumn0; }
  void setDecorationAtColumn0(QVariant decoration) {
    _decorationAtColumn0 = decoration;  }
  virtual SharedUiItem itemAt(const QModelIndex &index) const = 0;
  virtual void updateItem(SharedUiItem item) = 0;
  virtual void renameItem(SharedUiItem item, QString oldId) = 0;
};

#endif // SHAREDUIITEMSMODEL_H
