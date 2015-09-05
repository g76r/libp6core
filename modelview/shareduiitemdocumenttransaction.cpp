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
