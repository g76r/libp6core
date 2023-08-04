/* Copyright 2014-2023 Hallowyn, Gregoire Barbier and others.
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
#include "shareduiitemsmodel.h"
#include <QtDebug>
#include <QAbstractProxyModel>
#include "shareduiitemdocumentmanager.h"
#include "shareduiitemlist.h"
#include <QtDebug>

const QString SharedUiItemsModel::_suiQualifiedIdsListMimeType {
  "application/shareduiitem-qualifiedid-list"
};

const QString SharedUiItemsModel::_suiPlacesMimeType {
  "application/shareduiitem-places"
};

const QStringList SharedUiItemsModel::_suiMimeTypes {
  SharedUiItemsModel::_suiQualifiedIdsListMimeType,
  SharedUiItemsModel::_suiPlacesMimeType
};

SharedUiItemsModel::SharedUiItemsModel(QObject *parent)
  : QAbstractItemModel(parent), _columnsCount(0),
    _roleNames(QAbstractItemModel::roleNames()), _documentManager(0) {
}

int SharedUiItemsModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  // LATER have an orientation parameter, do not assume item section == column
  return _columnsCount;
}

QVariant SharedUiItemsModel::data(const QModelIndex &index, int role) const {
  // LATER have an orientation parameter, do not assume item section == column
  if (index.isValid()) {
    /*if (role == Qt::DecorationRole && index.column() == 0
        && !_decorationAtColumn0.isNull())
      return _decorationAtColumn0;*/
    return itemAt(index).uiData(index.column(), role);
  }
  return QVariant();
}

QVariant SharedUiItemsModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const {
  // LATER have an orientation parameter, do not assume item section == column
  return orientation == Qt::Horizontal && _mapRoleSectionHeader.contains(role)
      ? _mapRoleSectionHeader.value(role).value(section) : QVariant();
}

void SharedUiItemsModel::setHeaderDataFromTemplate(
    SharedUiItem templateItem, int role) {
  _columnsCount = templateItem.uiSectionCount();
  QHash<int,QVariant> mapSectionHeader;
  _roleNames = QAbstractItemModel::roleNames();
  for (int section = 0; section < _columnsCount; ++section) {
    mapSectionHeader.insert(section, templateItem.uiHeaderData(section, role));
    _roleNames.insert(section, templateItem.uiSectionName(section));
  }
  _mapRoleSectionHeader.insert(role, mapSectionHeader);
}

Qt::ItemFlags SharedUiItemsModel::flags(const QModelIndex & index) const {
  // LATER have an orientation parameter, do not assume item section == column
  // LATER modify flags on the fly if dm is not set ?
  return itemAt(index).uiFlags(index.column());
}

bool SharedUiItemsModel::setData(
    const QModelIndex &index, const QVariant &value, int role) {
  //qDebug() << "SharedUiItemsModel::setData index:" << index << "value:"
  //         << value << "role:" << role << "model(this):" << this
  //         << "dm:" << _documentManager;
  SharedUiItem oldItem = itemAt(index);
  //qDebug() << "oldItem:" << oldItem.qualifiedId();
  if (role != Qt::EditRole || !index.isValid() || oldItem.isNull()
      || !_documentManager) // cannot set data
    return false;
  if (oldItem.uiData(index.column(), role) == value) // nothing to do
    return true;
  return _documentManager->changeItemByUiData(oldItem, index.column(), value);
}

void SharedUiItemsModel::moveRowsByRownums(
    QModelIndex parent, QList<int> sourceRows, int targetRow) {
  SharedUiItemList<> items;
  for (int rownum : sourceRows)
    items.append(itemAt(index(rownum, 0, parent)));
  //qDebug() << "  list:" << items.join(' ', true);
  //qDebug() << "  target params:" << parent << parent.isValid()
  //         << parent.row() << targetRow << rowCount(parent);
  if (targetRow < 0 || targetRow > rowCount(parent)) {
    qDebug() << "SharedUiItemsModel::moveRowsByRownums: targetRownum out of "
                "bounds:" << targetRow;
    return;
  }
  //qDebug() << "  target:" << targetRow << " i.e. just after:"
  //         << parent.child(targetRow-1, 0).data()
  //         << "under:" << parent.data();
  // remove source rows
  std::sort(sourceRows.begin(), sourceRows.end());
  int rowsBeforeTarget = 0;
  for (int i = 0; i < sourceRows.size(); ++i) {
    if (sourceRows[i] < targetRow) {
      ++rowsBeforeTarget;
    }
    removeRow(sourceRows[i]-i, // deduce already removed rows from rownum
              parent);
  }
  targetRow -= rowsBeforeTarget; // deduce rows before target row
  //qDebug() << "  target (after remove):" << targetRow << rowCount(parent);
  // insert target rows
  for (int i = 0; i < items.size(); ++i) {
    // add already inserted rows to target rownum
    insertItemAt(items[i], targetRow+i, parent);
  }
  if (_documentManager)
    _documentManager->reorderItems(items);
}

Qt::DropActions SharedUiItemsModel::supportedDropActions() const {
  // support MoveAction in addition to CopyAction to make drag'n drop eordering
  // work for views in InternalMove mode, since when in InternalMove, the view
  // will force MoveAction if supported and do nothing if not supported,
  // regardless its default action
  return Qt::CopyAction | Qt::MoveAction;
}

void SharedUiItemsModel::setDocumentManager(
    SharedUiItemDocumentManager *documentManager) {
  if (_documentManager)
    disconnect(_documentManager, 0, this, 0);
  _documentManager = documentManager;
  resetData();
  if (_documentManager) {
    connect(_documentManager, &SharedUiItemDocumentManager::itemChanged,
            this, &SharedUiItemsModel::changeItem);
    connect(_documentManager, &SharedUiItemDocumentManager::dataReset,
            this, &SharedUiItemsModel::resetData);
  }
}

void SharedUiItemsModel::resetData() {
  int rows = rowCount();
  if (rows)
    removeRows(0, rows);
  if (_documentManager) {
    // LATER also populate data if _itemQualifierFilter is empty
    for (auto idQualifier : _itemQualifierFilter) {
      for (const SharedUiItem &item
           : _documentManager->itemsByIdQualifier(idQualifier)) {
        createOrUpdateItem(item);
      }
    }
  }
  emit dataReset();
}

void SharedUiItemsProxyModelHelper::setApparentModel(
    QAbstractItemModel *model) {
  _realModel = qobject_cast<SharedUiItemsModel*>(model);
  _proxies.clear();
  if (!_realModel) {
    QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel*>(model);
    while (proxy && !_realModel) {
      _proxies.prepend(proxy);
      _realModel = qobject_cast<SharedUiItemsModel*>(proxy->sourceModel());
      proxy = qobject_cast<QAbstractProxyModel*>(proxy->sourceModel());
    }
  }
}

QModelIndex SharedUiItemsProxyModelHelper::mapFromReal(
    QModelIndex realIndex) const {
  foreach(QAbstractProxyModel *proxy, _proxies)
    realIndex = proxy->mapFromSource(realIndex);
  return realIndex;
}

QModelIndex SharedUiItemsProxyModelHelper::mapToReal(
    QModelIndex apparentIndex) const {
  for (int i = _proxies.size()-1; i >= 0; --i)
    apparentIndex = _proxies[i]->mapToSource(apparentIndex);
  return apparentIndex;
}

QHash<int,QByteArray> SharedUiItemsModel::roleNames() const {
  return _roleNames;
}

SharedUiItem SharedUiItemsModel::itemAt(const QModelIndex &) const {
  return SharedUiItem();
}

QModelIndex SharedUiItemsModel::indexOf(QByteArray) const {
  return QModelIndex();
}

void SharedUiItemsModel::insertItemAt(SharedUiItem, int, QModelIndex) {
}

void SharedUiItemsModel::changeItem(SharedUiItem, SharedUiItem, QByteArray) {
}
