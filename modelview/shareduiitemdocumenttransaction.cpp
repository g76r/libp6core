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
#include "shareduiitemdocumenttransaction.h"
#include "shareduiitemdocumentmanager.h"

void SharedUiItemDocumentTransaction::storeItemChange(
    const SharedUiItem &newItem, const SharedUiItem &oldItem,
    const Utf8String &qualifier) {
  ChangeItemCommand *command =
      new ChangeItemCommand(_dm, newItem, oldItem, qualifier, this);
  switch (childCount()) {
  case 1:
    setText(command->text());
    break;
  case 2:
    setText(text()+" and other changes");
    break;
  }
  auto oldId = oldItem.id(), newId = newItem.id();
  auto &changingItems = _changingItems[qualifier];
  if (!oldItem.isNull() && !changingItems.contains(oldId))
    _originalItems[qualifier].insert(oldId, oldItem);
  if (!oldItem.isNull())
    changingItems.insert(oldId, SharedUiItem());
  if (!newItem.isNull())
    changingItems.insert(newId, newItem);
}

SharedUiItem SharedUiItemDocumentTransaction::itemById(
    const Utf8String &qualifier, const Utf8String &id) const {
  auto newItems = _changingItems[qualifier];
  return newItems.contains(id) ? newItems.value(id)
                               : _dm->itemById(qualifier, id);
}

SharedUiItemList SharedUiItemDocumentTransaction::itemsByQualifier(
    const Utf8String &qualifier) const {
  auto changingItems = _changingItems.value(qualifier);
  SharedUiItemList items;
  for (const auto &item: changingItems.values())
    if (!item.isNull())
      items.append(item);
  for (const auto &item: _dm->itemsByQualifier(qualifier))
    if (!changingItems.contains(item.id()))
      items.append(item);
  return items;
}

SharedUiItemList SharedUiItemDocumentTransaction::changingItems() const {
  SharedUiItemList items;
  for (const auto &[qualifier,original_items]: _changingItems.asKeyValueRange())
    for (const auto &item: original_items) {
      if (!item.isNull())
        items.append(item);
    }
  return items;
}

SharedUiItemList SharedUiItemDocumentTransaction::originalItems() const {
  SharedUiItemList items;
  for (const auto &[qualifier,original_items]: _originalItems.asKeyValueRange())
    for (const auto &item: original_items) {
      if (!item.isNull())
        items.append(item);
    }
  return items;
}

/*SharedUiItem SharedUiItemDocumentTransaction::oldItemIdByChangingItem(
    SharedUiItem changingItem) const {
  Q_UNUSED(changingItem)
  // LATER (together with compression)
  return SharedUiItem();
}*/

SharedUiItemList SharedUiItemDocumentTransaction::foreignKeySources(
    const Utf8String &sourceQualifier, int sourceSection,
    const Utf8String &referenceId) const {
  SharedUiItemList sources;
  auto changingItems = _changingItems.value(sourceQualifier);
  for (const auto &item: changingItems.values()) {
    if (item.uiData(sourceSection) == referenceId)
      sources.append(item);
  }
  for (const auto &item: _dm->itemsByQualifier(sourceQualifier)) {
    if (item.uiData(sourceSection) == referenceId
        && !changingItems.contains(item.id()))
      sources.append(item);
  }
  return sources;
}

bool SharedUiItemDocumentTransaction::changeItemByUiData(
    const SharedUiItem &oldItem, int section, const QVariant &value,
    QString *errorString) {
  auto qualifier = oldItem.qualifier();
  SharedUiItemDocumentManager::Setter setter =
      _dm->_setters.value(qualifier);
  SharedUiItem newItem = oldItem;
  if (setter) {
    // LATER always EditRole ?
    // LATER simplify constraints processing since only one section is touched
    if (setter(&newItem, section, value, errorString, this, Qt::EditRole)) {
      if (_dm->processConstraintsAndPrepareChangeItem(
            this, newItem, oldItem, qualifier, errorString)) {
        return true;
      }
    }
  } else {
    *errorString = "No setter registred for item type "+oldItem.qualifier();
  }
  return false;
}

bool SharedUiItemDocumentTransaction::changeItem(
    const SharedUiItem &newItem, const SharedUiItem &oldItem,
    const Utf8String &qualifier, QString *errorString) {
  return _dm->processConstraintsAndPrepareChangeItem(
        this, newItem, oldItem, qualifier, errorString);
}

SharedUiItem SharedUiItemDocumentTransaction::createNewItem(
    const Utf8String &qualifier, PostCreationModifier modifier,
    QString *errorString) {
  SharedUiItemDocumentManager::Creator creator =
      _dm->_creators.value(qualifier);
  SharedUiItem nullItem;
  if (creator) {
    auto id = _dm->generateNewId(this, qualifier);
    SharedUiItem newItem = creator(this, id, errorString);
    if (newItem.isNull()) {
      return nullItem;
    } else {
      if (modifier)
        modifier(this, &newItem, errorString);
      if (!_dm->processConstraintsAndPrepareChangeItem(
            this, newItem, nullItem, qualifier, errorString))
        return nullItem;
    }
    return newItem;
  } else {
    *errorString = "No creator registered for item of type "+qualifier;
    return nullItem;
  }
}

Utf8String SharedUiItemDocumentTransaction::generateNewId(
    const Utf8String &qualifier, const Utf8String &prefix) const {
  return _dm->generateNewId(this, qualifier, prefix);
}

SharedUiItemDocumentTransaction::ChangeItemCommand::ChangeItemCommand(
    SharedUiItemDocumentManager *dm, const SharedUiItem &newItem,
    const SharedUiItem &oldItem, const Utf8String &qualifier,
    CoreUndoCommand *parent)
  : CoreUndoCommand(parent), _dm(dm), _newItem(newItem), _oldItem(oldItem),
    _qualifier(qualifier)  {
  if (newItem.isNull())
    setText("Deleting a "+oldItem.qualifier());
  else if (oldItem.isNull())
    setText("Creating a "+newItem.qualifier());
  else
    setText("Changing a "+oldItem.qualifier());
}

void SharedUiItemDocumentTransaction::ChangeItemCommand::redo() {
  if (_dm)
    _dm->commitChangeItem(_newItem, _oldItem, _qualifier);
}

void SharedUiItemDocumentTransaction::ChangeItemCommand::undo() {
  if (_dm)
    _dm->commitChangeItem(_oldItem, _newItem, _qualifier);
}

int SharedUiItemDocumentTransaction::ChangeItemCommand::id() const {
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
