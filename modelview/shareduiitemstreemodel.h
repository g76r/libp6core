#ifndef SHAREDUIITEMSTREEMODEL_H
#define SHAREDUIITEMSTREEMODEL_H

#include "shareduiitemtreeitem.h"
#include "shareduiitemsmodel.h"

/** Model holding AbstractUiItems, one item per line within a tree, one item
 * section per column. */
class LIBQTSSUSHARED_EXPORT SharedUiItemsTreeModel : public SharedUiItemsModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsTreeModel)
  QList<SharedUiItemTreeItem*> _roots;

public:
  explicit SharedUiItemsTreeModel(QObject *parent = 0);
  ~SharedUiItemsTreeModel();
  QModelIndex index(int row, int column, const QModelIndex &parent) const;
  QModelIndex parent(const QModelIndex &child) const;
  int rowCount(const QModelIndex &parent) const;
  SharedUiItem itemAt(const QModelIndex &index) const;
  QList<SharedUiItemTreeItem*> roots() const { return _roots; }
  void setRoots(QList<SharedUiItemTreeItem*> roots);
  void clearRoots() { setRoots(QList<SharedUiItemTreeItem*>()); }
};

#endif // SHAREDUIITEMSTREEMODEL_H
