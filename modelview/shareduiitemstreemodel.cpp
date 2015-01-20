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
#include "shareduiitemstreemodel.h"

// LATER add optimization methods such as sibling() and hasChildren()

SharedUiItemsTreeModel::SharedUiItemsTreeModel(QObject *parent)
  : SharedUiItemsModel(parent), _root(this, SharedUiItem(), 0, 0) {
}

SharedUiItemsTreeModel::~SharedUiItemsTreeModel() {
}

QModelIndex SharedUiItemsTreeModel::index(
    int row, int column, const QModelIndex &parent) const {
  if (hasIndex(row, column, parent)) {
    const TreeItem *parentTreeItem = (TreeItem*)parent.internalPointer();
    if (!parentTreeItem) // this should never happen since no valid index points to _root
      parentTreeItem = &_root;
    return createIndex(row, column, parentTreeItem->child(row));
  }
  return QModelIndex();
}

QModelIndex SharedUiItemsTreeModel::parent(const QModelIndex &child) const {
  if (child.isValid()) {
    TreeItem *childTreeItem = ((TreeItem*)child.internalPointer());
    TreeItem *parentTreeItem = childTreeItem ? childTreeItem->parent() : 0;
    if (parentTreeItem) // this should always happen since no valid index points to _root
      if (parentTreeItem != &_root)
        return createIndex(parentTreeItem->row(), 0, parentTreeItem);
  }
  return QModelIndex();
}

int SharedUiItemsTreeModel::rowCount(const QModelIndex &parent) const {
  TreeItem *parentTreeItem = ((TreeItem*)parent.internalPointer());
  //  if (parentTreeItem)
  //    qDebug() << "SharedUiItemsTreeModel::rowCount"
  //             << parentTreeItem->item().qualifiedId()
  //             << parentTreeItem->childrenCount();
  return parentTreeItem ? parentTreeItem->childrenCount()
                        : _root.childrenCount();
}

SharedUiItem SharedUiItemsTreeModel::itemAt(const QModelIndex &index) const {
  TreeItem *treeItem = ((TreeItem*)index.internalPointer());
  return treeItem ? treeItem->item() : SharedUiItem();
}

QModelIndex SharedUiItemsTreeModel::indexOf(QString qualifiedId) const {
  TreeItem *treeItem = _itemsIndex.value(qualifiedId);
  return treeItem ? createIndex(treeItem->row(), 0, treeItem) : QModelIndex();
}

void SharedUiItemsTreeModel::changeItem(SharedUiItem newItem,
                                        SharedUiItem oldItem) {
  //qDebug() << "SharedUiItemsTreeModel::changeItem" << newItem.qualifiedId()
  //         << oldItem.qualifiedId();
  if (newItem.isNull()) {
    if (!oldItem.isNull()) { // delete
      QModelIndex index = indexOf(oldItem);
      TreeItem *treeItem = ((TreeItem*)index.internalPointer());
      if (treeItem)
        removeRows(treeItem->row(), 1, index.parent());
    }
  } else if (oldItem.isNull()) { // create
    QModelIndex parent;
    int row = -1; // will be replaced by size() a few lines below
    setNewItemInsertionPoint(newItem, &parent, &row);
    TreeItem *parentTreeItem = ((TreeItem*)parent.internalPointer());
    if (!parentTreeItem)
      parentTreeItem = &_root;
    int rowCount = parentTreeItem->childrenCount();
    if (row < 0 || row > rowCount)
      row = rowCount;
    //qDebug() << "   SharedUiItemsTreeModel::changeItem: beginInsertRows"
    //         << parent << row << row;
    beginInsertRows(parent, row, row);
    new TreeItem(this, newItem, parentTreeItem, row);
    //qDebug() << "SharedUiItemsTreeModel::changeItem: endInsertRows";
    endInsertRows();
  } else { // update
    QModelIndex index = indexOf(oldItem);
    TreeItem *treeItem = ((TreeItem*)index.internalPointer());
    if (treeItem) {
      treeItem->item() = newItem;
      // update index, in case id has changed
      _itemsIndex.remove(oldItem.qualifiedId());
      _itemsIndex.insert(newItem.qualifiedId(), treeItem);
      emit dataChanged(index, index);
    }
  }
}

void SharedUiItemsTreeModel::setNewItemInsertionPoint(
    SharedUiItem newItem, QModelIndex *parent, int *row) {
  Q_UNUSED(newItem)
  Q_UNUSED(parent)
  Q_UNUSED(row)
  // do nothing since we leave the default value as is
}

void SharedUiItemsTreeModel::clear() {
  _itemsIndex.clear();
  int rowCount = _root.childrenCount();
  if (rowCount > 0) {
    beginRemoveRows(QModelIndex(), 0, rowCount-1);
    while (rowCount--)
      _root.deleteChild(0);
    endRemoveRows();
  }
}

bool SharedUiItemsTreeModel::removeRows(
    int row, int count, const QModelIndex &parent) {
  if (row < 0 || count < 0)
    return false;
  TreeItem *parentTreeItem = ((TreeItem*)parent.internalPointer());
  if (!parentTreeItem)
    parentTreeItem = &_root;
  int rowCount = parentTreeItem->childrenCount();
  int last = row+count-1;
  if (row >= rowCount)
    return true;
  if (last >= rowCount) {
    last = rowCount-1;
    count = last-row+1;
  }
  //qDebug() << "SharedUiItemsTreeModel::removeRows"
  //         << parent << row << last << count << parent.data();
  beginRemoveRows(parent, row, last);
  while (count--)
    parentTreeItem->deleteChild(row);
  endRemoveRows();
  return true;
}
