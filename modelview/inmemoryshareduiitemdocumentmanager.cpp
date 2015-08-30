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
#include "inmemoryshareduiitemdocumentmanager.h"

InMemorySharedUiItemDocumentManager::InMemorySharedUiItemDocumentManager(
    QObject *parent) : SharedUiItemDocumentManager(parent) {
}

bool InMemorySharedUiItemDocumentManager::changeItem(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier,
    QString *errorString) {
  QString reason;
  if (!oldItem.isNull() && !_repository[idQualifier].contains(oldItem.id())) {
    // reject createOrUpdate and deleteIfExist behaviors
    reason = "old item "+oldItem.qualifiedId()+" not found";
  } else {
    if (!oldItem.isNull() && newItem != oldItem) { // renamed or deleted
      _repository[idQualifier].remove(oldItem.id());
    }
    if (!newItem.isNull()) { // created or updated
      _repository[idQualifier][newItem.id()] = newItem;
    }
    emit itemChanged(newItem, oldItem, idQualifier);
  }
  if (!reason.isEmpty() && errorString)
    *errorString = reason;
  return reason.isEmpty();
}

SharedUiItem InMemorySharedUiItemDocumentManager::itemById(
    QString idQualifier, QString id) const {
  return _repository.value(idQualifier).value(id);
}

SharedUiItemList<SharedUiItem> InMemorySharedUiItemDocumentManager
::itemsByIdQualifier(QString idQualifier) const {
  SharedUiItemList<SharedUiItem> list;
  foreach (SharedUiItem item, _repository.value(idQualifier))
    list.append(item);
  return list;
}
