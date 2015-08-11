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

SharedUiItem InMemorySharedUiItemDocumentManager::createNewItem(
    QString idQualifier) {
  //qDebug() << "createNewItem" << idQualifier;
  Creator creator = _creators.value(idQualifier);
  SharedUiItem newItem;
  if (creator) {
    QString id = genererateNewId(idQualifier);
    newItem = (*creator)(id);
    _repository[idQualifier][id] = newItem;
    emit itemChanged(newItem, SharedUiItem(), idQualifier);
    //qDebug() << "created";
  }
  return newItem;
}

bool InMemorySharedUiItemDocumentManager::changeItem(
    SharedUiItem newItem, SharedUiItem oldItem) {
  QString idQualifier;
  if (newItem != oldItem && !oldItem.isNull()) { // renamed or deleted
    idQualifier = oldItem.idQualifier();
    _repository[idQualifier].remove(oldItem.id());
  }
  if (!newItem.isNull()) {
    idQualifier = newItem.idQualifier();
    _repository[idQualifier][newItem.id()] = newItem;
  }
  if (!idQualifier.isEmpty())
    emit itemChanged(newItem, oldItem, idQualifier);
  return true;
}

bool InMemorySharedUiItemDocumentManager::changeItemByUiData(
    SharedUiItem oldItem, int section, const QVariant &value) {
  SharedUiItem newItem = oldItem;
  Setter setter = _setters.value(oldItem.idQualifier());
  //qDebug() << "changeItemByUiData" << oldItem.qualifiedId() << section
  //         << value << setter;
  if (setter && (newItem.*setter)(section, value, 0, Qt::EditRole, this))
    return changeItem(newItem, oldItem);
  return false;
}

SharedUiItem InMemorySharedUiItemDocumentManager::itemById(
    QString idQualifier, QString id) const {
  return _repository.value(idQualifier).value(id);
}

InMemorySharedUiItemDocumentManager &InMemorySharedUiItemDocumentManager
::registerItemType(QString idQualifier,
                    InMemorySharedUiItemDocumentManager::Setter setter,
                    InMemorySharedUiItemDocumentManager::Creator creator) {
  _setters.insert(idQualifier, setter);
  _creators.insert(idQualifier, creator);
  //qDebug() << "registered" << idQualifier;
  return *this;
}

SharedUiItemList<SharedUiItem> InMemorySharedUiItemDocumentManager
::itemsByIdQualifier(QString idQualifier) const {
  SharedUiItemList<SharedUiItem> list;
  foreach (SharedUiItem item, _repository.value(idQualifier))
    list.append(item);
  return list;
}
