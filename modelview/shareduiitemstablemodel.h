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

#include "shareduiitemsmodel.h"

/** Model holding AbstractUiItems, one item per line, one item section per
 * column. */
class LIBQTSSUSHARED_EXPORT SharedUiItemsTableModel
    : public SharedUiItemsModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsTableModel)
  QList<SharedUiItem> _items;

public:
  explicit SharedUiItemsTableModel(QObject *parent = 0);
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &child) const;
  void resetItems(QList<SharedUiItem> items);
  void sortAndResetItems(QList<SharedUiItem> items) {
    qSort(items);
    resetItems(items);
  }
  template <class T> void resetItems(QList<T> items) {
    // LATER try to find a more efficient cast method
    QList<SharedUiItem> castedItems;
    foreach (const SharedUiItem &i, items)
      castedItems.append(i);
    resetItems(castedItems);
  }
  template <class T> void sortAndResetItems(QList<T> items) {
    // LATER try to find a more efficient cast method
    QList<SharedUiItem> castedItems;
    foreach (const SharedUiItem &i, items)
      castedItems.append(i);
    qSort(castedItems);
    resetItems(castedItems);
  }
  void updateItem(SharedUiItem item);
  void renameItem(SharedUiItem item, QString oldId);

protected:
  SharedUiItem itemAt(const QModelIndex &index) const;
};

#endif // SHAREDUIITEMSTABLEMODEL_H
