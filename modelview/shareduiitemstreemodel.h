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
#ifndef SHAREDUIITEMSTREEMODEL_H
#define SHAREDUIITEMSTREEMODEL_H

#include "shareduiitemsmodel.h"
#include "shareduiitem.h"
#include <QList>
#include <QSet>

/** Model holding SharedUiItems, one item per row within a tree, one item
 * section per column. */
class LIBQTSSUSHARED_EXPORT SharedUiItemsTreeModel : public SharedUiItemsModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsTreeModel)

protected:
  /** Item used by SharedUiItemsTreeModel and subclasses. */
  class LIBQTSSUSHARED_EXPORT TreeItem {
    SharedUiItemsTreeModel *_model; // kind of java non-static inner class
    SharedUiItem _item;
    int _row;
    TreeItem *_parent;
    QList<TreeItem*> _children;

  public:
    TreeItem(SharedUiItemsTreeModel *model, SharedUiItem item, TreeItem *parent,
             int row) : _model(model), _item(item), _row(row), _parent(parent) {
      QString id = item.qualifiedId();
      if (parent) {
        parent->_children.insert(row, this);
        for (++row; row < parent->_children.size(); ++row)
          _parent->_children[row]->_row = row;
      }
      if (!id.isEmpty())
        _model->_itemsIndex.insert(id, this);
    }
    ~TreeItem() {
      _model->_itemsIndex.remove(_item.qualifiedId());
      if (_parent)
        _parent->_children.removeAt(_row);
      // must not use qDeleteAll since ~child modifies its parent's _children
      foreach(TreeItem *child, _children)
        delete child;
    }
    SharedUiItem &item() { return _item; }
    int row() const { return _row; }
    TreeItem *parent() const { return _parent; }
    int childrenCount() const { return _children.size(); }
    TreeItem *child(int row) const {
      return row >= 0 && row < _children.size() ? _children[row] : 0; }
    void deleteChild(int row) { removeChild(row, true); }
    void adoptChild(TreeItem *child) {
      if (!child)
        return;
      if (child->_parent)
        child->_parent->removeChild(child->_row, false);
      child->_parent = this;
      child->_row = _children.size();
      _children.append(child);
    }
    //void dumpForDebug(QString indentation = QString(),
    //                  const TreeItem *parent = 0) const;

  private:
    void removeChild(int row, bool shouldDelete) {
      if (row >= _children.size() || row < 0)
        return;
      TreeItem *child = _children[row];
      if (shouldDelete)
        delete child; // destructor will call _children.removeAt() on its parent
      else
        _children.removeAt(row);
      for (; row < _children.size(); ++row)
        _children[row]->_row = row;
    }
  };
  friend class TreeItem;

private:
  TreeItem *_root;
  QHash<QString,TreeItem*> _itemsIndex; // key: qualified id
  QSet<QString> _changeItemQualifierFilter;

public:
  explicit SharedUiItemsTreeModel(QObject *parent = 0);
  ~SharedUiItemsTreeModel();
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &child) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  using SharedUiItemsModel::itemAt;
  SharedUiItem itemAt(const QModelIndex &index) const override;
  using SharedUiItemsModel::indexOf;
  QModelIndex indexOf(QString qualifiedId) const override;
  void changeItem(SharedUiItem newItem, SharedUiItem oldItem,
                  QString idQualifier) override;
  bool removeRows(int row, int count, const QModelIndex &parent) override;
  void insertItemAt(SharedUiItem newItem, int row,
                    QModelIndex parent = QModelIndex()) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QMimeData *mimeData(const QModelIndexList &indexes) const override;
  QStringList mimeTypes() const override;
  // LATER implement canDropMimeData() to make the user know where he can drop
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

protected:
  void clear();
  /** This method is called by changeItem() when a new item is inserted, to
   * choose where in the tree it should be added, and, when an item is updated,
   * to check wether it should change parent or not.
   * Subclasses should override this method to define where an item should be
   * attached in the tree.
   * The implementation can set the parent (default: root item) and the row
   * (default: after last row). Row == -1 means "last row for selected parent",
   * actual row will be computed by caller.
   * Limit: on update, row is currently ignored if parent does not change. */
  virtual void determineItemPlaceInTree(
      SharedUiItem newItem, QModelIndex *parent, int *row);
  /** Build a tree path string from index, e.g. "0.2.1" for second child of
   * third child of first child of root.
   * Usefull to serialize item position in tree e.g. for drag'n drop. */
  static inline QString itemPath(const QModelIndex &index);
  /** Return parent path and set rownum to rightmost index in path, if rownum
   * != 0. */
  static inline QString splitPath(QString path, int *rownum = 0);
  /** Create index knowing path string, as given by itemPath. */
  inline QModelIndex indexFromPath(QString path);

private:
  /** *item = _root if null, row = (*item)->size() if < 0 or > count() */
  inline void adjustTreeItemAndRow(TreeItem **item, int *row);
  inline void updateIndexIfIdChanged(QString newId, QString oldId,
                                     TreeItem *newTreeItem);
  inline TreeItem *treeItemByIndex(const QModelIndex &index) const;
  // hide functions that cannot work with SharedUiItem paradigm to avoid
  // misunderstanding
  using QAbstractItemModel::insertRows;
  using QAbstractItemModel::insertRow;
  using QAbstractItemModel::insertColumns;
  using QAbstractItemModel::insertColumn;
  using QAbstractItemModel::removeColumns;
  using QAbstractItemModel::removeColumn;
  //void dumpForDebug() const;
};

#endif // SHAREDUIITEMSTREEMODEL_H
