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
#include "shareduiitemstablemodel.h"
#include <QtDebug>
#include <QMimeData>
#include "modelview/shareduiitemlist.h"
#include "modelview/shareduiitemdocumentmanager.h"

SharedUiItemsTableModel::SharedUiItemsTableModel(QObject *parent)
  : SharedUiItemsModel(parent),
    _defaultInsertionPoint(SharedUiItemsTableModel::LastItem),
    _maxrows(INT_MAX) {
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

void SharedUiItemsTableModel::setItems(QList<SharedUiItem> items) {
  if (!_items.isEmpty()) {
    beginRemoveRows(QModelIndex(), 0, _items.size()-1);
    _items.clear();
    endRemoveRows();
  }
  if (!items.isEmpty()) {
    beginInsertRows(QModelIndex(), 0, items.size()-1);
    _items = items;
    endInsertRows();
  }
}

void SharedUiItemsTableModel::insertItemAt(int row, SharedUiItem newItem) {
  if (row < 0 || row > rowCount())
    return;
  beginInsertRows(QModelIndex(), row, row);
  _items.insert(row, newItem);
  endInsertRows();
  int toBeRemoved = _items.size() - _maxrows;
  if (toBeRemoved > 0) {
    int deletionPoint =
        (_defaultInsertionPoint == FirstItem) ? (_maxrows-toBeRemoved+1) : 0;
    beginRemoveRows(QModelIndex(), deletionPoint, deletionPoint+toBeRemoved);
    for (; toBeRemoved; --toBeRemoved) {
      //emit itemChanged(SharedUiItem(), _items.value(deletionPoint));
      _items.removeAt(deletionPoint);
    }
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

void SharedUiItemsTableModel::changeItem(SharedUiItem newItem,
                                         SharedUiItem oldItem) {
  QModelIndex oldIndex = indexOf(oldItem);
  if (newItem.isNull()) {
    if (oldIndex.isValid()) {
      // delete
      removeItems(oldIndex.row(), oldIndex.row());
    }
  } else if (oldItem.isNull() || !oldIndex.isValid()) {
    // create
    insertItemAt(_defaultInsertionPoint == FirstItem ? 0 : rowCount(), newItem);
  } else {
    // update
    _items[oldIndex.row()] = newItem;
    emit dataChanged(oldIndex, index(oldIndex.row(), columnCount()-1));
  }
  emit itemChanged(newItem, oldItem);
}

QModelIndex SharedUiItemsTableModel::indexOf(QString qualifiedId) const {
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
  return SharedUiItemsModel::flags(index)
      // add selectable flag to all items by default, some models may hold
      // unselectable (structure) items
      | Qt::ItemIsSelectable
      // add drag and drop flags to enable internal dnd
      | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

static QString suiqlMimeType { "application/shareduiitem-qualifiedid-list" };
static QString suirMimeType { "application/shareduiitem-rownum" };
static QStringList suiMimeTypes { suiqlMimeType, suirMimeType };

QMimeData *SharedUiItemsTableModel::mimeData(
    const QModelIndexList &indexes) const {
  //qDebug() << "mimeData" << indexes.size() << indexes;
  if (indexes.isEmpty())
    return 0;
  QMimeData *md = new QMimeData;
  QSet<int> rows;
  QStringList ids;
  QStringList rownums;
  foreach (const QModelIndex &index, indexes) {
    int row = index.row();
    if (!rows.contains(row)) {
      ids.append(itemAt(row).qualifiedId());
      rownums.append(QString::number(row));
      rows.insert(row);
    }
  }
  //qDebug() << "  list:" << ids << rownums;
  md->setData(suiqlMimeType, ids.join(' ').toUtf8());
  md->setData(suirMimeType, rownums.join(' ').toUtf8());
  return md;
}

Qt::DropActions SharedUiItemsTableModel::supportedDropActions() const {
  //qDebug() << "SharedUiItemsTableModel::supportedDropActions";
  return Qt::MoveAction;
}

QStringList SharedUiItemsTableModel::mimeTypes() const {
  return suiMimeTypes;
}

bool SharedUiItemsTableModel::dropMimeData(
    const QMimeData *data, Qt::DropAction action, int row, int column,
    const QModelIndex &parent) {
  if (!data)
    return false;
  if (action == Qt::MoveAction) { // moving rows through internal drag'n drop
    QList<QByteArray> idsArrays = data->data(suiqlMimeType).split(' ');
    QList<QByteArray> rownumsArrays = data->data(suirMimeType).split(' ');
    SharedUiItemList items;
    QList<int> rownums;
    if (idsArrays.size() != rownumsArrays.size()) {
      qDebug() << "SharedUiItemsTableModel::dropMimeData() received an "
                  "inconsistent drop unusable for internal move";
      return false;
    }
    for (int i = 0; i < idsArrays.size(); ++ i) {
      QString qualifiedId = QString::fromUtf8(idsArrays[i]);
      int rownum = QString::fromLatin1(rownumsArrays[i]).toInt();
      if (!qualifiedId.isEmpty() && rownum >= 0 && rownum < _items.size()
          && _items[rownum].qualifiedId() == qualifiedId) {
        items.append(_items[rownum]);
        rownums.append(rownum);
      } else {
        qDebug() << "SharedUiItemsTableModel::dropMimeData() received an "
                    "external drop unusable for internal move";
        return false;
      }
    }
    //qDebug() << "  list:" << items.join(' ', true);
    //qDebug() << "  target params:" << parent << parent.isValid() << parent.row()
    //         << row;
    int targetRow;
    // tree views will try to drop as childre of hovered item
    if (parent.isValid())
      targetRow = parent.row()+1; // move after hovered item
    // if row == -1 and !parent.isValid(), we're outside any item
    else if (row < 0)
      targetRow = rowCount(); // move at the end
    // FIXME likely that table views always give root parent
    else
      targetRow = row+1; // move after hovered item
    //qDebug() << "  target:" << targetRow << " i.e. just after:"
    //         << _items.value(targetRow-1).id();
    // remove source rows
    qSort(rownums);
    for (int i = 0; i < rownums.size(); ++i) {
      if (rownums[i] <= targetRow) {
        --targetRow; // deduce rows before target row from target row
      }
      removeRow(rownums[i]-i); // deduce already removed rows from rownum
    }
    // insert target rows
    for (int i = 0; i < items.size(); ++i)
      insertItemAt(targetRow+i, items[i]); // add already inserted rows
    if (_documentManager)
      _documentManager->reorderedItems(_items);
    return true;
  }
  return false;
}
