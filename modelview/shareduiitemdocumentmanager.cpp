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
#include <QtDebug>

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

void SharedUiItemDocumentManager::registerItemType(
    QString idQualifier, SharedUiItemDocumentManager::Setter setter,
    SharedUiItemDocumentManager::Creator creator) {
  _setters.insert(idQualifier, setter);
  _creators.insert(idQualifier, creator);
  //qDebug() << "registered" << idQualifier << this;
}

bool SharedUiItemDocumentManager::changeItem(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier,
    QString *errorString) {
  Q_ASSERT(newItem.isNull() || newItem.idQualifier() == idQualifier);
  Q_ASSERT(oldItem.isNull() || oldItem.idQualifier() == idQualifier);
  // support for transforming into update an createOrUpdate call
  // and ensure that, on update or delete, oldItem is the real one, not a
  // placeholder only bearing ids
  oldItem = itemById(idQualifier, oldItem.id());
  if (oldItem.isNull() && newItem.isNull())
    return true; // nothing to do (support deleteIfExists on inexistent item)
  CoreUndoCommand *command =
      internalChangeItem(newItem, oldItem, idQualifier, errorString);
  //qDebug() << "SharedUiItemDocumentManager::changeItem" << command;
  if (command) {
    command->redo();
    delete command;
    return true;
  }
  return false;
}

CoreUndoCommand *SharedUiItemDocumentManager::internalChangeItem(
    SharedUiItem newItem, SharedUiItem oldItem,
    QString idQualifier, QString *errorString) {
  CoreUndoCommand *command = new CoreUndoCommand;
  QString reason;
  if (!errorString)
    errorString = &reason;
  // LATER simplify constraints processing when called through changeItemByUiData()
  if (checkAndApplyConstraintsOnChangeItem(
        command, newItem, oldItem, idQualifier, errorString)
      && prepareChangeItem(
        command, newItem, oldItem, idQualifier, errorString)) {
    return command;
  }
  delete command;
  return 0;
}

SharedUiItemDocumentManager::ChangeItemCommand::ChangeItemCommand(
    SharedUiItemDocumentManager *dm, SharedUiItem newItem, SharedUiItem oldItem,
    QString idQualifier, CoreUndoCommand *parent)
  : CoreUndoCommand(parent), _dm(dm), _newItem(newItem), _oldItem(oldItem),
    _idQualifier(idQualifier),
    _ignoreFirstRedo(false/*alreadyCommitedBeforeFirstRedo*/) {
  //qDebug() << "ChangeItemCommand::ChangeItemCommand()" << parent;
  // LATER: compose a textual description of command and call setText
}

void SharedUiItemDocumentManager::ChangeItemCommand::redo() {
  //qDebug() << "SharedUiItemDocumentManager::ChangeItemCommand::redo()"
  //         << _dm << _ignoreFirstRedo << _newItem << _oldItem;
  if (_dm && !_ignoreFirstRedo)
    _dm->commitChangeItem(_newItem, _oldItem, _idQualifier);
  else
    _ignoreFirstRedo = false;
}

void SharedUiItemDocumentManager::ChangeItemCommand::undo() {
  //qDebug() << "SharedUiItemDocumentManager::ChangeItemCommand::undo()"
  //         << _dm << _newItem << _oldItem;
  if (_dm)
    _dm->commitChangeItem(_oldItem, _newItem, _idQualifier);
}

void SharedUiItemDocumentManager::commitChangeItem(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier) {
  emit itemChanged(newItem, oldItem, idQualifier);
}

SharedUiItem SharedUiItemDocumentManager::createNewItem(
    QString idQualifier, QString *errorString) {
  SharedUiItem newItem;
  CoreUndoCommand *command =
      internalCreateNewItem(&newItem, idQualifier, errorString);
  if (command) {
    command->redo();
    delete command;
    return newItem;
  }
  return SharedUiItem();
}

CoreUndoCommand *SharedUiItemDocumentManager::internalCreateNewItem(
    SharedUiItem *newItem, QString idQualifier, QString *errorString) {
  Q_ASSERT(newItem != 0);
  CoreUndoCommand *command = new CoreUndoCommand;
  QString reason;
  if (!errorString)
    errorString = &reason;
  Creator creator = _creators.value(idQualifier);
  if (creator) {
    QString id = genererateNewId(idQualifier);
    *newItem = creator(id);
    if (!newItem->isNull()) {
      new ChangeItemCommand(this, *newItem, SharedUiItem(), idQualifier,
                            command);
    }
    return command;
  } else {
    if (errorString)
      *errorString = "no creator registered for item of type "+idQualifier;
  }
  delete command;
  return 0;
}

bool SharedUiItemDocumentManager::changeItemByUiData(
    SharedUiItem oldItem, int section, const QVariant &value,
    QString *errorString) {
  CoreUndoCommand *command = internalChangeItemByUiData(
        oldItem, section, value, errorString);
  if (command) {
    command->redo();
    delete command;
    return true;
  }
  return false;
}

CoreUndoCommand *SharedUiItemDocumentManager::internalChangeItemByUiData(
    SharedUiItem oldItem, int section, const QVariant &value,
    QString *errorString) {
  CoreUndoCommand *command = new CoreUndoCommand;
  QString reason;
  if (!errorString)
    errorString = &reason;
  QString idQualifier = oldItem.idQualifier();
  Setter setter = _setters.value(idQualifier);
  SharedUiItem newItem = oldItem;
  if (setter) {
    // LATER always EditRole ?
    // LATER simplify constraints processing since only one section is touched
    if ((newItem.*setter)(section, value, errorString, Qt::EditRole, this)) {
      if (checkAndApplyConstraintsOnChangeItem(
            command, newItem, oldItem, idQualifier, errorString)
          && prepareChangeItem(
            command, newItem, oldItem, idQualifier, errorString)) {
        return command;
      }
    }
  } else {
    if (errorString)
      *errorString = "No setter registred for item type "+oldItem.idQualifier();
  }
  delete command;
  return 0;
}

void SharedUiItemDocumentManager::addForeignKey(
    QString sourceQualifier, int sourceSection, QString referenceQualifier,
    int referenceSection, OnChangePolicy onDeletePolicy,
    OnChangePolicy onUpdatePolicy, bool nullable) {
  _foreignKeys.append(
        ForeignKey(sourceQualifier, sourceSection, referenceQualifier,
                   referenceSection, onDeletePolicy, onUpdatePolicy, nullable));
}

static SharedUiItemList<> sources(
    SharedUiItemDocumentManager *dm, QString sourceQualifier, int sourceSection,
    QString referenceId) {
  SharedUiItemList<> sources;
  foreach (const SharedUiItem &item, dm->itemsByIdQualifier(sourceQualifier)) {
    if (item.uiData(sourceSection) == referenceId)
      sources.append(item);
  }
  return sources;
}

// FIXME not sure that & are needed
// FIXME remove fk._nullable
// FIXME split On -> {Before,After}
bool SharedUiItemDocumentManager::checkAndApplyConstraintsOnChangeItem(
    CoreUndoCommand *command, SharedUiItem &newItem, SharedUiItem &oldItem,
    QString idQualifier, QString *errorString) {
  QString reason;
  if (!errorString)
    errorString = &reason;
  foreach (const ForeignKey &fk, _foreignKeys) {
    // foreign keys from this item
    if (fk._sourceQualifier == idQualifier) {
      // FIXME referential integrity
    }
    // foreign keys to this item
    if (fk._referenceQualifier == idQualifier) {
      if (newItem.isNull()) {
        // on delete policy
        SharedUiItemList<> references =
            sources(this, fk._sourceQualifier, fk._sourceSection, oldItem.id());
        switch (fk._onDeletePolicy) {
        case Unknown:
        case NoAction:
          if (!references.isEmpty()) {
            *errorString = "Cannot delete "+idQualifier+" \""+oldItem.id()
                +"\" because it is stil referenced by "
                +QString::number(references.size())+" "+fk._sourceQualifier
                +"(s).";
            return false;
          }
          break;
        case SetNull:
          // FIXME
          break;
        case Cascade:
          // FIXME
          break;
        }
      } else if (newItem.uiData(fk._referenceSection)
                 != oldItem.uiData(fk._referenceSection)) {
        // on update policy
        SharedUiItemList<> references =
            sources(this, fk._sourceQualifier, fk._sourceSection, oldItem.id());
        switch (fk._onUpdatePolicy) {
        case Unknown:
        case NoAction:
          if (!references.isEmpty()) {
            *errorString = "Cannot rename "+idQualifier+" \""+oldItem.id()
                +"\" because it is stil referenced by "
                +QString::number(references.size())+" "+fk._sourceQualifier
                +"(s).";
            return false;
          }
          break;
        case SetNull:
          // FIXME
          break;
        case Cascade:
          // FIXME
          break;
        }
      }
    }
  }
  return true;
}
