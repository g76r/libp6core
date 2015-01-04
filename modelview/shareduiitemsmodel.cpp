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
#include "shareduiitemsmodel.h"
#include <QtDebug>
#include <QAbstractProxyModel>

SharedUiItemsModel::SharedUiItemsModel(QObject *parent)
  : QAbstractItemModel(parent), _columnsCount(0) {
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
    const SharedUiItem &templateItem, int role) {
  _columnsCount = templateItem.uiSectionCount();
  QHash<int,QVariant> mapSectionHeader;
  for (int section = 0; section < _columnsCount; ++section)
    mapSectionHeader.insert(section, templateItem.uiHeaderData(section, role));
  _mapRoleSectionHeader.insert(role, mapSectionHeader);
}

Qt::ItemFlags	SharedUiItemsModel::flags(const QModelIndex & index) const {
  // LATER have an orientation parameter, do not assume item section == column
  return itemAt(index).uiFlags(index.column());
}
