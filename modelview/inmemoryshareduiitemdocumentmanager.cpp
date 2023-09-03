/* Copyright 2015-2023 Hallowyn, Gregoire Barbier and others.
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
#include "inmemoryshareduiitemdocumentmanager.h"
#include "log/log.h"

InMemorySharedUiItemDocumentManager::InMemorySharedUiItemDocumentManager(
    QObject *parent) : SharedUiItemDocumentManager(parent) {
}

bool InMemorySharedUiItemDocumentManager::prepareChangeItem(
    SharedUiItemDocumentTransaction *transaction, const SharedUiItem &new_item,
    const SharedUiItem &old_item, const Utf8String &qualifier,
    QString *) {
  storeItemChange(transaction, new_item, old_item, qualifier);
  return true; // cannot fail
}

void InMemorySharedUiItemDocumentManager::commitChangeItem(
    const SharedUiItem &new_item, const SharedUiItem &old_item,
    const Utf8String &qualifier) {
  if (!old_item.isNull() && new_item != old_item) { // renamed or deleted
    _repository[qualifier].remove(old_item.id());
  }
  if (!new_item.isNull()) { // created or updated
    _repository[qualifier][new_item.id()] = new_item;
  }
  emit itemChanged(new_item, old_item, qualifier);
}

SharedUiItem InMemorySharedUiItemDocumentManager::itemById(
    const Utf8String &qualifier, const Utf8String &id) const {
  return _repository.value(qualifier).value(id);
}

SharedUiItemList<SharedUiItem> InMemorySharedUiItemDocumentManager
::itemsByIdQualifier(const Utf8String &qualifier) const {
  SharedUiItemList<SharedUiItem> list;
  if (!_repository.contains(qualifier))
    Log::warning() << "itemsByIdQualifier() called with id qualifier not found "
                      "in repository:" << qualifier;
  return _repository.value(qualifier).values();
}
