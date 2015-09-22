/* Copyright 2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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

protected:
  QHash<QString,QHash<QString,SharedUiItem>> _repository;

public:
  explicit InMemorySharedUiItemDocumentManager(QObject *parent = 0);
  bool prepareChangeItem(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem, SharedUiItem oldItem,
      QString idQualifier, QString *errorString) override;
  void commitChangeItem(SharedUiItem newItem, SharedUiItem oldItem,
                        QString idQualifier) override;
  using SharedUiItemDocumentManager::itemById;
  SharedUiItem itemById(QString idQualifier, QString id) const override;
  using SharedUiItemDocumentManager::itemsByIdQualifier;
  SharedUiItemList<SharedUiItem> itemsByIdQualifier(QString idQualifier) const;
};

#endif // INMEMORYSHAREDUIITEMDOCUMENTMANAGER_H
