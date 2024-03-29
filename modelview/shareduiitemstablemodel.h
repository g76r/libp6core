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
#ifndef SHAREDUIITEMSTABLEMODEL_H
#define SHAREDUIITEMSTABLEMODEL_H

#include "shareduiitemsmodel.h"
#include "modelview/shareduiitemlist.h"

// LATER provides a circular buffer implementation, in addition to QList

/** Model holding SharedUiItems, one item per row, one item section per
 * column. */
class LIBP6CORESHARED_EXPORT SharedUiItemsTableModel
    : public SharedUiItemsModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsTableModel)

public:
  enum DefaultInsertionPoint { LastItem, FirstItem };

private:
  DefaultInsertionPoint _defaultInsertionPoint;
  int _maxrows;

protected:
  SharedUiItemList _items;

public:
  explicit SharedUiItemsTableModel(QObject *parent = 0);
  explicit SharedUiItemsTableModel(
      SharedUiItem templateItem,
      DefaultInsertionPoint defaultInsertionPoint = LastItem,
      QObject *parent = 0);
  explicit SharedUiItemsTableModel(SharedUiItem templateItem, QObject *parent)
    : SharedUiItemsTableModel(templateItem, LastItem, parent) { }
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &child) const override;
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
  void sortAndSetItems(const SharedUiItemList &items) {
    setItems(items.sorted()); }
  void insertItemAt(const SharedUiItem &newItem, int row,
                    const QModelIndex &parent = {}) override;
  // LATER add insertItemsAt(int row, QList<SharedUiItem> newItems)
  // or even template<class T> insertItemsAt(int row, QList<T> newItems)
  virtual bool removeItems(int first, int last);
  using SharedUiItemsModel::itemAt;
  SharedUiItem itemAt(const QModelIndex &index) const override;
  SharedUiItem itemAt(int row) const { return itemAt(index(row, 0)); }
  using SharedUiItemsModel::indexOf;
  QModelIndex indexOf(const Utf8String &qualifiedId) const override;
  void changeItem(const SharedUiItem &newItem, const SharedUiItem &oldItem,
                  const Utf8String &qualifier) override;
  bool removeRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QMimeData *mimeData(const QModelIndexList &indexes) const override;
  QStringList mimeTypes() const override;
  bool dropMimeData(
      const QMimeData *data, Qt::DropAction action, int targetRow,
      int targetColumn, const QModelIndex &droppedParent) override;
  SharedUiItemList items() const { return _items; }

public slots:
  virtual void setItems(const SharedUiItemList &items);

private:
  // hide functions that cannot work with SharedUiItem paradigm to avoid
  // misunderstanding
  using QAbstractItemModel::insertRows;
  using QAbstractItemModel::insertRow;
  using QAbstractItemModel::insertColumns;
  using QAbstractItemModel::insertColumn;
  using QAbstractItemModel::removeColumns;
  using QAbstractItemModel::removeColumn;
};

#endif // SHAREDUIITEMSTABLEMODEL_H
