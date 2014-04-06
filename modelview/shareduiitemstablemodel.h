/* Copyright 2014 Hallowyn and others.
 * This file is part of qron, see <http://qron.hallowyn.com/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SHAREDUIITEMSTABLEMODEL_H
#define SHAREDUIITEMSTABLEMODEL_H

#include "shareduiitem.h"
#include <QAbstractTableModel>

/** Model holding AbstractUiItems, one item per line, one item section per
 * column. */
class LIBQTSSUSHARED_EXPORT SharedUiItemsTableModel
    : public QAbstractTableModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsTableModel)
  QList<SharedUiItem> _items;
  SharedUiItem _templateItem;

public:
  explicit SharedUiItemsTableModel(SharedUiItem templateItem,
                                   QObject *parent = 0);
  explicit SharedUiItemsTableModel(QObject *parent = 0);
  /** The template item is an empty item used to count columns and read column
   * headers data. */
  SharedUiItem templateItem() const { return _templateItem; }
  void setTemplateItem(SharedUiItem templateItem) {
    _templateItem = templateItem; }
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  void resetItems(QList<SharedUiItem> items);
  template <class T> void resetItems(QList<T> items) {
    // LATER try to find a more efficient cast method
    QList<SharedUiItem> castedItems;
    foreach (const SharedUiItem &i, items)
      castedItems.append(i);
    resetItems(castedItems);
  }
  void updateItem(SharedUiItem item);
  void renameItem(SharedUiItem item, QString oldId);

protected:
  SharedUiItem item(int row) const { return _items.value(row); }
};

#endif // SHAREDUIITEMSTABLEMODEL_H
