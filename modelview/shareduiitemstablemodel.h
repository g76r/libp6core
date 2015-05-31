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
#ifndef SHAREDUIITEMSTABLEMODEL_H
#define SHAREDUIITEMSTABLEMODEL_H

#include "shareduiitemsmodel.h"

// LATER provides a circular buffer implementation, in addition to QList

/** Model holding SharedUiItems, one item per line, one item section per
 * column. */
class LIBQTSSUSHARED_EXPORT SharedUiItemsTableModel
    : public SharedUiItemsModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsTableModel)

public:
  enum DefaultInsertionPoint { LastItem, FirstItem };

private:
  DefaultInsertionPoint _defaultInsertionPoint;
  int _maxrows;

protected:
  QList<SharedUiItem> _items;

public:
  explicit SharedUiItemsTableModel(QObject *parent = 0);
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &child) const;
  DefaultInsertionPoint defaultInsertionPoint() const {
    return _defaultInsertionPoint; }
  /** Set where changeItem() should add an new item.
   * Default: LastItem. */
  void setDefaultInsertionPoint(DefaultInsertionPoint defaultInsertionPoint) {
    _defaultInsertionPoint = defaultInsertionPoint; }
  int maxrows() const { return _maxrows; }
  /** Set maximum rows, when reached "older" rows will be removed by
   * changeItem() at the time a new item is inserted.
   * "Older" rows are determined as opposite sides from defaultInsertionPoint().
   * Default: INT_MAX */
  void setMaxrows(int maxrows) { _maxrows = maxrows; }
  virtual void setItems(QList<SharedUiItem> items);
  void sortAndSetItems(QList<SharedUiItem> items) {
    qSort(items);
    setItems(items);
  }
  template <class T> void setItems(QList<T> items) {
    // LATER try to find a more efficient cast method
    QList<SharedUiItem> castedItems;
    foreach (const SharedUiItem &i, items)
      castedItems.append(i);
    setItems(castedItems);
  }
  template <class T> void sortAndSetItems(QList<T> items) {
    // LATER try to find a more efficient cast method
    QList<SharedUiItem> castedItems;
    foreach (const SharedUiItem &i, items)
      castedItems.append(i);
    qSort(castedItems);
    setItems(castedItems);
  }
  /** Insert an item before row 'row', or append it at the end if
   * row == rowCount().
   * @see QAbstractItemModel::insertRow */
  virtual void insertItemAt(int row, SharedUiItem newItem);
  virtual bool removeItems(int first, int last);
  SharedUiItem itemAt(const QModelIndex &index) const;
  /** Convenience method */
  SharedUiItem itemAt(int row) const;
  using SharedUiItemsModel::indexOf;
  QModelIndex indexOf(QString qualifiedId) const;
  void changeItem(SharedUiItem newItem, SharedUiItem oldItem);
  bool removeRows(int row, int count, const QModelIndex &parent);

private:
  // TODO should remimplement these methods rather than naively hiding them
  // hide functions that would bypass our items list if they were called
  using QAbstractItemModel::insertRows;
  using QAbstractItemModel::insertRow;
};

#endif // SHAREDUIITEMSTABLEMODEL_H
