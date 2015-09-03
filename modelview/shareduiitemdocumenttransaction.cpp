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

void SharedUiItemDocumentTransaction::changeItem(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier) {
  new ChangeItemCommand(_dm, newItem, oldItem, idQualifier, this);
  QString id = newItem.id();
  if (newItem.isNull()) {
    _deletedItems[idQualifier].insert(id);
    _newItems[idQualifier].remove(id);
  } else {
    _deletedItems[idQualifier].remove(id);
    _newItems[idQualifier][id] = newItem;
  }
}

SharedUiItem SharedUiItemDocumentTransaction::itemById(
    QString idQualifier, QString id) {
  if (_deletedItems[idQualifier].contains(id))
    return SharedUiItem();
  SharedUiItem item = _newItems[idQualifier].value(id);
  return item.isNull() ? _dm->itemById(idQualifier, id) : item;
}

SharedUiItemList<> SharedUiItemDocumentTransaction::foreignKeySources(
    QString sourceQualifier, int sourceSection, QString referenceId) {
  SharedUiItemList<> sources;
  QHash<QString,SharedUiItem> &newItems = _newItems[sourceQualifier];
  foreach (const SharedUiItem &item, newItems.values()) {
    if (item.uiData(sourceSection) == referenceId)
      sources.append(item);
  }
  foreach (const SharedUiItem &item,
           _dm->itemsByIdQualifier(sourceQualifier)) {
    if (item.uiData(sourceSection) == referenceId
        && !newItems.contains(item.id())
        && !_deletedItems.contains(item.id()))
      sources.append(item);
  }
  return sources;
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
