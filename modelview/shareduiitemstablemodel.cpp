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
#include "shareduiitemstablemodel.h"
#include "modelview/shareduiitemlist.h"
#include "modelview/shareduiitemdocumentmanager.h"
#include <QMimeData>

SharedUiItemsTableModel::SharedUiItemsTableModel(QObject *parent)
  : SharedUiItemsModel(parent),
    _defaultInsertionPoint(SharedUiItemsTableModel::LastItem),
    _maxrows(INT_MAX) {
}

SharedUiItemsTableModel::SharedUiItemsTableModel(
    SharedUiItem templateItem, DefaultInsertionPoint defaultInsertionPoint,
    QObject *parent)
  : SharedUiItemsModel(parent),
    _defaultInsertionPoint(defaultInsertionPoint),
    _maxrows(INT_MAX) {
  setHeaderDataFromTemplate(templateItem);
}

int SharedUiItemsTableModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : _items.size();
}

QModelIndex SharedUiItemsTableModel::index(int row, int column,
                                           const QModelIndex &parent) const {
  if (hasIndex(row, column, parent))
    return createIndex(row, column);
  return QModelIndex();
}

QModelIndex SharedUiItemsTableModel::parent(const QModelIndex &child) const {
  Q_UNUSED(child)
  return QModelIndex();
}

void SharedUiItemsTableModel::setItems(const SharedUiItemList &original_items) {
  SharedUiItemList limited_items = original_items;
  qsizetype new_size = limited_items.size(), old_size = _items.size();
  if (new_size > _maxrows) {
    limited_items.remove(new_size, new_size-_maxrows);
    limited_items.squeeze();
    new_size = _maxrows;
  }
  if (old_size) {
    beginRemoveRows({}, 0, old_size-1);
    _items.clear();
    endRemoveRows();
  }
  if (new_size) {
    beginInsertRows({}, 0, new_size-1);
    _items = limited_items;
    endInsertRows();
  }
}

void SharedUiItemsTableModel::insertItemAt(const SharedUiItem &newItem,
    int row, const QModelIndex &parent) {
  if (row < 0 || row > rowCount() || parent.isValid())
    return;
  beginInsertRows(QModelIndex(), row, row);
  _items.insert(row, newItem);
  endInsertRows();
  int toBeRemoved = _items.size() - _maxrows;
  if (toBeRemoved > 0) {
    int deletionPoint = (_defaultInsertionPoint == FirstItem) ? _maxrows : 0;
    beginRemoveRows(QModelIndex(), deletionPoint, deletionPoint+toBeRemoved-1);
    _items.remove(deletionPoint, toBeRemoved);
    endRemoveRows();
  }
  //emit itemChanged(item, SharedUiItem());
}

bool SharedUiItemsTableModel::removeItems(int first, int last) {
  int rowCount = _items.size();
  if (first < 0 || last < first || first >= rowCount)
    return false;
  if (last >= rowCount)
    last = rowCount-1;
  beginRemoveRows(QModelIndex(), first, last);
  while (first <= last--) {
    //emit itemChanged(SharedUiItem(), _items.value(first));
    _items.removeAt(first);
  }
  endRemoveRows();
  return true;
}

SharedUiItem SharedUiItemsTableModel::itemAt(const QModelIndex &index) const {
  if (index.isValid())
    return _items.value(index.row());
  return SharedUiItem();
}

void SharedUiItemsTableModel::changeItem(
    const SharedUiItem &newItem, const SharedUiItem &originalOldItem,
    const Utf8String &qualifier) {
  SharedUiItem oldItem = originalOldItem;
  if (!itemQualifierFilter().isEmpty()
      && !itemQualifierFilter().contains(qualifier))
    return;
  if (newItem.isNull()) {
    QModelIndex oldIndex = indexOf(oldItem);
    if (oldIndex.isValid()) { // delete
      removeItems(oldIndex.row(), oldIndex.row());
    } else {
      // ignore changeItem(null,null)
      return;
    }
  } else {
    if (oldItem.isNull()) {
      // if an item with same id exists, change create into update
      QModelIndex index = indexOf(newItem.qualifiedId());
      if (index.isValid())
        oldItem = itemAt(index);
    } else {
      // if no item with oldItem id exists, change update into create
      QModelIndex index = indexOf(oldItem.qualifiedId());
      if (! index.isValid())
        oldItem = SharedUiItem();
    }
    if (oldItem.isNull()) { // create
      insertItemAt(newItem,
                   _defaultInsertionPoint == FirstItem ? 0 : rowCount());
    } else { // update (incl. rename)
      QModelIndex oldIndex = indexOf(oldItem);
      _items[oldIndex.row()] = newItem;
      emit dataChanged(index(oldIndex.row(), 0),
                       index(oldIndex.row(), columnCount()-1));
    }
  }
  emit itemChanged(newItem, oldItem);
}

QModelIndex SharedUiItemsTableModel::indexOf(
    const Utf8String &qualifiedId) const {
  // TODO add index to improve lookup performance
  // see SharedUiItemsTreeModel for index example
  // don't forget to update index changeItem when id changes (= item renamed)
  if (!qualifiedId.isNull())
    for (int row = 0; row < _items.size(); ++row)
      if (_items[row].qualifiedId() == qualifiedId)
        return createIndex(row, 0);
  return QModelIndex();
}

bool SharedUiItemsTableModel::removeRows(
    int row, int count, const QModelIndex &parent) {
  return parent.isValid() ? false : removeItems(row, row+count-1);
}

Qt::ItemFlags SharedUiItemsTableModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;
  return SharedUiItemsModel::flags(index)
      // the table topology is caracterized by only root having children
      | Qt::ItemNeverHasChildren
      // add selectable flag to all items by default, some models may hold
      // unselectable (structure) items
      | Qt::ItemIsSelectable
      // add drag and drop flags to enable internal dnd
      | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

QMimeData *SharedUiItemsTableModel::mimeData(
    const QModelIndexList &indexes) const {
  //qDebug() << "mimeData" << indexes.size() << indexes;
  if (indexes.isEmpty())
    return 0;
  QMimeData *md = new QMimeData;
  QSet<int> rowsSet;
  QStringList ids;
  QStringList rows;
  for (const auto &index: indexes) {
    int row = index.row();
    if (!rowsSet.contains(row)) {
      ids.append(itemAt(row).qualifiedId());
      rows.append(QString::number(row));
      rowsSet.insert(row);
    }
  }
  //qDebug() << "  list:" << ids << rows;
  md->setData(_suiQualifiedIdsListMimeType, ids.join(' ').toUtf8());
  md->setData(_suiPlacesMimeType, rows.join(' ').toUtf8());
  return md;
}

QStringList SharedUiItemsTableModel::mimeTypes() const {
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
bool SharedUiItemsTableModel::dropMimeData(
    const QMimeData *data, Qt::DropAction action, int targetRow,
    int targetColumn, const QModelIndex &droppedParent) {
  Q_UNUSED(action)
  Q_UNUSED(targetColumn)
  if (!data)
    return false;
  //qDebug() << "SharedUiItemsTableModel::dropMimeData" << action;
  Utf8StringList idsArrays =
      data->data(_suiQualifiedIdsListMimeType).split(' ');
  Utf8StringList rowsArrays = data->data(_suiPlacesMimeType).split(' ');
  SharedUiItemList items;
  QList<int> rows;
  if (droppedParent.isValid()) {
    // tree views will try to drop as child of hovered item
    // to preserve flat topology, drop after the item rather than as a child
    targetRow = droppedParent.row();
  }
  if (targetRow == -1) {
    // dropping outside any item, therefore append as last child
    targetRow = rowCount();
  }
  if (idsArrays.size() != rowsArrays.size()) {
    qDebug() << "SharedUiItemsTableModel::dropMimeData() received an "
                "inconsistent drop unusable for internal move";
    return false;
  }
  for (int i = 0; i < idsArrays.size(); ++ i) {
    auto qualifiedId = idsArrays[i];
    int row = rowsArrays[i].toInt();
    if (!qualifiedId.isEmpty() && row >= 0 && row < _items.size()
        && _items[row].qualifiedId() == qualifiedId) {
      items.append(_items[row]);
      rows.append(row);
    } else {
      qDebug() << "SharedUiItemsTableModel::dropMimeData() received an "
                  "external drop unusable for internal move";
      //qDebug() << "  " << !qualifiedId.isEmpty() << row << _items.size()
      //         << _items[row].qualifiedId() << qualifiedId;
      return false;
    }
  }
  moveRowsByRownums(QModelIndex(), rows, targetRow);
  return true;
}
