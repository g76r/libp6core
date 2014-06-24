#include "shareduiitemstreemodel.h"

SharedUiItemsTreeModel::SharedUiItemsTreeModel(QObject *parent)
  : SharedUiItemsModel(parent) {
}

SharedUiItemsTreeModel::~SharedUiItemsTreeModel() {
  qDeleteAll(_roots);
}

QModelIndex SharedUiItemsTreeModel::index(int row, int column,
                                    const QModelIndex &parent) const {
  // note: hasIndex() checks row and columns boundaries by calling rowCount()
  // and columnCount() for parent, therefore also indirectly checking that
  // parent.internalPointer() is not null
  //qDebug() << "GroupsTasksModel::index" << row << column << parent.isValid()
  //         << hasIndex(row, column, parent) << parent.parent().isValid();
  if (hasIndex(row, column, parent))
    return parent.isValid()
        ? createIndex(row, column,
                      ((SharedUiItemTreeItem*)parent.internalPointer())
                      ->child(row))
        : createIndex(row, column, _roots.value(row));
  return QModelIndex();
}

QModelIndex SharedUiItemsTreeModel::parent(const QModelIndex &child) const {
  if (child.isValid()) {
    SharedUiItemTreeItem *c = ((SharedUiItemTreeItem*)child.internalPointer());
    if (c && c->parent())
      return createIndex(c->row(), 0, c->parent());
  }
  return QModelIndex();
}

int SharedUiItemsTreeModel::rowCount(const QModelIndex &parent) const {
  //qDebug() << "rowCount:" << parent.isValid() << parent.parent().isValid();
  if (parent.isValid()) {
    SharedUiItemTreeItem *p = ((SharedUiItemTreeItem*)parent.internalPointer());
    if (p) { // null p is theorically impossible
      //qDebug() << "->" << p->childrenCount();
      return p->childrenCount();
    }
  } else {
    //qDebug() << "->" << _roots.size();
    return _roots.size();
  }
  return 0;
}

SharedUiItem SharedUiItemsTreeModel::itemAt(const QModelIndex &index) const {
  if (index.isValid()) {
    SharedUiItemTreeItem *i = ((SharedUiItemTreeItem*)index.internalPointer());
    if (i)
      return i->item();
  }
  return SharedUiItem();
}

void SharedUiItemsTreeModel::setRoots(QList<SharedUiItemTreeItem*> roots) {
  // TODO rather determine what items must be deleted and what was kept
  if (_roots != roots) {
    if (!_roots.isEmpty()) {
      beginRemoveRows(QModelIndex(), 0, _roots.size()-1);
      qDeleteAll(_roots);
      _roots.clear();
      endRemoveRows();
    }
    if (!roots.isEmpty()) {
      beginInsertRows(QModelIndex(), 0, roots.size()-1);
      _roots = roots;
      endInsertRows();
    }
  }
}
