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
#include <QSet>

// LATER provides a circular buffer implementation, in addition to QList

/** Model holding SharedUiItems, one item per row, one item section per
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
  QSet<QString> _changeItemQualifierFilter;

protected:
  QList<SharedUiItem> _items;

public:
  explicit SharedUiItemsTableModel(QObject *parent = 0);
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
  void insertItemAt(SharedUiItem newItem, int row,
                    QModelIndex parent = QModelIndex()) override;
  // LATER add insertItemsAt(int row, QList<SharedUiItem> newItems)
  // or even template<class T> insertItemsAt(int row, QList<T> newItems)
  virtual bool removeItems(int first, int last);
  using SharedUiItemsModel::itemAt;
  SharedUiItem itemAt(const QModelIndex &index) const override;
  SharedUiItem itemAt(int row) const { return itemAt(index(row, 0)); }
  using SharedUiItemsModel::indexOf;
  QModelIndex indexOf(QString qualifiedId) const override;
  void changeItem(SharedUiItem newItem, SharedUiItem oldItem,
                  QString idQualifier) override;
  bool removeRows(int row, int count,
                  const QModelIndex &parent = QModelIndex()) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QMimeData *mimeData(const QModelIndexList &indexes) const override;
  QStringList mimeTypes() const override;
  bool dropMimeData(
      const QMimeData *data, Qt::DropAction action, int targetRow,
      int targetColumn, const QModelIndex &droppedParent) override;
  void setChangeItemQualifierFilter(QSet<QString> acceptedQualifiers) {
    _changeItemQualifierFilter = acceptedQualifiers; }
  void setChangeItemQualifierFilter(QList<QString> acceptedQualifiers) {
    _changeItemQualifierFilter = QSet<QString>::fromList(acceptedQualifiers); }
  void setChangeItemQualifierFilter(QString acceptedQualifier) {
    _changeItemQualifierFilter.clear();
    _changeItemQualifierFilter.insert(acceptedQualifier); }
  void clearChangeItemQualifierFilter() {
    _changeItemQualifierFilter.clear(); }

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
