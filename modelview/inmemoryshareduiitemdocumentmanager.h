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
#ifndef INMEMORYSHAREDUIITEMDOCUMENTMANAGER_H
#define INMEMORYSHAREDUIITEMDOCUMENTMANAGER_H

#include "shareduiitemdocumentmanager.h"
#include <QHash>

/** Simple generic implementation of SharedUiItemDocumentManager holding in
 * memory a repository of items by idQualifier and id.
 *
 * To enable holding items, registerItemType() must be called for every
 * idQualifier, in such a way:
 *   dm->registerItemType(
 *         "foobar", static_cast<InMemorySharedUiItemDocumentManager::Setter>(
 *         &Foobar::setUiData),
 *         [](QString id) -> SharedUiItem { return Foobar(id); });
 *
 */
class LIBQTSSUSHARED_EXPORT InMemorySharedUiItemDocumentManager
    : public SharedUiItemDocumentManager {
  Q_OBJECT
  Q_DISABLE_COPY(InMemorySharedUiItemDocumentManager)

public:
  using Setter = bool (SharedUiItem::*)(int, const QVariant &, QString *, int,
  const SharedUiItemDocumentManager *);
  using Creator = SharedUiItem (*)(QString id);

protected:
  QHash<QString,QHash<QString,SharedUiItem>> _repository;
  QHash<QString,Setter> _setters;
  QHash<QString,Creator> _creators;

public:
  explicit InMemorySharedUiItemDocumentManager(QObject *parent = 0);
  SharedUiItem createNewItem(QString idQualifier) override;
  bool changeItem(SharedUiItem newItem, SharedUiItem oldItem,
                  QString idQualifier) override;
  bool changeItemByUiData(
      SharedUiItem oldItem, int section, const QVariant &value) override;
  /** Richer method that make it possible to implement interactive undo/redo
   * over it. */
  bool changeItemByUiData(SharedUiItem oldItem, int section,
                          const QVariant &value, SharedUiItem *newItem);
  using SharedUiItemDocumentManager::itemById;
  SharedUiItem itemById(QString idQualifier, QString id) const override;
  /** This method must be called for every item type the document manager will
   * hold, to enable it to create and modify such items. */
  InMemorySharedUiItemDocumentManager &registerItemType(
      QString idQualifier, Setter setter, Creator creator);
  using SharedUiItemDocumentManager::itemsByIdQualifier;
  SharedUiItemList<SharedUiItem> itemsByIdQualifier(QString idQualifier) const;
};

#endif // INMEMORYSHAREDUIITEMDOCUMENTMANAGER_H
