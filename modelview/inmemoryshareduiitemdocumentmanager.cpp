/* Copyright 2015-2017 Hallowyn, Gregoire Barbier and others.
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
#include <QtDebug>

InMemorySharedUiItemDocumentManager::InMemorySharedUiItemDocumentManager(
    QObject *parent) : SharedUiItemDocumentManager(parent) {
}

bool InMemorySharedUiItemDocumentManager::prepareChangeItem(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem, SharedUiItem oldItem,
    QString idQualifier, QString *errorString) {
  Q_UNUSED(errorString)
  storeItemChange(transaction, newItem, oldItem, idQualifier);
  return true; // cannot fail
}

void InMemorySharedUiItemDocumentManager::commitChangeItem(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier) {
  if (!oldItem.isNull() && newItem != oldItem) { // renamed or deleted
    _repository[idQualifier].remove(oldItem.id());
  }
  if (!newItem.isNull()) { // created or updated
    _repository[idQualifier][newItem.id()] = newItem;
  }
  SharedUiItemDocumentManager::commitChangeItem(newItem, oldItem, idQualifier);
}

SharedUiItem InMemorySharedUiItemDocumentManager::itemById(
    QString idQualifier, QString id) const {
  return _repository.value(idQualifier).value(id);
}

SharedUiItemList<SharedUiItem> InMemorySharedUiItemDocumentManager
::itemsByIdQualifier(QString idQualifier) const {
  SharedUiItemList<SharedUiItem> list;
#ifdef QT_DEBUG
  if (!_repository.contains(idQualifier))
    qDebug() << "itemsByIdQualifier() called with id qualifier not found in "
                "repository:"
             << idQualifier;
#endif
  foreach (SharedUiItem item, _repository.value(idQualifier))
    list.append(item);
  return list;
}
