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
#ifndef SIMPLESHAREDUIITEMDOCUMENTMANAGER_H
#define SIMPLESHAREDUIITEMDOCUMENTMANAGER_H

#include "shareduiitemdocumentmanager.h"
#include <QHash>

/** Simple generic implementation of SharedUiItemDocumentManager holding in
 * memory a repository of items by idQualifier and id.
 *
 * To enable holding items, registerItemType() must be called for every
 * idQualifier, in such a way:
 *   dm->registerItemType(
 *         "foobar", static_cast<SimpleSharedUiItemDocumentManager::Setter>(
 *         &Foobar::setUiData),
 *         [](QString id) -> SharedUiItem { return Foobar(id); });
 *
 */
class LIBQTSSUSHARED_EXPORT SimpleSharedUiItemDocumentManager
    : public SharedUiItemDocumentManager {
public:
  using Setter = bool (SharedUiItem::*)(int, const QVariant &, QString *, int,
  const SharedUiItemDocumentManager *);
  using Creator = SharedUiItem (*)(QString id);

private:
  QHash<QString,QHash<QString,SharedUiItem>> _repository;
  QHash<QString,Setter> _setters;
  QHash<QString,Creator> _creators;

public:
  explicit SimpleSharedUiItemDocumentManager(QObject *parent = 0);
  SharedUiItem createNewItem(QString idQualifier) override;
  bool changeItem(SharedUiItem newItem, SharedUiItem oldItem);
  bool changeItemByUiData(
      SharedUiItem oldItem, int section, const QVariant &value) override;
  SharedUiItem itemById(QString idQualifier, QString id) const override;
  /** This method must be called for every item type the document manager will
   * hold, to enable it to create and modify such items. */
  SimpleSharedUiItemDocumentManager &registerItemType(
      QString idQualifier, Setter setter, Creator creator);
  /** This method build a list of every item currently holded, given it class
   * (T) and qualifierId. */
  template<class T = SharedUiItem>
  QList<T> itemsByQualifierId(QString qualifierId) const {
    QList<T> list;
    foreach (SharedUiItem item, _repository.value(qualifierId))
      list.append(*static_cast<T*>(&item));
    return list;
  }
};

#endif // SIMPLESHAREDUIITEMDOCUMENTMANAGER_H
