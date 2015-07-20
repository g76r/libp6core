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
#include <QtDebug>

// LATER add optimization methods such as sibling() and hasChildren()

SharedUiItemsTreeModel::SharedUiItemsTreeModel(QObject *parent)
  : SharedUiItemsModel(parent),
    _root(new TreeItem(this, SharedUiItem(), 0, 0)) {
}

SharedUiItemsTreeModel::~SharedUiItemsTreeModel() {
  delete _root;
}

QModelIndex SharedUiItemsTreeModel::index(
    int row, int column, const QModelIndex &parent) const {
  if (hasIndex(row, column, parent)) {
    const TreeItem *parentTreeItem = (TreeItem*)parent.internalPointer();
    if (!parentTreeItem) // this should never happen since no valid index points to _root
      parentTreeItem = _root;
    return createIndex(row, column, parentTreeItem->child(row));
  }
  return QModelIndex();
}

QModelIndex SharedUiItemsTreeModel::parent(const QModelIndex &child) const {
  if (child.isValid()) {
    TreeItem *childTreeItem = ((TreeItem*)child.internalPointer());
    TreeItem *parentTreeItem = childTreeItem ? childTreeItem->parent() : 0;
    if (parentTreeItem) // this should always happen since no valid index points to _root
      if (parentTreeItem != _root)
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
                        : _root->childrenCount();
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
  if (newItem.isNull()) {
    if (!oldItem.isNull()) { // delete
      QModelIndex index = indexOf(oldItem);
      TreeItem *treeItem = ((TreeItem*)index.internalPointer());
      if (treeItem)
        removeRows(treeItem->row(), 1, index.parent());
    }
  } else if (oldItem.isNull()) { // create
    QModelIndex parent;
    int row = -1; // -1 will be replaced by size() in adjustTreeItemAndRow()
    determineItemPlaceInTree(newItem, &parent, &row);
    TreeItem *parentTreeItem = ((TreeItem*)parent.internalPointer());
    adjustTreeItemAndRow(&parentTreeItem, &row);
    beginInsertRows(parent, row, row);
    new TreeItem(this, newItem, parentTreeItem, row);
    endInsertRows();
  } else { // update (or rename)
    QModelIndex index = indexOf(oldItem);
    TreeItem *treeItem = ((TreeItem*)index.internalPointer());
    if (treeItem) {
      QModelIndex oldParent = index.parent(), newParent = oldParent;
      QString newId = newItem.qualifiedId(), oldId = oldItem.qualifiedId();
      int row = -1; // -1 will be replaced by size() in adjustTreeItemAndRow()
      determineItemPlaceInTree(newItem, &newParent, &row);
      if (newParent != oldParent) { // parent in tree model changed
        TreeItem *oldParentTreeItem = ((TreeItem*)oldParent.internalPointer());
        if (!oldParentTreeItem)
          oldParentTreeItem = _root;
        TreeItem *newParentTreeItem = ((TreeItem*)newParent.internalPointer());
        adjustTreeItemAndRow(&newParentTreeItem, &row);
        if (beginMoveRows(oldParent, treeItem->row(), treeItem->row(),
                          newParent, row)) {
          newParentTreeItem->adoptChild(treeItem);
          updateIndexIfIdChanged(newId, oldId, treeItem);
          endMoveRows();
          goto end;
        } else {
          qDebug() << QString("ignoring result of ")
                      +this->metaObject()->className()
                      +"::determineItemPlaceInTree() since it was rejected by "
                      "QAbstractItemModel::beginMoveRows()";
        }
      }
      // the following is executed if either parent in tree model did not
      // change or the change was refused by beginMoveRows() because it was
      // found inconsistent
      treeItem->item() = newItem;
      updateIndexIfIdChanged(newId, oldId, treeItem);
      emit dataChanged(index, index);
    }
  }
end:
  emit itemChanged(newItem, oldItem);
}

void SharedUiItemsTreeModel::updateIndexIfIdChanged(
    QString newId, QString oldId, TreeItem *newTreeItem) {
  if (newId != oldId) {
    _itemsIndex.remove(oldId);
    _itemsIndex.insert(newId, newTreeItem);
  }
}

void SharedUiItemsTreeModel::adjustTreeItemAndRow(TreeItem **item, int *row) {
  if (!*item)
    *item = _root;
  int rowCount = (*item)->childrenCount();
  if (*row < 0 || *row > rowCount)
    *row = rowCount;
}

void SharedUiItemsTreeModel::determineItemPlaceInTree(
    SharedUiItem newItem, QModelIndex *parent, int *row) {
  Q_UNUSED(newItem)
  Q_UNUSED(parent)
  Q_UNUSED(row)
  // do nothing since we leave the default value as is
}

void SharedUiItemsTreeModel::clear() {
  _itemsIndex.clear();
  int rowCount = _root->childrenCount();
  if (rowCount > 0) {
    beginRemoveRows(QModelIndex(), 0, rowCount-1);
    while (rowCount--)
      _root->deleteChild(0);
    endRemoveRows();
  }
}

bool SharedUiItemsTreeModel::removeRows(
    int row, int count, const QModelIndex &parent) {
  if (row < 0 || count < 0)
    return false;
  TreeItem *parentTreeItem = ((TreeItem*)parent.internalPointer());
  if (!parentTreeItem)
    parentTreeItem = _root;
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

Qt::ItemFlags SharedUiItemsTreeModel::flags(const QModelIndex &index) const {
  return SharedUiItemsModel::flags(index)
      // add selectable flag to all items by default, some models may hold
      // unselectable (structure) items
      | Qt::ItemIsSelectable
      // add drag and drop flags to enable internal dnd
      | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}
