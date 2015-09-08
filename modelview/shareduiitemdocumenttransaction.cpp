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
#include "shareduiitemdocumenttransaction.h"
#include "shareduiitemdocumentmanager.h"

void SharedUiItemDocumentTransaction::storeItemChange(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier) {
  new ChangeItemCommand(_dm, newItem, oldItem, idQualifier, this);
  if (!oldItem.isNull())
    _newItems[idQualifier][oldItem.id()] = SharedUiItem();
  if (!newItem.isNull())
    _newItems[idQualifier][newItem.id()] = newItem;
}

SharedUiItem SharedUiItemDocumentTransaction::itemById(
    QString idQualifier, QString id) const {
  const QHash<QString,SharedUiItem> newItems = _newItems[idQualifier];
  return newItems.contains(id) ? newItems.value(id)
                               : _dm->itemById(idQualifier, id);
}

SharedUiItemList<> SharedUiItemDocumentTransaction::changingItems() const {
  SharedUiItemList<> items;
  foreach (const QString &idQualifier, _newItems.keys())
    foreach (const SharedUiItem &item, _newItems.value(idQualifier).values()) {
      if (!item.isNull())
        items.append(item);
    }
  return items;
}

SharedUiItem SharedUiItemDocumentTransaction::oldItemIdByChangingItem(
    SharedUiItem changingItem) const {
  Q_UNUSED(changingItem)
  // LATER (together with compression)
  return SharedUiItem();
}

SharedUiItemList<> SharedUiItemDocumentTransaction::foreignKeySources(
    QString sourceQualifier, int sourceSection, QString referenceId) const {
  SharedUiItemList<> sources;
  const QHash<QString,SharedUiItem> newItems = _newItems[sourceQualifier];
  foreach (const SharedUiItem &item, newItems.values()) {
    if (item.uiData(sourceSection) == referenceId)
      sources.append(item);
  }
  foreach (const SharedUiItem &item,
           _dm->itemsByIdQualifier(sourceQualifier)) {
    if (item.uiData(sourceSection) == referenceId
        && !newItems.contains(item.id()))
      sources.append(item);
  }
  return sources;
}

bool SharedUiItemDocumentTransaction::changeItemByUiData(
    SharedUiItem oldItem, int section, const QVariant &value,
    QString *errorString) {
  QString idQualifier = oldItem.idQualifier();
  SharedUiItemDocumentManager::Setter setter =
      _dm->_setters.value(idQualifier);
  SharedUiItem newItem = oldItem;
  if (setter) {
    // LATER always EditRole ?
    // LATER simplify constraints processing since only one section is touched
    if (setter(&newItem, section, value, errorString, this, Qt::EditRole)) {
      if (_dm->processConstraintsAndPrepareChangeItem(
            this, newItem, oldItem, idQualifier, errorString)) {
        return true;
      }
    }
  } else {
    *errorString = "No setter registred for item type "+oldItem.idQualifier();
  }
  return false;
}

bool SharedUiItemDocumentTransaction::changeItem(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier,
    QString *errorString) {
  return _dm->processConstraintsAndPrepareChangeItem(
        this, newItem, oldItem, idQualifier, errorString);
}

SharedUiItem SharedUiItemDocumentTransaction::createNewItem(
    QString idQualifier, QString *errorString) {
  SharedUiItemDocumentManager::Creator creator =
      _dm->_creators.value(idQualifier);
  SharedUiItem nullItem;
  if (creator) {
    QString id = _dm->genererateNewId(idQualifier);
    SharedUiItem newItem = creator(id);
    if (newItem.isNull()) {
      *errorString = "Creation of item of type "+idQualifier+" failed";
      return nullItem;
    } else {
      if (!_dm->processConstraintsAndPrepareChangeItem(
            this, newItem, nullItem, idQualifier, errorString))
        return nullItem;
    }
    return newItem;
  } else {
    *errorString = "No creator registered for item of type "+idQualifier;
    return nullItem;
  }
}

SharedUiItemDocumentTransaction::ChangeItemCommand::ChangeItemCommand(
    SharedUiItemDocumentManager *dm, SharedUiItem newItem, SharedUiItem oldItem,
    QString idQualifier, CoreUndoCommand *parent)
  : CoreUndoCommand(parent), _dm(dm), _newItem(newItem), _oldItem(oldItem),
    _idQualifier(idQualifier)  {
  //qDebug() << "ChangeItemCommand::ChangeItemCommand()" << parent;
  // LATER: compose a textual description of command and call setText
}

void SharedUiItemDocumentTransaction::ChangeItemCommand::redo() {
  //qDebug() << "SharedUiItemDocumentManager::ChangeItemCommand::redo()"
  //         << _dm << _ignoreFirstRedo << _newItem << _oldItem;
  if (_dm)
    _dm->commitChangeItem(_newItem, _oldItem, _idQualifier);
}

void SharedUiItemDocumentTransaction::ChangeItemCommand::undo() {
  //qDebug() << "SharedUiItemDocumentManager::ChangeItemCommand::undo()"
  //         << _dm << _newItem << _oldItem;
  if (_dm)
    _dm->commitChangeItem(_oldItem, _newItem, _idQualifier);
}

int	SharedUiItemDocumentTransaction::ChangeItemCommand::id() const {
  return 42;
}

bool SharedUiItemDocumentTransaction::ChangeItemCommand::mergeWith(
    const CoreUndoCommand *command) {
  const ChangeItemCommand *other =
      static_cast<const ChangeItemCommand *>(command);
  Q_UNUSED(other)
  // LATER implement this or find another way to compress commands within a transaction in case the same item is changed several times in chain
  return false;
}
