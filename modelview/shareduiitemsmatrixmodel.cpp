/* Copyright 2015-2023 Hallowyn, Gregoire Barbier and others.
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
#include "shareduiitemsmatrixmodel.h"
#include "util/paramset.h"
#include "shareduiitemdocumentmanager.h"
#include "modelview/genericshareduiitem.h"

SharedUiItemsMatrixModel::ItemBinding SharedUiItemsMatrixModel::nullBinding;

SharedUiItemsMatrixModel::SharedUiItemsMatrixModel(QObject *parent)
  : SharedUiItemsModel(parent) {
}

QModelIndex SharedUiItemsMatrixModel::index(
    int row, int column, const QModelIndex &parent) const {
  if (row >= 0 && row < _rowsCount && column >= 0 && column < _columnsCount
      && !parent.isValid())
    return createIndex(row, column);
  return QModelIndex();
}

QModelIndex SharedUiItemsMatrixModel::parent(const QModelIndex &child) const {
  Q_UNUSED(child)
  return QModelIndex();
}

int SharedUiItemsMatrixModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : _cells.size();
}

int SharedUiItemsMatrixModel::columnCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : _columnsCount;
}

SharedUiItem SharedUiItemsMatrixModel::itemAt(const QModelIndex &index) const {
  return cellBindingAt(index)._item;
}

const SharedUiItemsMatrixModel::ItemBinding &
SharedUiItemsMatrixModel::cellBindingAt(const QModelIndex &index) const {
  if (index.isValid() && !index.parent().isValid()
      && index.row() >= 0 && index.row() < _cells.size()
      && index.column() >= 0 && index.column() < _cells[index.row()].size()) {
    return _cells[index.row()][index.column()];
  }
  return nullBinding;
}

QModelIndex SharedUiItemsMatrixModel::indexOf(QByteArray qualifiedId) const {
  GenericSharedUiItem oldItem(qualifiedId);
  for (int i = 0; i < _cells.size(); ++i) {
    const QList<ItemBinding> &row = _cells[i];
    for (int j = 0; j < row.size(); ++j) {
      const ItemBinding &binding = row[j];
      if (binding._item == oldItem) {
        return index(i, j);
      }
    }
  }
  return QModelIndex();
}

void SharedUiItemsMatrixModel::insertItemAt(
    SharedUiItem newItem, int row, QModelIndex parent) {
  Q_UNUSED(newItem)
  Q_UNUSED(row)
  Q_UNUSED(parent)
  qWarning() << "SharedUiItemsMatrixModel::insertItemAt() called whereas it has"
                " no meaning. Do nothing.";
}

void SharedUiItemsMatrixModel::changeItem(
    SharedUiItem newItem, SharedUiItem oldItem, QByteArray idQualifier) {
  Q_UNUSED(idQualifier)
  //qDebug() << "SharedUiItemsMatrixModel::changeItem" << newItem << oldItem
  //         << idQualifier;
  if (oldItem.isNull())
    return;
  for (int i = 0; i < _verticalHeaders.size(); ++i) {
    ItemBinding &binding = _verticalHeaders[i];
    if (binding._item == oldItem) {
      binding._item = newItem;
      //qDebug() << "  updated vertical header" << i;
      emit headerDataChanged(Qt::Vertical, i, i);
    }
  }
  for (int i = 0; i < _horizontalHeaders.size(); ++i) {
    ItemBinding &binding = _horizontalHeaders[i];
    if (binding._item == oldItem) {
      binding._item = newItem;
      //qDebug() << "  updated horizontal header" << i;
      emit headerDataChanged(Qt::Horizontal, i, i);
    }
  }
  for (int i = 0; i < _cells.size(); ++i) {
    QList<ItemBinding> &row = _cells[i];
    for (int j = 0; j < row.size(); ++j) {
      ItemBinding &binding = row[j];
      if (binding._item == oldItem) {
        binding._item = newItem;
        QModelIndex topLeft = index(i, j);
        //qDebug() << "  updated cell" << i << j;
        emit dataChanged(topLeft, topLeft);
      }
    }
  }
}

void SharedUiItemsMatrixModel::bindHeader(
    int section, Qt::Orientation orientation, SharedUiItem item,
    QString display, QString tooltip) {
  QList<ItemBinding> *headers;
  switch (orientation) {
  case Qt::Horizontal:
    headers = &_horizontalHeaders;
    if (headers->size() < section+1) {
      beginInsertColumns(QModelIndex(), headers->size(), section);
      headers->resize(section+1);
      _columnsCount = qMax(_columnsCount, headers->size());
      endInsertColumns();
    }
    break;
  case Qt::Vertical:
    headers = &_verticalHeaders;
    if (headers->size() < section+1) {
      beginInsertRows(QModelIndex(), headers->size(), section);
      headers->resize(section+1);
      _rowsCount = qMax(_rowsCount, headers->size());
      endInsertRows();
    }
    break;
  default:
    qWarning() << "SharedUiItemsMatrixModel::bindHeader() called with unknown "
                  "orientation:" << orientation;
    return;
  }
  SharedUiItem oldItem = (*headers)[section]._item;
  (*headers)[section] = ItemBinding(item, display, tooltip);
  emit headerBinded(section, orientation, item, oldItem, display, tooltip);
  emit headerDataChanged(orientation, section, section);
}

void SharedUiItemsMatrixModel::bindCell(
    int row, int column, SharedUiItem item, QString display, QString tooltip,
    int editableSection) {
  if (_cells.size() < row+1) {
    beginInsertRows(QModelIndex(), _cells.size(), row);
    _cells.resize(row+1);
    _rowsCount = qMax(_rowsCount, _cells.size());
    endInsertRows();
  }
  if (_cells[row].size() < column+1) {
    beginInsertColumns(QModelIndex(), _cells[row].size(), column);
    _cells[row].resize(column+1);
    _columnsCount = qMax(_columnsCount, _cells[row].size());
    endInsertColumns();
  }
  SharedUiItem oldItem = _cells[row][column]._item;
  _cells[row][column] = ItemBinding(item, display, tooltip, editableSection);
  QModelIndex i = index(row, column);
  emit cellBinded(row, column, item, oldItem, display, tooltip,
                  editableSection);
  emit dataChanged(i, i);
}

void SharedUiItemsMatrixModel::clearBindings() {
  // TODO method name is misleading
  beginResetModel();
  _cells.clear();
  _horizontalHeaders.clear();
  _verticalHeaders.clear();
  _rowsCount = _columnsCount = 0;
  endResetModel();
}

QVariant SharedUiItemsMatrixModel::evaluate(
    SharedUiItemsMatrixModel::ItemBinding binding, int role) const {
  int evaluationRole = role;
  if (_forceDisplayRoleWhenEvaluatingTooltips && role == Qt::ToolTipRole)
    evaluationRole = Qt::DisplayRole;
  // TODO make again a way to choose role when using SUI as a PP
  //SharedUiItemParamsProvider pp(binding._item, evaluationRole);
  switch(role) {
  case Qt::DisplayRole:
  case SharedUiItem::ExternalDataRole:
    return ParamSet().evaluate(binding._display, false, &binding._item);
  case Qt::ToolTipRole:
    return ParamSet().evaluate(binding._tooltip, false, &binding._item);
  case Qt::EditRole:
  case SharedUiItem::IdQualifierRole:
  case SharedUiItem::IdRole:
  case SharedUiItem::QualifiedIdRole:
    return binding._item.uiData(binding._editableSection, role);
  }
  return QVariant();
}

QVariant SharedUiItemsMatrixModel::data(
    const QModelIndex &index, int role) const {
  if (index.isValid() && !index.parent().isValid()
      && index.row() >= 0 && index.row() < _cells.size()
      && index.column() >= 0 && index.column() < _cells[index.row()].size()) {
    return evaluate(_cells[index.row()][index.column()], role);
  }
  return QVariant();
}

bool SharedUiItemsMatrixModel::setData(
    const QModelIndex &index, const QVariant &value, int role) {
  //qDebug() << "SharedUiItemsMatrixModel::setData 1" << index << value << role
  //         << _documentManager << index.isValid() << index.parent().isValid()
  //         << index.row() << "/" << _cells.size() << index.column();
  if (role != Qt::EditRole || !index.isValid() || !_documentManager)
    return false; // cannot set data
  if (index.parent().isValid() || index.row() < 0
      || index.row() >= _cells.size() || index.column() < 0
      || index.column() >= _cells[index.row()].size())
    return false; // no binding here
  const ItemBinding &binding = _cells[index.row()][index.column()];
  if (binding._item.isNull())
    return false; // empty binding here
  if (binding._item.uiData(binding._editableSection, role) == value)
    return true; // nothing to do, this is already current value
  //qDebug() << "SharedUiItemsMatrixModel::setData" << index << value << role
  //         << binding._item << binding._editableSection;
  return _documentManager->changeItemByUiData(
        binding._item, binding._editableSection, value);
}

QVariant SharedUiItemsMatrixModel::headerData(
    int section, Qt::Orientation orientation, int role) const {
  const QList<ItemBinding> &headers =
      (orientation == Qt::Horizontal) ? _horizontalHeaders : _verticalHeaders;
  if (section >= 0 && section < headers.size())
    return evaluate(headers[section], role);
  return QVariant();
}

Qt::ItemFlags SharedUiItemsMatrixModel::flags(const QModelIndex &index) const {
  Qt::ItemFlags flags = Qt::NoItemFlags;
  if (index.isValid() && !index.parent().isValid()
      && index.row() >= 0 && index.row() < _cells.size()
      && index.column() >= 0 && index.column() < _cells[index.row()].size()) {
    flags |= Qt::ItemNeverHasChildren;
    const ItemBinding &binding = _cells[index.row()][index.column()];
    if (!binding._item.isNull())
      flags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    // LATER not sure checking _item.uiFlags() is needed
    if (binding._editableSection >= 0
        && binding._item.uiFlags(binding._editableSection) & Qt::ItemIsEditable)
      flags |= Qt::ItemIsEditable;
  }
  return flags;
}

QHash<int, QByteArray> SharedUiItemsMatrixModel::roleNames() const {
  // shortcut SharedUiItemsModel's overriding
  return QAbstractItemModel::roleNames();
}

void SharedUiItemsMatrixModel::setHeaderDataFromTemplate(
    SharedUiItem templateItem, int role) {
  Q_UNUSED(templateItem)
  Q_UNUSED(role)
  qWarning() << "SharedUiItemsMatrixModel::setHeaderDataFromTemplate() called "
                "whereas it's non-sense for a matrix model.";
}
