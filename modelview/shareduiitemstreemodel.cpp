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

SharedUiItemsTreeModel::SharedUiItemsTreeModel(QObject *parent)
  : SharedUiItemsModel(parent) {
}

SharedUiItemsTreeModel::~SharedUiItemsTreeModel() {
  qDeleteAll(_roots);
}

QModelIndex SharedUiItemsTreeModel::index(int row, int column,
                                    const QModelIndex &parent) const {
  // note: hasIndex() checks row and columns boundaries by calling rowCount()
  // and columnCount() for parent, therefore also indirectly checking that
  // parent.internalPointer() is not null
  //qDebug() << "GroupsTasksModel::index" << row << column << parent.isValid()
  //         << hasIndex(row, column, parent) << parent.parent().isValid();
  if (hasIndex(row, column, parent))
    return parent.isValid()
        ? createIndex(row, column,
                      ((TreeItem*)parent.internalPointer())->child(row))
        : createIndex(row, column, _roots.value(row));
  return QModelIndex();
}

QModelIndex SharedUiItemsTreeModel::parent(const QModelIndex &child) const {
  if (child.isValid()) {
    TreeItem *c = ((TreeItem*)child.internalPointer());
    if (c && c->parent())
      return createIndex(c->row(), 0, c->parent());
  }
  return QModelIndex();
}

int SharedUiItemsTreeModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    TreeItem *p = ((TreeItem*)parent.internalPointer());
    if (p) { // null p is theorically impossible
      return p->childrenCount();
    }
  } else {
    return _roots.size();
  }
  return 0;
}

SharedUiItem SharedUiItemsTreeModel::itemAt(const QModelIndex &index) const {
  if (index.isValid()) {
    TreeItem *i = ((TreeItem*)index.internalPointer());
    if (i)
      return i->item();
  }
  return SharedUiItem();
}

QModelIndex SharedUiItemsTreeModel::indexOf(QString qualifiedId) const {
  TreeItem *treeItem = _itemsIndex.value(qualifiedId);
  return treeItem ? createIndex(treeItem->row(), 0, treeItem) : QModelIndex();
}

#include <QtDebug>
void SharedUiItemsTreeModel::changeItem(SharedUiItem newItem,
                                        SharedUiItem oldItem) {
  if (newItem.isNull()) {
    if (!oldItem.isNull()) { // delete
      QModelIndex index = indexOf(oldItem);
      if (index.isValid())
        removeRow(index.row(), index.parent());
    }
  } else if (oldItem.isNull()) { // create
    QModelIndex parent;
    int row = -1; // will be replaced by size() a few lines below
    setNewItemInsertionPoint(newItem, &parent, &row);
    TreeItem *parentTreeItem = ((TreeItem*)parent.internalPointer());
    int rowCount = parentTreeItem ? parentTreeItem->childrenCount()
                                  : _roots.size();
    if (row < 0 || row > rowCount)
      row = rowCount;
    qDebug() << "SharedUiItemsTreeModel::changeItem: beginInsertRows"
             << parent << row << row;
    beginInsertRows(parent, row, row);
    new TreeItem(this, newItem, parentTreeItem, row);
    qDebug() << "SharedUiItemsTreeModel::changeItem: endInsertRows";
    endInsertRows();
  } else { // update
    QModelIndex index = indexOf(oldItem);
    TreeItem *ti = ((TreeItem*)index.internalPointer());
    if (ti) {
      ti->item() = newItem;
      // update index, in case id has changed
      _itemsIndex.remove(oldItem.qualifiedId());
      _itemsIndex.insert(newItem.qualifiedId(), ti);
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
  if (_roots.size() > 0) {
    beginRemoveRows(QModelIndex(), 0, _roots.size()-1);
    qDeleteAll(_roots);
    _roots.clear();
    endRemoveRows();
  }
}

#include <QtDebug>
bool SharedUiItemsTreeModel::removeRows(
    int row, int count, const QModelIndex &parent) {
  if (row < 0 || count < 0)
    return false;
  TreeItem *parentTreeItem = ((TreeItem*)parent.internalPointer());
  int rowCount = parentTreeItem ? parentTreeItem->childrenCount()
                                : _roots.size();
  int last = row+count-1;
  if (row >= rowCount)
    return true;
  if (last >= rowCount) {
    last = rowCount-1;
    count = last-row+1;
  }
  qDebug() << "SharedUiItemsTreeModel::removeRows"
           << parent << row << last << count << parent.data();
  beginRemoveRows(parent, row, last);
  while (count--)
    parentTreeItem->deleteChild(row);
  endRemoveRows();
  return true;
}

