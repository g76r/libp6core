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
#include "simpleshareduiitemdocumentmanager.h"

SimpleSharedUiItemDocumentManager::SimpleSharedUiItemDocumentManager(
    QObject *parent) : SharedUiItemDocumentManager(parent) {
}

SharedUiItem SimpleSharedUiItemDocumentManager::createNewItem(
    QString idQualifier) {
  //qDebug() << "createNewItem" << idQualifier;
  Creator creator = _creators.value(idQualifier);
  SharedUiItem newItem;
  if (creator) {
    QString id = genererateNewId(idQualifier);
    newItem = (*creator)(id);
    _repository[idQualifier][id] = newItem;
    emit itemChanged(newItem, SharedUiItem());
    //qDebug() << "created";
  }
  return newItem;
}

bool SimpleSharedUiItemDocumentManager::changeItem(
    SharedUiItem newItem, SharedUiItem oldItem) {
  if (newItem != oldItem) // renamed
    _repository[oldItem.idQualifier()].remove(oldItem.id());
  _repository[newItem.idQualifier()][newItem.id()] = newItem;
  emit itemChanged(newItem, oldItem);
  //qDebug() << "changed";
  return true;
}

bool SimpleSharedUiItemDocumentManager::changeItemByUiData(
    SharedUiItem oldItem, int section, const QVariant &value) {
  SharedUiItem newItem = oldItem;
  Setter setter = _setters.value(oldItem.idQualifier());
  //qDebug() << "changeItemByUiData" << oldItem.qualifiedId() << section
  //         << value << setter;
  if (setter && (newItem.*setter)(section, value, 0, Qt::EditRole, this))
    return changeItem(newItem, oldItem);
  return false;
}

SharedUiItem SimpleSharedUiItemDocumentManager::itemById(
    QString idQualifier, QString id) const {
  return _repository.value(idQualifier).value(id);
}

SimpleSharedUiItemDocumentManager &SimpleSharedUiItemDocumentManager
::registerItemType(QString idQualifier,
                    SimpleSharedUiItemDocumentManager::Setter setter,
                    SimpleSharedUiItemDocumentManager::Creator creator) {
  _setters.insert(idQualifier, setter);
  _creators.insert(idQualifier, creator);
  //qDebug() << "registered" << idQualifier;
  return *this;
}
