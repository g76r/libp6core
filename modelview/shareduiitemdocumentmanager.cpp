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
#include "shareduiitemdocumentmanager.h"

SharedUiItemDocumentManager::SharedUiItemDocumentManager(QObject *parent)
  : QObject(parent) {
}

SharedUiItem SharedUiItemDocumentManager::itemById(QString qualifiedId) const {
  int pos = qualifiedId.indexOf(':');
  return (pos == -1) ? itemById(QString(), qualifiedId)
                     : itemById(qualifiedId.left(pos), qualifiedId.mid(pos+1));
}

QString SharedUiItemDocumentManager::genererateNewId(QString idQualifier) {
  QString id;
  for (int i = 1; i < 100; ++i) {
    id = idQualifier+QString::number(i);
    if (itemById(idQualifier, id).isNull())
      return id;
  }
  forever {
    id = idQualifier+QString::number(qrand());
    if (itemById(idQualifier, id).isNull())
      return id;
  }
}

void SharedUiItemDocumentManager::reorderItems(QList<SharedUiItem> items) {
  Q_UNUSED(items)
}

SharedUiItem SharedUiItemDocumentManager::createNewItem(
    QString idQualifier, QString *errorString) {
  Creator creator = _creators.value(idQualifier);
  //qDebug() << "createNewItem" << idQualifier << this << creator;
  SharedUiItem newItem;
  if (creator) {
    QString id = genererateNewId(idQualifier);
    newItem = (*creator)(id);
    if (changeItem(newItem, SharedUiItem(), idQualifier, errorString))
      return newItem;
    return SharedUiItem();
    //qDebug() << "created";
  } else {
    if (errorString)
      *errorString = "no creator registered for item of type "+idQualifier;
  }
  return newItem;
}

void SharedUiItemDocumentManager::registerItemType(
    QString idQualifier, SharedUiItemDocumentManager::Setter setter,
    SharedUiItemDocumentManager::Creator creator) {
  _setters.insert(idQualifier, setter);
  _creators.insert(idQualifier, creator);
  //qDebug() << "registered" << idQualifier << this;
}

bool SharedUiItemDocumentManager::changeItemByUiData(
    SharedUiItem oldItem, int section, const QVariant &value,
    QString *errorString) {
  //if (oldItem.uiData(section, Qt::EditRole) == value)
  //  return true; // nothing to do
  Setter setter = _setters.value(oldItem.idQualifier());
  SharedUiItem newItem = oldItem;
  if (setter) {
    // LATER always EditRole ?
    if ((newItem.*setter)(section, value, errorString, Qt::EditRole, this)
      && changeItem(newItem, oldItem, oldItem.idQualifier(), errorString))
      return true;
  } else {
    if (errorString)
      *errorString = "No setter registred for item type "+oldItem.idQualifier();
  }
  return false;
}
