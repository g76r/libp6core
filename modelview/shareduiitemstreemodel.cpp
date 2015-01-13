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

void SharedUiItemsTreeModel::setRoots(QList<TreeItem*> roots) {
  // TODO rather determine what items must be deleted and what was kept
  if (_roots != roots) {
    if (!_roots.isEmpty()) {
      beginRemoveRows(QModelIndex(), 0, _roots.size()-1);
      qDeleteAll(_roots);
      _roots.clear();
      endRemoveRows();
    }
    if (!roots.isEmpty()) {
      beginInsertRows(QModelIndex(), 0, roots.size()-1);
      _roots = roots;
      endInsertRows();
    }
  }
}

QModelIndex SharedUiItemsTreeModel::indexOf(SharedUiItem item) const {
  TreeItem *treeItem = _itemsIndex.value(item.qualifiedId());
  return treeItem ? createIndex(treeItem->row(), 0, treeItem) : QModelIndex();
}

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
    int row = _roots.size();
    setNewItemInsertionPoint(newItem, &parent, &row);
    beginInsertRows(parent, row, row);
    _roots.append(new TreeItem(this, newItem, _roots.size()));
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
  // do nothing since we leave default value as is
}
