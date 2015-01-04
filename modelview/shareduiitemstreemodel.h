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
#ifndef SHAREDUIITEMSTREEMODEL_H
#define SHAREDUIITEMSTREEMODEL_H

#include "shareduiitemsmodel.h"
#include "shareduiitem.h"
#include <QList>

/** Item used by SharedUiItemsTreeModel and subclasses. */
class LIBQTSSUSHARED_EXPORT SharedUiItemsTreeModelItem {
  SharedUiItem _item;
  int _row;
  SharedUiItemsTreeModelItem *_parent;
  QList<SharedUiItemsTreeModelItem*> _children;

public:
  /** Constructor for root items. */
  SharedUiItemsTreeModelItem(SharedUiItem item, int row)
    : _item(item), _row(row), _parent(0) { }
  /** Constructor for non-root items. */
  SharedUiItemsTreeModelItem(
      SharedUiItem item, SharedUiItemsTreeModelItem *parent)
    : _item(item), _row(parent->childrenCount()), _parent(parent) {
    parent->_children.append(this);
  }
  ~SharedUiItemsTreeModelItem() {
    qDeleteAll(_children);
  }
  SharedUiItem &item() { return _item; }
  int row() const { return _row; }
  SharedUiItemsTreeModelItem *parent() const { return _parent; }
  int childrenCount() const { return _children.size(); }
  SharedUiItemsTreeModelItem *child(int row) const {
    return row >= 0 && row < _children.size() ? _children[row] : 0;
  }
};

/** Model holding SharedUiItems, one item per line within a tree, one item
 * section per column. */
class LIBQTSSUSHARED_EXPORT SharedUiItemsTreeModel : public SharedUiItemsModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsTreeModel)
  QList<SharedUiItemsTreeModelItem*> _roots;

public:
  explicit SharedUiItemsTreeModel(QObject *parent = 0);
  ~SharedUiItemsTreeModel();
  QModelIndex index(int row, int column, const QModelIndex &parent) const;
  QModelIndex parent(const QModelIndex &child) const;
  int rowCount(const QModelIndex &parent) const;
  SharedUiItem itemAt(const QModelIndex &index) const;
  QModelIndex indexOf(SharedUiItem item) const;
  void changeItem(SharedUiItem newItem, SharedUiItem oldItem);
  QList<SharedUiItemsTreeModelItem*> roots() const { return _roots; }
  void setRoots(QList<SharedUiItemsTreeModelItem*> roots);
  void clearRoots() { setRoots(QList<SharedUiItemsTreeModelItem*>()); }
};

#endif // SHAREDUIITEMSTREEMODEL_H
