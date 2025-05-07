/* Copyright 2014-2025 Hallowyn, Gregoire Barbier and others.
 * This file is part of libpumpkin, see <http://libpumpkin.g76r.eu/>.
 * Libpumpkin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libpumpkin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libpumpkin.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "shareduiitemstreemodel.h"
#include <QMimeData>

// LATER add optimization methods such as sibling() and hasChildren()

SharedUiItemsTreeModel::TreeItem::TreeItem(
    SharedUiItemsTreeModel *model, SharedUiItem item, TreeItem *parent,
    int row) : _model(model), _item(item), _row(row), _parent(parent) {
  auto id = item.qualifiedId();
  if (parent) {
    parent->_children.insert(row, this);
    for (++row; row < parent->_children.size(); ++row)
      _parent->_children[row]->_row = row;
  }
  if (!id.isEmpty())
    _model->_itemsIndex.insert(id, this);
}

SharedUiItemsTreeModel::TreeItem::~TreeItem() {
  _model->_itemsIndex.remove(_item.qualifiedId());
  qDeleteAll(_children);
}

void SharedUiItemsTreeModel::TreeItem::adoptChild(TreeItem *child, int newRow) {
  //qDebug() << "adoptChild" << this << child << newRow << "/" << _children.size();
  Q_ASSERT_X(child, "SharedUiItemsTreeModel::TreeItem::adoptChild()",
             "null child");
  Q_ASSERT_X(!isDescendantOf(child),
             "SharedUiItemsTreeModel::TreeItem::adoptChild()",
             "cannot adopt an ancestor");
  Q_ASSERT_X((newRow >= 0 && newRow <= _children.size()),
             "SharedUiItemsTreeModel::TreeItem::adoptChild()",
             "inconsistent row number");
  if (child->_parent)
    child->_parent->removeChild(child->_row, false);
  child->_parent = this;
  child->_row = newRow;
  _children.insert(newRow, child);
}

bool SharedUiItemsTreeModel::TreeItem::isDescendantOf(TreeItem *ancestor) const {
  if (!ancestor)
    return false;
  if (ancestor == this)
    return true;
  for (TreeItem *ti = _parent; ti; ti = ti->_parent)
    if (ti == ancestor)
      return true;
  return false;
}

void SharedUiItemsTreeModel::TreeItem::removeChild(int row, bool shouldDelete) {
  Q_ASSERT_X((row >= 0 && row < _children.size()),
             "SharedUiItemsTreeModel::TreeItem::removeChild",
             "inconsistent row number");
  if (shouldDelete)
    delete _children[row];
  _children.removeAt(row);
  for (; row < _children.size(); ++row)
    _children[row]->_row = row;
}

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
    const TreeItem *parentTreeItem =
        parent.isValid() ? treeItemByIndex(parent) : _root;
    return createIndex(row, column, parentTreeItem->child(row));
  }
  return QModelIndex();
}

QModelIndex SharedUiItemsTreeModel::parent(const QModelIndex &child) const {
  if (child.isValid()) {
    TreeItem *parentTreeItem = treeItemByIndex(child)->parent();
    // parentTreeItem should never be null since no valid index points to _root
    Q_ASSERT_X(parentTreeItem, "SharedUiItemsTreeModel::parent",
               "inconsistent item index");
    if (parentTreeItem != _root)
      return createIndex(parentTreeItem->row(), 0, parentTreeItem);
  }
  return QModelIndex();
}

int SharedUiItemsTreeModel::rowCount(const QModelIndex &parent) const {
  return treeItemByIndex(parent)->childrenCount();
}

SharedUiItem SharedUiItemsTreeModel::itemAt(const QModelIndex &index) const {
  return treeItemByIndex(index)->item();
}

QModelIndex SharedUiItemsTreeModel::indexOf(
    const Utf8String &qualifiedId) const {
  TreeItem *treeItem = _itemsIndex.value(qualifiedId);
  return treeItem ? createIndex(treeItem->row(), 0, treeItem) : QModelIndex();
}

void SharedUiItemsTreeModel::changeItem(
    const SharedUiItem &newItem, const SharedUiItem &originalOldItem,
    const Utf8String &qualifier) {
  SharedUiItem oldItem = originalOldItem;
  if (!itemQualifierFilter().isEmpty()
      && !itemQualifierFilter().contains(qualifier))
    return;
  //qDebug() << "SharedUiItemsTreeModel::changeItem" << newItem.id()
  //         << oldItem.id() << qualifier;
  if (newItem.isNull()) {
    if (!oldItem.isNull()) { // delete
      QModelIndex oldIndex = indexOf(oldItem);
      //qDebug() << "delete" << newItem << oldItem << oldIndex.isValid();
      if (oldIndex.isValid()) {
        removeRows(treeItemByIndex(oldIndex)->row(), 1, oldIndex.parent());
      }
    } else {
      // ignore changeItem(null,null)
    }
  } else {
    if (oldItem.isNull()) {
      // if an item with same id exists, change create into update
      TreeItem *treeItem = _itemsIndex.value(newItem.qualifiedId());
      if (treeItem)
        oldItem = treeItem->item();
    } else {
      // if no item with oldItem id exists, change update into create
      TreeItem *treeItem = _itemsIndex.value(oldItem.qualifiedId());
      if (!treeItem)
        oldItem = SharedUiItem();
    }
    if (oldItem.isNull()) { // create
      //qDebug() << "create" << newItem << oldItem; // << _itemsIndex.keys();
      QModelIndex parent;
      int row = -1; // -1 will be replaced by size() in adjustTreeItemAndRow()
      determineItemPlaceInTree(newItem, &parent, &row);
      TreeItem *parentTreeItem = treeItemByIndex(parent);
      adjustTreeItemAndRow(&parentTreeItem, &row);
      beginInsertRows(parent, row, row);
      new TreeItem(this, newItem, parentTreeItem, row);
      endInsertRows();
    } else { // update (incl. rename)
      //qDebug() << "update" << newItem << oldItem;
      QModelIndex oldIndex = indexOf(oldItem);
      TreeItem *treeItem = treeItemByIndex(oldIndex);
      QModelIndex oldParent = oldIndex.parent(), newParent = oldParent;
      auto newId = newItem.qualifiedId(), oldId = oldItem.qualifiedId();
      int newRow = -1; // -1 will be replaced by size() in adjustTreeItemAndRow()
      determineItemPlaceInTree(newItem, &newParent, &newRow);
      TreeItem *newParentTreeItem = treeItemByIndex(newParent);
      adjustTreeItemAndRow(&newParentTreeItem, &newRow);
      treeItem->item() = newItem;
      updateIndexIfIdChanged(newId, oldId, treeItem);
      emit dataChanged(oldIndex, oldIndex);
      // LATER make it possible for determineItemPlaceInTree to change row without changing parent
      if (newParent != oldParent) { // need to move item in the tree
        if (newParentTreeItem->isDescendantOf(treeItem)) {
          qDebug() << "SharedUiItemsTreeModel::changeItem denies item moving "
                      "because new parent would be a descendant of moved "
                      "child.";
        } else {
          //qDebug() << "reparenting:" << treeItem->item().id()
          //         <<"parent:" << oldParent << treeItem
          //        << treeItem->row() << "->" << newParent
          //        << newParentTreeItem << newRow << "/"
          //        << newParentTreeItem->childrenCount();
          //qDebug() << "  root:" << _root << _root->item().id();
          Q_ASSERT_X(beginMoveRows(
                       oldParent, treeItem->row(), treeItem->row(),
                       newParent, newRow),
                     "SharedUiItemsTreeModel::changeItem",
                     "inconsistent reparenting according to beginMoveRows");
          newParentTreeItem->adoptChild(treeItem, newRow);
          updateIndexIfIdChanged(newId, oldId, treeItem); // LATER not sure needed
          endMoveRows();
        }
      }
    }
  }
  emit itemChanged(newItem, oldItem);
}

void SharedUiItemsTreeModel::updateIndexIfIdChanged(
    const Utf8String &newId, const Utf8String &oldId, TreeItem *newTreeItem) {
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
    const SharedUiItem &, QModelIndex *, int *) {
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
    const SharedUiItem &newItem, int row, const QModelIndex &parent) {
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
  for  (const auto &index: indexes) {
    QString path = itemPath(index);
    if (!pathsSet.contains(path)) {
      ids.append(itemAt(index).qualifiedId());
      paths.append(path);
      pathsSet.insert(path);
    }
  }
  md->setData(_suiQualifiedIdsListMimeType, ids.join(' ').toUtf8());
  md->setData(_suiPlacesMimeType, paths.join(' ').toUtf8());
  return md;
}

QStringList SharedUiItemsTreeModel::mimeTypes() const {
  return _suiMimeTypes;
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
  Utf8StringList idsArrays =
      data->data(_suiQualifiedIdsListMimeType).split(' ');
  Utf8StringList pathsArrays = data->data(_suiPlacesMimeType).split(' ');
  auto firstParentPath = splitPath(pathsArrays.value(0));
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
    auto qualifiedId = idsArrays[i];
    int row;
    // can only move items if they are all child of same parent
    if (splitPath(pathsArrays[i], &row) != firstParentPath)
      return false;
    TreeItem *treeItem = treeItemByIndex(index(row, 0, sourceParent));
    if (treeItem->childrenCount())
      return false; // LATER learn to move non-leaves items
    if (!qualifiedId.isEmpty() && treeItem
        && treeItem->item().qualifiedId() == qualifiedId) {
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

Utf8String SharedUiItemsTreeModel::itemPath(const QModelIndex &index) {
  if (index.parent().isValid())
      return itemPath(index.parent())+"."+Utf8String::number(index.row());
  if (index.isValid())
    return Utf8String::number(index.row());
  return {};
}

Utf8String SharedUiItemsTreeModel::splitPath(
    const Utf8String &path, int *rownum) {
  int i = path.lastIndexOf('.');
  if (i < 0) {
    if (rownum)
      *rownum = path.toInt();
    return {};
  } else {
    if (rownum)
      *rownum = path.mid(i+1).toInt();
    return path.left(i);
  }
}

QModelIndex SharedUiItemsTreeModel::indexFromPath(const Utf8String &path) {
  if (path.isEmpty())
    return {};
  auto elements = path.split('.');
  //qDebug() << "indexFromPath" << path << elements;
  QModelIndex index;
  for (const auto &element: elements) {
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
      qWarning() << "SharedUiItemsTreeModel received an index not related to "
                    "this model:" << index.model() << "instead of" << this;
      Q_ASSERT_X((index.model() == this),
                 "SharedUiItemsTreeModel::treeItemByIndex",
                 "index not related to this model");
    } else {
      TreeItem *treeItem = (TreeItem*)index.internalPointer();
      Q_ASSERT_X(treeItem, "SharedUiItemsTreeModel::treeItemByIndex",
                 "inconsistent index");
      return treeItem;
    }
  }
  return _root;
}
