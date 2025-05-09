/* Copyright 2015-2025 Hallowyn, Gregoire Barbier and others.
 * This file is part of libpumpkin, see <http://libpumpkin.g76r.eu/>.
 * Libpumpkin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libpumpkin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libpumpkin.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef INMEMORYSHAREDUIITEMDOCUMENTMANAGER_H
#define INMEMORYSHAREDUIITEMDOCUMENTMANAGER_H

#include "shareduiitemdocumentmanager.h"

/** Simple generic implementation of SharedUiItemDocumentManager holding in
 * memory a repository of items by qualifier and id.
 *
 * To enable holding items, registerItemType() must be called for every
 * qualifier, in such a way:
 *   dm->registerItemType(
 *         "foobar", static_cast<InMemorySharedUiItemDocumentManager::Setter>(
 *         &Foobar::setUiData),
 *         [](QString id) static -> SharedUiItem { return Foobar(id); });
 *
 */
class LIBP6CORESHARED_EXPORT InMemorySharedUiItemDocumentManager
    : public SharedUiItemDocumentManager {
  Q_OBJECT
  Q_DISABLE_COPY(InMemorySharedUiItemDocumentManager)

protected:
  QMap<Utf8String,QHash<Utf8String,SharedUiItem>> _repository;

public:
  explicit InMemorySharedUiItemDocumentManager(QObject *parent = nullptr);
  using SharedUiItemDocumentManager::prepareChangeItem;
  bool prepareChangeItem(SharedUiItemDocumentTransaction *transaction,
      const SharedUiItem &new_item, const SharedUiItem &old_item,
      const Utf8String &qualifier, QString *errorString) override;
  using SharedUiItemDocumentManager::commitChangeItem;
  void commitChangeItem(
      const SharedUiItem &new_item, const SharedUiItem &old_item,
      const Utf8String &qualifier) override;
  using SharedUiItemDocumentManager::itemById;
  SharedUiItem itemById(
      const Utf8String &qualifier, const Utf8String &id) const override;
  using SharedUiItemDocumentManager::itemsByQualifier;
  SharedUiItemList itemsByQualifier(const Utf8String &qualifier) const override;
};

#endif // INMEMORYSHAREDUIITEMDOCUMENTMANAGER_H
