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
#ifndef SHAREDUIITEMSMODEL_H
#define SHAREDUIITEMSMODEL_H

#include <QAbstractProxyModel>
#include "shareduiitem.h"
#include "libqtssu_global.h"
#include <QString>

class SharedUiItemDocumentManager;

/** Base class for model holding SharedUiItems, being them table or
 * tree-oriented they provides one item section per column.
 *
 * Beware that, like any QAbstractItemModel, this class is not thread-safe
 * and that most of its methods must never be called from another thread than
 * the main thread. If triggered from another thread, changeItem() must be
 * called through signal/slot connection or using QMetaObject::invokeMethod().
 * If data has to be read from another thread, this must be done through a
 * thread-safe view which is connected to this model and offer thread-safe read
 * methods, like TextTableView.
 */
class LIBQTSSUSHARED_EXPORT SharedUiItemsModel : public QAbstractItemModel {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemsModel)
  int _columnsCount;
  QHash<int,QHash<int,QVariant> > _mapRoleSectionHeader;

protected:
  SharedUiItemDocumentManager *_documentManager;
  /** Mime type for space-separated list of qualified ids, for drag'n drop */
  static const QString suiQualifiedIdsListMimeType;
  /** Mime type for space-separated list of places of item, for drag'n drop,
   * especially internal drag'n drop used to reorder items.
   * In same order than qualified ids list. Place can be e.g. rownum for table
   * models or paths to id for tree models. */
  static const QString suiPlacesMimeType;
  /** List of previous mime types. */
  static const QStringList suiMimeTypes;

public:
  explicit SharedUiItemsModel(QObject *parent = 0);
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  /** Set header according to what template item returns.
   * Also set columns count. */
  virtual void setHeaderDataFromTemplate(SharedUiItem templateItem,
                                         int role = Qt::DisplayRole);
  virtual SharedUiItem itemAt(const QModelIndex &index) const = 0;
  inline SharedUiItem itemAt(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const {
    return itemAt(index(row, column, parent));
  }
  /** Convenience template performing downcast. */
  template<class T>
  inline T itemAt(QString qualifier, const QModelIndex &index) const {
    SharedUiItem item = itemAt(index);
    return item.idQualifier() == qualifier ? static_cast<T&>(item) : T();
  }
  /** Convenience template performing downcast. */
  template<class T>
  inline T itemAt(const char *qualifier, const QModelIndex &index) const {
    SharedUiItem item = itemAt(index);
    return item.idQualifier() == qualifier ? static_cast<T&>(item) : T();
  }
  /** Convenience template performing downcast. */
  template<class T>
  inline T itemAt(QString qualifier, int row, int column,
           const QModelIndex &parent = QModelIndex()) const {
    SharedUiItem item = itemAt(row, column, parent);
    return item.idQualifier() == qualifier ? static_cast<T&>(item) : T();
  }
  /** Convenience template performing downcast. */
  template<class T>
  inline T itemAt(const char *qualifier, int row, int column,
           const QModelIndex &parent = QModelIndex()) const {
    SharedUiItem item = itemAt(row, column, parent);
    return item.idQualifier() == qualifier ? static_cast<T&>(item) : T();
  }
  QModelIndex indexOf(SharedUiItem item) const {
    return indexOf(item.qualifiedId()); }
  QModelIndex indexOf(QString idQualifier, QString id) const {
    return indexOf(SharedUiItem::qualifiedId(idQualifier, id)); }
  virtual QModelIndex indexOf(QString qualifiedId) const = 0;
  Qt::ItemFlags	flags(const QModelIndex &index) const;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole);
  /** Insert an item before row 'row', or append it at the end if
   * row == rowCount().
   * @see QAbstractItemModel::insertRow */
  virtual void insertItemAt(SharedUiItem newItem, int row,
                            QModelIndex parent = QModelIndex()) = 0;
  Qt::DropActions supportedDropActions() const override;
  SharedUiItemDocumentManager *documentManager() const {
    return _documentManager; }
  /** Set document manager and populate model with its items matching qualifier
   * ids in idQualifiers. */
  virtual void setDocumentManager(SharedUiItemDocumentManager *documentManager,
                                  QStringList idQualifiers = QStringList());

public slots:
  /** Operate a change on an item within this model.
   *
   * If oldItem is found in the model and newItem.isNull(), remove old item from
   * the model.
   * If oldItem is found in the model and newItem is not null, update the item,
   * taking care that old and new item ids may be different is the item is
   * being renamed.
   * If oldItem is not found in the model and new item is not null, create it,
   * regardless oldItem is null or garbage or equals to newItem.
   *
   * If relevant this method can filter which items are actually handled or not,
   * for example depending on newItem.idQualifier(). If so, some of the calls to
   * changeItem() may be ignored.
   *
   * This slot can be connected to SharedUiItemDocumentManager::itemChanged()
   * signal, or any more precise signals (kind of
   * FoobarDocumentManager::foobarItemChanged(Foobar newFoobar,
   * Foobar oldFoobar)).
   *
   * Must emit itemChanged() after having updated data.
   * @see SharedUiItemDocumentManager::itemChanged()
   * @see SharedUiItemDocumentManager::changeItem()
   */
  virtual void changeItem(SharedUiItem newItem, SharedUiItem oldItem,
                          QString idQualifier) = 0;
  /** Short for changeItem(newItem, SharedUiItem(), newItem.idQualifier()). */
  void createOrUpdateItem(SharedUiItem newItem) {
    changeItem(newItem, SharedUiItem(), newItem.idQualifier()); }
  /** Short for changeItem(SharedUiItem(), oldItem, oldItem.idQualifier()). */
  void deleteItemIfExists(SharedUiItem oldItem) {
    changeItem(SharedUiItem(), oldItem, oldItem.idQualifier()); }

signals:
  /** Emited by changeItem(), after it performed model changes.
   * This is a way for SharedUiItem-aware views to subscribe to changes that
   * this model also subscribe for without connecting to relevant document
   * manager signals by itself.
   * Only changeItem() emits this signal, other methods that may change items,
   * like removeRows() don't emit this signal. */
  void itemChanged(SharedUiItem newItem, SharedUiItem oldItem);

protected:
  /** Move children rows just before a given target row.
   * Generic method callable by model implementations, especially to
   * implement rows reordering.
   * @param parent common parent of rows to be moved
   * @param sourceRows rows to be moved
   * @param targetRow row before which to move,
   *   0 <= targetRow <= rowCount(parent) */
  void moveRowsByRownums(QModelIndex parent, QList<int> sourceRows,
                         int targetRow);
};

/** Helper class to access a SharedUiItemsModel and its specific methods
 * through a several QAbstractProxyModel.
 * This class is needed because QAbstractProxyModel maps standard Qt features
 * but not specific SharedUiItem ones (such as indexOf(SharedUiItem) or signal
 * itemChanged()).
 */
class LIBQTSSUSHARED_EXPORT SharedUiItemsProxyModelHelper {
  SharedUiItemsModel *_realModel;
  QList <QAbstractProxyModel*> _proxies;

public:
  SharedUiItemsProxyModelHelper() : _realModel(0) { }
  SharedUiItemsProxyModelHelper(QAbstractItemModel *model) {
    setApparentModel(model); }
  void setApparentModel(QAbstractItemModel *model);
  QAbstractItemModel *apparentModel() const {
    if (_proxies.isEmpty())
      return _realModel;
    return _proxies.last();
  }
  /** Null if apparent model not set or not proxying a SharedUiItemsModel. */
  SharedUiItemsModel *realModel() const { return _realModel; }
  /** Equivalent to realModel() != 0. */
  bool isValid() const { return _realModel; }
  QModelIndex mapFromReal(QModelIndex realIndex) const;
  QModelIndex mapToReal(QModelIndex apparentIndex) const;
  QModelIndex indexOf(SharedUiItem item) const {
    return _realModel ? mapFromReal(_realModel->indexOf(item))
                      : QModelIndex(); }
  QModelIndex indexOf(QString idQualifier, QString id) const {
    return _realModel ? mapFromReal(_realModel->indexOf(idQualifier, id))
                      : QModelIndex(); }
  QModelIndex indexOf(QString qualifiedId) const {
    return _realModel ? mapFromReal(_realModel->indexOf(qualifiedId))
                      : QModelIndex(); }
  SharedUiItem itemAt(const QModelIndex &index) const {
    return _realModel ? _realModel->itemAt(mapToReal(index)) : SharedUiItem(); }
  SharedUiItem itemAt(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const {
    if (!_realModel)
      return SharedUiItem();
    if (_proxies.isEmpty())
      return _realModel->itemAt(row, column, parent);
    return _realModel->itemAt(mapToReal(apparentModel()->index(row, column,
                                                               parent)));
  }
  /** Convenience template performing downcast. */
  template<class T>
  inline T itemAt(QString qualifier, const QModelIndex &index) const {
    SharedUiItem item = itemAt(index);
    return item.idQualifier() == qualifier ? static_cast<T&>(item) : T();
  }
  /** Convenience template performing downcast. */
  template<class T>
  inline T itemAt(const char *qualifier, const QModelIndex &index) const {
    SharedUiItem item = itemAt(index);
    return item.idQualifier() == qualifier ? static_cast<T&>(item) : T();
  }
  /** Convenience template performing downcast. */
  template<class T>
  inline T itemAt(QString qualifier, int row, int column,
           const QModelIndex &parent = QModelIndex()) const {
    SharedUiItem item = itemAt(row, column, parent);
    return item.idQualifier() == qualifier ? static_cast<T&>(item) : T();
  }
  /** Convenience template performing downcast. */
  template<class T>
  inline T itemAt(const char *qualifier, int row, int column,
           const QModelIndex &parent = QModelIndex()) const {
    SharedUiItem item = itemAt(row, column, parent);
    return item.idQualifier() == qualifier ? static_cast<T&>(item) : T();
  }
};

#endif // SHAREDUIITEMSMODEL_H
