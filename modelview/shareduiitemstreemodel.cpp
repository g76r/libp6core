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
#include <QMimeData>

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
    const TreeItem *parentTreeItem = treeItemByIndex(parent);
    if (!parentTreeItem) // this should never happen since no valid index points to _root
      parentTreeItem = _root;
    return createIndex(row, column, parentTreeItem->child(row));
  }
  return QModelIndex();
}

QModelIndex SharedUiItemsTreeModel::parent(const QModelIndex &child) const {
  if (child.isValid()) {
    TreeItem *childTreeItem = treeItemByIndex(child);
    TreeItem *parentTreeItem = childTreeItem ? childTreeItem->parent() : 0;
    if (parentTreeItem) // this should always happen since no valid index points to _root
      if (parentTreeItem != _root)
        return createIndex(parentTreeItem->row(), 0, parentTreeItem);
  }
  return QModelIndex();
}

int SharedUiItemsTreeModel::rowCount(const QModelIndex &parent) const {
  TreeItem *parentTreeItem = treeItemByIndex(parent);
  //  if (parentTreeItem)
  //    qDebug() << "SharedUiItemsTreeModel::rowCount"
  //             << parentTreeItem->item().qualifiedId()
  //             << parentTreeItem->childrenCount();
  return parentTreeItem ? parentTreeItem->childrenCount()
                        : _root->childrenCount();
}

SharedUiItem SharedUiItemsTreeModel::itemAt(const QModelIndex &index) const {
  TreeItem *treeItem = treeItemByIndex(index);
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
      TreeItem *treeItem = treeItemByIndex(index);
      if (treeItem)
        removeRows(treeItem->row(), 1, index.parent());
    }
  } else if (oldItem.isNull()) { // create
    QModelIndex parent;
    int row = -1; // -1 will be replaced by size() in adjustTreeItemAndRow()
    determineItemPlaceInTree(newItem, &parent, &row);
    TreeItem *parentTreeItem = treeItemByIndex(parent);
    adjustTreeItemAndRow(&parentTreeItem, &row);
    beginInsertRows(parent, row, row);
    new TreeItem(this, newItem, parentTreeItem, row);
    endInsertRows();
  } else { // update (or rename)
    QModelIndex index = indexOf(oldItem);
    TreeItem *treeItem = treeItemByIndex(index);
    if (treeItem) {
      QModelIndex oldParent = index.parent(), newParent = oldParent;
      QString newId = newItem.qualifiedId(), oldId = oldItem.qualifiedId();
      int row = -1; // -1 will be replaced by size() in adjustTreeItemAndRow()
      determineItemPlaceInTree(newItem, &newParent, &row);
      if (newParent != oldParent) { // parent in tree model changed
        TreeItem *oldParentTreeItem = treeItemByIndex(oldParent);
        if (!oldParentTreeItem)
          oldParentTreeItem = _root;
        TreeItem *newParentTreeItem = treeItemByIndex(newParent);
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
  TreeItem *parentTreeItem = treeItemByIndex(parent);
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
  //dumpForDebug();
  //qDebug() << "SharedUiItemsTreeModel::removeRows"
  //         << parentTreeItem->child(row)->item().id()
  //         << row << count << parent << itemAt(parent).id() << last;
  beginRemoveRows(parent, row, last);
  while (count--) {
    //qDebug() << "  removing" << row << count;
    parentTreeItem->deleteChild(row);
  }
  endRemoveRows();
  //qDebug() << "  removed. parent children:" << parentTreeItem->childrenCount()
  //         << "index keys:" << _itemsIndex.keys();
  //for (int i = 0; i < parentTreeItem->childrenCount(); ++i)
  //  qDebug() << "  child:" << i << parentTreeItem->child(i)->item().id();
  //dumpForDebug();
  return true;
}

void SharedUiItemsTreeModel::insertItemAt(
    SharedUiItem newItem, int row, QModelIndex parent) {
  //qDebug() << "SharedUiItemsTreeModel::insertItemAt"
  //         << newItem << row << parent << itemAt(parent).id();
  TreeItem *parentTreeItem = treeItemByIndex(parent);
  adjustTreeItemAndRow(&parentTreeItem, &row);
  //dumpForDebug();
  //qDebug() << "  adjusted:" << newItem << row << parent << itemAt(parent).id();
  beginInsertRows(parent, row, row);
  new TreeItem(this, newItem, parentTreeItem, row);
  endInsertRows();
  //qDebug() << "  inserted. parent children:" << parentTreeItem->childrenCount()
  //        << "index keys:" << _itemsIndex.keys();
  //for (int row = 0; row < parentTreeItem->childrenCount(); ++row)
  //  qDebug() << "  child:" << row << parentTreeItem->child(row)
  //           << parentTreeItem->child(row)->item().id();
  //dumpForDebug();
}

Qt::ItemFlags SharedUiItemsTreeModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;
  return SharedUiItemsModel::flags(index)
      // add selectable flag to all items by default, some models may hold
      // unselectable (structure) items
      | Qt::ItemIsSelectable
      // add drag and drop flags to enable internal dnd
      | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

QMimeData *SharedUiItemsTreeModel::mimeData(
    const QModelIndexList &indexes) const {
  if (indexes.isEmpty())
    return 0;
  QMimeData *md = new QMimeData;
  QSet<QString> pathsSet;
  QStringList ids;
  QStringList paths;
  foreach (const QModelIndex &index, indexes) {
    QString path = itemPath(index);
    if (!pathsSet.contains(path)) {
      ids.append(itemAt(index).qualifiedId());
      paths.append(path);
      pathsSet.insert(path);
    }
  }
  md->setData(suiQualifiedIdsListMimeType, ids.join(' ').toUtf8());
  md->setData(suiPlacesMimeType, paths.join(' ').toUtf8());
  return md;
}

QStringList SharedUiItemsTreeModel::mimeTypes() const {
  return suiMimeTypes;
}

// Support for moving rows by internal drag'n drop within the same view.
// Note that the implementation is different from the one done by
// QAbstractItemModel: there is no need that the action be MoveAction, which
// make it possible for the internal move to work even if the view is in
// DragAndDrop mode, not only in InternalMove, and therefore it is possible to
// mix with external drag'n drop; as a consequence, it is possible, although
// hard to do on purpose, to trigger strange drag'n drop move by draging items
// from a view and dropping them onto another one, provided dragged items are
// found too in both view at the same time.
// Since we may accept drop from other views, we need to strongly check that
// every dropped item belong to this model. And otherwise do nothing.
// Only support moving rows if they all belong to the same parent, and their
// target parent is the same than their source parent.
bool SharedUiItemsTreeModel::dropMimeData(
    const QMimeData *data, Qt::DropAction action, int targetRow,
    int targetColumn, const QModelIndex &droppedParent) {
  Q_UNUSED(action)
  Q_UNUSED(targetColumn)
  if (!data)
    return false;
  //qDebug() << "SharedUiItemsTreeModel::dropMimeData action:" << action
  //         << "pos:" << targetRow << targetColumn << droppedParent.data() << "data:"
  //         << QString::fromUtf8(data->data(suiQualifiedIdsListMimeType))
  //         << QString::fromUtf8(data->data(suiPlacesMimeType));
  QList<QByteArray> idsArrays =
      data->data(suiQualifiedIdsListMimeType).split(' ');
  QList<QByteArray> pathsArrays = data->data(suiPlacesMimeType).split(' ');
  QString firstParentPath = splitPath(pathsArrays.value(0));
  QModelIndex sourceParent = indexFromPath(firstParentPath);
  QModelIndex targetParent = droppedParent;
  //qDebug() << "*******************" << targetRow << targetColumn
  //         << sourceParent << targetParent;
  if (sourceParent != targetParent) {
    // cannot reparent when moving, only reordering is supported
    if (sourceParent == targetParent.parent()) {
      // in the case the target parent is a child of the source parent,
      // drop after the target parent
      targetRow = targetParent.row();
      targetParent = targetParent.parent();
    } else {
      return false;
    }
  }
  //qDebug() << "******************2" << targetRow << targetColumn
  //         << sourceParent << targetParent;
  if (targetRow == -1) {
    if (targetParent.isValid()) {
      // dropping directly on parent item, therefore insert as first child
      targetRow =  0;
    } else {
      // dropping outside any item, therefore append as last child
      targetRow = rowCount(targetParent);
    }
  }
  //qDebug() << "firstParentPath:" << firstParentPath << "sourceParent:"
  //        << sourceParent << sourceParent.data().toString()
  //         << "targetParent:" << targetParent << targetParent.data().toString()
  //         << "targetRow:" << targetRow;
  QList<int> rows;
  if (idsArrays.size() != pathsArrays.size()) {
    qDebug() << "SharedUiItemsTreeModel::dropMimeData() received an "
                "inconsistent drop unusable for internal move";
    return false;
  }
  for (int i = 0; i < idsArrays.size(); ++ i) {
    QString qualifiedId = QString::fromUtf8(idsArrays[i]);
    int row;
    // can only move items if they are all child of same parent
    if (splitPath(QString::fromLatin1(pathsArrays[i]), &row)
        != firstParentPath)
      return false;
    TreeItem *ti = treeItemByIndex(index(row, 0, sourceParent));
    if (ti->childrenCount())
      return false; // LATER learn to move non-leaves items
    if (!qualifiedId.isEmpty() && ti
        && ti->item().qualifiedId() == qualifiedId) {
      rows.append(row);
    } else {
      //qDebug() << "SharedUiItemsTreeModel::dropMimeData() received an "
      //            "external drop unusable for internal move";
      //qDebug() << "  " << ti << "id:"
      //         << (ti?ti->item().qualifiedId():QString()) << qualifiedId
      //        << "row:" << row << (ti?ti->row():-1)
      //         << "parent:" << (ti?ti->parent():0);
      //qDebug() << "   index content:" << _itemsIndex;
      return false;
    }
  }
  // note that if dropped right on parent, row == -1, so row+1 will be 0
  moveRowsByRownums(sourceParent, rows, targetRow);
  return true;
}

QString SharedUiItemsTreeModel::itemPath(const QModelIndex &index) {
  if (index.parent().isValid())
      return itemPath(index.parent())+'.'+QString::number(index.row());
  if (index.isValid())
    return QString::number(index.row());
  return QString();
}

QString SharedUiItemsTreeModel::splitPath(QString path, int *rownum) {
  int i = path.lastIndexOf('.');
  if (i < 0) {
    if (rownum)
      *rownum = path.toInt();
    return QString();
  } else {
    if (rownum)
      *rownum = path.mid(i+1).toInt();
    return path.left(i);
  }
}

QModelIndex SharedUiItemsTreeModel::indexFromPath(QString path) {
  if (path.isEmpty())
    return QModelIndex();
  QStringList elements = path.split('.');
  //qDebug() << "indexFromPath" << path << elements;
  QModelIndex index;
  foreach (const QString &element, elements) {
    int row = element.toInt();
    index = this->index(row, 0, index);
    //qDebug() << "  " << element << row << index;
    if (!index.isValid()) // if some part of the path is incorrect, return root
      break;
  }
  //qDebug() << "  ->" << index;
  return index;
}

/*
void SharedUiItemsTreeModel::dumpForDebug() const {
  qDebug() << "begin dump";
  _root->dumpForDebug();
  qDebug() << "end dump";
}

void SharedUiItemsTreeModel::TreeItem::dumpForDebug(
    QString indentation, const SharedUiItemsTreeModel::TreeItem *parent) const {
  qDebug() << indentation << this << "item:" << _item.id() << "row:" << _row
           << "parent:" << _parent << "children:" << _children.size();
  if (_parent && _parent->_children.value(_row) != this)
    qDebug() << indentation << "INCONSISTENT PARENT (row) row:" << _row
             << "parent.child[row]:" << _parent->_children.value(_row)
             << "this:" << this;
  if (_parent != parent)
    qDebug() << indentation << "INCONSISTENT PARENT (pointer)";
  if (_model) {
    if (this->_parent) {
      TreeItem *indexed = _model->_itemsIndex.value(_item.qualifiedId());
      if (indexed != this) {
        qDebug() << indentation << "INCONSISTENT INDEX indexed:" << indexed
                 << "this:" << this;
      }
    }
  } else {
    qDebug() << indentation << "NULL MODEL";
  }
  indentation.append("  ");
  for (const TreeItem *child : _children)
    child->dumpForDebug(indentation, this);
}
*/

SharedUiItemsTreeModel::TreeItem *SharedUiItemsTreeModel::treeItemByIndex(
    const QModelIndex &index) const {
  if (index.isValid()) {
    if (index.model() != this) {
      // must never happen
      qWarning() << "SharedUiItemsTreeModel received an index not related to "
                    "this model" << index << this;
    } else {
      return (TreeItem*)index.internalPointer();
    }
  }
  return _root;
}
