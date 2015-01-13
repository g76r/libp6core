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

/** Model holding SharedUiItems, one item per line within a tree, one item
 * section per column. */
class LIBQTSSUSHARED_EXPORT SharedUiItemsTreeModel : public SharedUiItemsModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsTreeModel)

protected:
  /** Item used by SharedUiItemsTreeModel and subclasses. */
  class LIBQTSSUSHARED_EXPORT TreeItem {
    SharedUiItemsTreeModel *_model; // kind of java non-static inner class
    SharedUiItem _item;
    int _row;
    TreeItem *_parent;
    QList<TreeItem*> _children;

  public:
    /** Constructor for root items. */
    TreeItem(SharedUiItemsTreeModel *model, SharedUiItem item, int row)
      : _model(model), _item(item), _row(row), _parent(0) {
      QString id = item.qualifiedId();
      if (!id.isEmpty())
        _model->_itemsIndex.insert(id, this);
    }
    /** Constructor for non-root items. */
    TreeItem(SharedUiItemsTreeModel *model, SharedUiItem item, TreeItem *parent)
      : _model(model), _item(item), _row(parent->childrenCount()),
        _parent(parent) {
      parent->_children.append(this);
      QString id = item.qualifiedId();
      if (!id.isEmpty())
        _model->_itemsIndex.insert(id, this);
    }
    ~TreeItem() {
      _model->_itemsIndex.remove(_item.qualifiedId());
      qDeleteAll(_children);
    }
    SharedUiItem &item() { return _item; }
    int row() const { return _row; }
    TreeItem *parent() const { return _parent; }
    int childrenCount() const { return _children.size(); }
    TreeItem *child(int row) const {
      return row >= 0 && row < _children.size() ? _children[row] : 0;
    }
  };
  friend class TreeItem;

private:
  QList<TreeItem*> _roots;
  QHash<QString,TreeItem*> _itemsIndex; // key: qualified id

public:
  explicit SharedUiItemsTreeModel(QObject *parent = 0);
  ~SharedUiItemsTreeModel();
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &child) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  SharedUiItem itemAt(const QModelIndex &index) const;
  QModelIndex indexOf(SharedUiItem item) const;
  void changeItem(SharedUiItem newItem, SharedUiItem oldItem);

protected:
  QList<TreeItem*> roots() const { return _roots; }
  void setRoots(QList<TreeItem*> roots);
  void clearRoots() { setRoots(QList<TreeItem*>()); }
  /** This method is called to choose where in the tree changeItem() inserts
   * a new item (when oldItem.isNull()).
   * Subclasses should override this method to choose another place than
   * default one (after last row of root item, i.e. *parent = QModelIndex()
   * and *row = rowCount()). */
  virtual void setNewItemInsertionPoint(
      SharedUiItem newItem, QModelIndex *parent, int *row);
};

#endif // SHAREDUIITEMSTREEMODEL_H
