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
    TreeItem(SharedUiItemsTreeModel *model, SharedUiItem item, TreeItem *parent,
             int row) : _model(model), _item(item), _row(row), _parent(parent) {
      QString id = item.qualifiedId();
      if (parent)
        parent->_children.insert(row, this);
      if (!id.isEmpty())
        _model->_itemsIndex.insert(id, this);
    }
    ~TreeItem() {
      _model->_itemsIndex.remove(_item.qualifiedId());
      if (_parent)
        _parent->_children.removeOne(this);
      QList<TreeItem*> tmp = _children;
      qDeleteAll(tmp);
    }
    SharedUiItem &item() { return _item; }
    int row() const { return _row; }
    TreeItem *parent() const { return _parent; }
    int childrenCount() const { return _children.size(); }
    TreeItem *child(int row) const {
      return row >= 0 && row < _children.size() ? _children[row] : 0;
    }
    void deleteChild(int row) {
      if (row < _children.size() && row >= 0) {
        TreeItem *child = _children[row];
        _children.removeAt(row);
        delete child;
      }
    }
  };
  friend class TreeItem;

private:
  TreeItem _root;
  QHash<QString,TreeItem*> _itemsIndex; // key: qualified id

public:
  explicit SharedUiItemsTreeModel(QObject *parent = 0);
  ~SharedUiItemsTreeModel();
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &child) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  SharedUiItem itemAt(const QModelIndex &index) const;
  using SharedUiItemsModel::indexOf;
  QModelIndex indexOf(QString qualifiedId) const;
  void changeItem(SharedUiItem newItem, SharedUiItem oldItem);
  bool removeRows(int row, int count, const QModelIndex &parent);

protected:
  void clear();
  /** This method is called to choose where in the tree changeItem() inserts
   * a new item (when oldItem.isNull()).
   * Subclasses should override this method to choose another place than
   * default one (after last row of root item, i.e. *parent = QModelIndex()
   * which means "root" and *row = -1 which means "last position of selected
   * parent"). */
  virtual void setNewItemInsertionPoint(
      SharedUiItem newItem, QModelIndex *parent, int *row);
};

#endif // SHAREDUIITEMSTREEMODEL_H
