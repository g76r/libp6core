/* Copyright 2015 Hallowyn and others.
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
#ifndef SHAREDUIITEMDOCUMENTMANAGER_H
#define SHAREDUIITEMDOCUMENTMANAGER_H

#include <QObject>
#include "libqtssu_global.h"
#include "modelview/shareduiitem.h"
#include "shareduiitemlist.h"

/** Base class for SharedUiItem-based document managers.
 * @see SharedUiItem */
class LIBQTSSUSHARED_EXPORT SharedUiItemDocumentManager : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemDocumentManager)

public:
  explicit SharedUiItemDocumentManager(QObject *parent = 0);
  /** Method that user interface should call to create a new default item with
   * an automatically generated unique id.  */
  virtual SharedUiItem createNewItem(QString idQualifier) = 0;
  /** Method that user interface should call to change an item, one field at a
   * time.
   *
   * Suited for model/view edition. */
  virtual bool changeItemByUiData(SharedUiItem oldItem, int section,
                                  const QVariant &value) = 0;
  /** Method that user interface or non-interactive code should call to change
   * an item at whole. */
  virtual bool changeItem(SharedUiItem newItem, SharedUiItem oldItem) = 0;
  virtual SharedUiItem itemById(QString idQualifier, QString id) const = 0;
  /** Default: parses qualifiedId and calls itemById(QString,QString). */
  virtual SharedUiItem itemById(QString qualifiedId) const;
  /** Convenience template performing downcast. */
  template<class T>
  T itemById(QString idQualifier, QString id) const {
    SharedUiItem item = itemById(idQualifier, id);
    return static_cast<T&>(item);
  }
  /** Convenience template performing downcast. */
  template<class T>
  T itemById(QString qualifierId) const {
    SharedUiItem item = itemById(qualifierId);
    return static_cast<T&>(item);
  }
  /** This method build a list of every item currently holded, given their
   * qualifierId. */
  virtual SharedUiItemList<SharedUiItem> itemsByIdQualifier(
      QString idQualifier) const = 0;
  /** This method build a list of every item currently holded, given their class
   * (T) and qualifierId. */
  template<class T>
  SharedUiItemList<T> itemsByIdQualifier(QString idQualifier) const {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
    SharedUiItemList<SharedUiItem> list = itemsByIdQualifier(idQualifier);
    if (!list.isEmpty() && list[0].idQualifier() != idQualifier) {
      // LATER output warning
      //qWarning() << "SharedUiItemList<T>::itemsByIdQualifier called with "
      //              "inconsistent types and qualifier";
      return SharedUiItemList<T>();
    }
    return *reinterpret_cast<SharedUiItemList<T>*>(&list);
  }
  /** Notify document manager of a change in items order.
   *
   * Items list may contain a mix of several items type (i.e. with different
   * id qualifiers) and may be partial (i.e. not contains all items of a given
   * type).
   * In most cases items order are not significant.
   * Default implementation does nothing.
   */
  virtual void reorderedItems(QList<SharedUiItem> items);

signals:
  /** Emited whenever an item changes.
   *
   * When an item is created oldItem.isNull().
   * When an item is destroyed newItem.isNull().
   * When an item is renamed (changes id) newItem.id() != oldItem.id().
   * Otherwise the item is updated (changes anything apart its id).
   *
   * Notes: oldItem and newItem cannot be null at the same time, if none is null
   * then oldItem.idQualifier() == newItem.idQualifier().
   *
   * Subclasses may (should?) add specialized signals for their different item
   * types, e.g. "void customerChanged(Customer newItem, Customer oldItem)"
   * where Customer is a SharedUiItem.
   */
  void itemChanged(SharedUiItem newItem, SharedUiItem oldItem,
                   QString idQualifier);

protected:
  /** Can be called by createNewItem() implementations to generate a new id not
   * currently in use for the given idQualifier item type.
   * Generate id of the form idQualifier+number (e.g. "foobar1"). */
  QString genererateNewId(QString idQualifier);
};

#endif // SHAREDUIITEMDOCUMENTMANAGER_H
