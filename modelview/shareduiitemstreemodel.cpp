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
                      ((SharedUiItemsTreeModelItem*)parent.internalPointer())
                      ->child(row))
        : createIndex(row, column, _roots.value(row));
  return QModelIndex();
}

QModelIndex SharedUiItemsTreeModel::parent(const QModelIndex &child) const {
  if (child.isValid()) {
    SharedUiItemsTreeModelItem *c
        = ((SharedUiItemsTreeModelItem*)child.internalPointer());
    if (c && c->parent())
      return createIndex(c->row(), 0, c->parent());
  }
  return QModelIndex();
}

int SharedUiItemsTreeModel::rowCount(const QModelIndex &parent) const {
  //qDebug() << "rowCount:" << parent.isValid() << parent.parent().isValid();
  if (parent.isValid()) {
    SharedUiItemsTreeModelItem *p
        = ((SharedUiItemsTreeModelItem*)parent.internalPointer());
    if (p) { // null p is theorically impossible
      //qDebug() << "->" << p->childrenCount();
      return p->childrenCount();
    }
  } else {
    //qDebug() << "->" << _roots.size();
    return _roots.size();
  }
  return 0;
}

SharedUiItem SharedUiItemsTreeModel::itemAt(const QModelIndex &index) const {
  if (index.isValid()) {
    SharedUiItemsTreeModelItem *i
        = ((SharedUiItemsTreeModelItem*)index.internalPointer());
    if (i)
      return i->item();
  }
  return SharedUiItem();
}

void SharedUiItemsTreeModel::setRoots(
    QList<SharedUiItemsTreeModelItem*> roots) {
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
  // MAYDO add index to improve lookup performance
  // FIXME
  SharedUiItemsTreeModelItem *foo;
  return QModelIndex();
}

void SharedUiItemsTreeModel::changeItem(SharedUiItem newItem,
                                        SharedUiItem oldItem) {
  if (newItem.isNull()) {
    if (!oldItem.isNull()) {
      QModelIndex index = indexOf(oldItem);
      if (index.isValid())
        removeRow(index.row(), index.parent());
    }
  } else if (oldItem.isNull()) {
    // FIXME find a way to let subclasses choose insert position depending on item
    beginInsertRows(QModelIndex(), _roots.size(), _roots.size());
    _roots.append(new SharedUiItemsTreeModelItem(newItem, _roots.size()));
    endInsertRows();
  } else {
    QModelIndex index = indexOf(oldItem);
    SharedUiItemsTreeModelItem *ti
        = ((SharedUiItemsTreeModelItem*)index.internalPointer());
    if (ti) {
      ti->item() = newItem;
      emit dataChanged(index, index);
    }
  }
}
