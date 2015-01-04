/* Copyright 2014-2015 Hallowyn and others.
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
#ifndef SHAREDUIITEMSMODEL_H
#define SHAREDUIITEMSMODEL_H

#include <QAbstractItemModel>
#include "shareduiitem.h"
#include "libqtssu_global.h"

/** Base class for model holding SharedUiItems, being them table or
 * tree-oriented they provides one item section per column. */
class LIBQTSSUSHARED_EXPORT SharedUiItemsModel : public QAbstractItemModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsModel)
  int _columnsCount;
  QHash<int,QHash<int,QVariant> > _mapRoleSectionHeader;
  //QVariant _decorationAtColumn0;

public:
  explicit SharedUiItemsModel(QObject *parent = 0);
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  /** Set header according to what template item returns.
   * Also set columns count. */
  void setHeaderDataFromTemplate(const SharedUiItem &templateItem,
                                 int role = Qt::DisplayRole);
  /* Data returned for column 0 with Qt::DecorationRole */
  /*QVariant decorationAtColumn0() const { return _decorationAtColumn0; }
  void setDecorationAtColumn0(QVariant decoration) {
    _decorationAtColumn0 = decoration;  }*/
  virtual SharedUiItem itemAt(const QModelIndex &index) const = 0;
  virtual QModelIndex indexOf(SharedUiItem item) const = 0;
  Qt::ItemFlags	flags(const QModelIndex & index) const;

public slots:
  /** Notify a change on an item concerning this model.
   * Ready to connect to DocumentManager::itemChanged() signal, or any more
   * precise signals (kind of FoobarDocumentManager::foobarItemChanged(
   * Foobar newFoobar, Foobar oldFoobar)). */
  virtual void changeItem(SharedUiItem newItem, SharedUiItem oldItem) = 0;

signals:
  /** Emited whenever an item is created, renamed, updated or deleted,
   * being it through setData(), changeItem(). */
  // TODO not sure this is usefull
  //void itemChanged(SharedUiItem newItem, SharedUiItem oldItem);
};

#endif // SHAREDUIITEMSMODEL_H
