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

#include <QAbstractItemModel>
#include "shareduiitem.h"
#include "libqtssu_global.h"

class QAbstractProxyModel;

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
  //QVariant _decorationAtColumn0;

public:
  explicit SharedUiItemsModel(QObject *parent = 0);
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  /** Set header according to what template item returns.
   * Also set columns count. */
  virtual void setHeaderDataFromTemplate(SharedUiItem templateItem,
                                         int role = Qt::DisplayRole);
  /* Data returned for column 0 with Qt::DecorationRole */
  /*QVariant decorationAtColumn0() const { return _decorationAtColumn0; }
  void setDecorationAtColumn0(QVariant decoration) {
    _decorationAtColumn0 = decoration;  }*/
  virtual SharedUiItem itemAt(const QModelIndex &index) const = 0;
  QModelIndex indexOf(SharedUiItem item) const {
    return indexOf(item.qualifiedId()); }
  QModelIndex indexOf(QString idQualifier, QString id) const {
    return indexOf(SharedUiItem::qualifiedId(idQualifier, id)); }
  virtual QModelIndex indexOf(QString qualifiedId) const = 0;
  Qt::ItemFlags	flags(const QModelIndex &index) const;

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
   */
  virtual void changeItem(SharedUiItem newItem, SharedUiItem oldItem) = 0;
  /** Short for changeItem(newItem, SharedUiItem()). */
  void createItem(SharedUiItem newItem) { changeItem(newItem, SharedUiItem()); }
  /** Short for changeItem(newItem, newItem). */
  void createOrUpdateItem(SharedUiItem newItem) {
    changeItem(newItem, newItem); }
  /** Short for changeItem(SharedUiItem(), oldItem). */
  void deleteItem(SharedUiItem oldItem) { changeItem(SharedUiItem(), oldItem); }

signals:
  /** Emited by changeItem(), after it performed model changes.
   * This is a way for SharedUiItem-aware views to subscribe to changes that
   * this model also subscribe for without connecting to relevant document
   * manager signals by itself.
   * Only changeItem() emits this signal, other methods that may change items,
   * like deleteItem() don't emit this signal. */
  void itemChanged(SharedUiItem newItem, SharedUiItem oldItem);
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
  SharedUiItemsModel *realModel() const { return _realModel; }
  QModelIndex mapFromReal(QModelIndex realIndex) const;
  QModelIndex mapToReal(QModelIndex realIndex) const;
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
};

#endif // SHAREDUIITEMSMODEL_H
