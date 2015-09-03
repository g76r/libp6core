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

QString SharedUiItemDocumentManager::genererateNewId(
    QString idQualifier, QString prefix) {
  QString id;
  if (prefix.isEmpty())
    prefix = idQualifier;
  for (int i = 1; i < 100; ++i) {
    id = prefix+QString::number(i);
    if (itemById(idQualifier, id).isNull())
      return id;
  }
  forever {
    id = prefix+QString::number(qrand());
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
  if (processConstraintsAndPrepareChangeItem(
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
    SharedUiItem oldItem;
    if (!newItem->isNull()) {
      if (processConstraintsAndPrepareChangeItem(
            command, *newItem, oldItem, idQualifier, errorString)) {
        return command;
      }
    } else {
      *errorString = "creation of item of type "+idQualifier+" failed";
    }
  } else {
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
      if (processConstraintsAndPrepareChangeItem(
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

bool SharedUiItemDocumentManager::processConstraintsAndPrepareChangeItem(
    CoreUndoCommand *command, SharedUiItem newItem, SharedUiItem oldItem,
    QString idQualifier, QString *errorString) {
  // TODO add recursive context to detect constraints loops, or check recursive constraints when adding them (addFK()...)
  qDebug() << "SharedUiItemDocumentManager::prepareChangeItemWithConstraints"
           << newItem << oldItem << idQualifier;
  // to support for transforming into update an createOrUpdate call
  // and ensure that, on update or delete, oldItem is the real one, not a
  // placeholder only bearing ids such as GenericSharedUiItem, we must replace
  // oldItem by trusting only its ids
  oldItem = itemById(idQualifier, oldItem.id());
  if (oldItem.isNull()) {
    if (newItem.isNull()) {
      // nothing to do (e.g. deleteIfExists on inexistent item)
      return true;
    } else { // create
      return processBeforeCreate(command, &newItem, idQualifier, errorString)
          && prepareChangeItem(command, newItem, oldItem, idQualifier,
                               errorString)
          && processAfterCreate(command, newItem, idQualifier, errorString);
    }
  } else {
    if (newItem.isNull()) { // delete
      return processBeforeDelete(command, oldItem, idQualifier, errorString)
          && prepareChangeItem(command, newItem, oldItem, idQualifier,
                               errorString)
          && processAfterDelete(command, oldItem, idQualifier, errorString);
    } else { // update (incl. renaming)
      return processBeforeUpdate(command, &newItem, oldItem, idQualifier,
                                 errorString)
          && prepareChangeItem(command, newItem, oldItem, idQualifier,
                               errorString)
          && processAfterUpdate(command, newItem, oldItem, idQualifier,
                                errorString);
    }
  }
}

void SharedUiItemDocumentManager::addForeignKey(
    QString sourceQualifier, int sourceSection, QString referenceQualifier,
    int referenceSection, OnChangePolicy onUpdatePolicy,
    OnChangePolicy onDeletePolicy) {
  _foreignKeys.append(
        ForeignKey(sourceQualifier, sourceSection, referenceQualifier,
                   referenceSection, onDeletePolicy, onUpdatePolicy));
}

static SharedUiItemList<> foreignKeySources(
    SharedUiItemDocumentManager *dm, QString sourceQualifier, int sourceSection,
    QString referenceId) {
  SharedUiItemList<> sources;
  foreach (const SharedUiItem &item, dm->itemsByIdQualifier(sourceQualifier)) {
    if (item.uiData(sourceSection) == referenceId)
      sources.append(item);
  }
  return sources;
}

bool SharedUiItemDocumentManager::checkIdsConstraints(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier,
    QString *errorString) {
  if (!oldItem.isNull()) {
    if (oldItem.idQualifier() != idQualifier) {
      *errorString = "Old item \""+oldItem.qualifiedId()
          +"\" is inconsistent with qualifier \""+idQualifier+"\".";
      return false;
    }
  }
  if (!newItem.isNull()) {
    if (newItem.idQualifier() != idQualifier) {
      *errorString = "New item \""+newItem.qualifiedId()
          +"\" is inconsistent with qualifier \""+idQualifier+"\".";
      return false;
    }
    if (newItem.id().isEmpty()) {
      *errorString = "Id cannot be empty.";
      return false;
    }
    if (newItem.id() != oldItem.id() // create or rename
        && !itemById(idQualifier, newItem.id()).isNull()) {
      *errorString = "New id is already used by another "+idQualifier+": "
          +newItem.id();
      return false;
    }
  }
  return true;
}

bool SharedUiItemDocumentManager::processBeforeUpdate(
    CoreUndoCommand *command, SharedUiItem *newItem, SharedUiItem oldItem,
    QString idQualifier, QString *errorString) {
  Q_UNUSED(command)
  if (!checkIdsConstraints(*newItem, oldItem, idQualifier, errorString))
    return false;
  foreach (const ForeignKey &fk, _foreignKeys) {
    // foreign keys from this item
    if (fk._sourceQualifier == idQualifier) {
      // referential integrity
      QString referenceId = newItem->uiString(fk._sourceSection);
      if (!referenceId.isEmpty()
          && itemById(fk._referenceQualifier, referenceId).isNull()) {
        *errorString = "Cannot change "+idQualifier+" \""+oldItem.id()
            +"\" because there is no "+fk._referenceQualifier+" with id \""
            +referenceId+"\".";
        return false;
      }
    }
    // foreign keys to this item
    if (fk._referenceQualifier == idQualifier) {
      if (newItem->uiData(fk._referenceSection)
          != oldItem.uiData(fk._referenceSection)) {
        // on update policy
        SharedUiItemList<> sources = foreignKeySources(
              this, fk._sourceQualifier, fk._sourceSection, oldItem.id());
        switch (fk._onUpdatePolicy) {
        case SetNull: // FIXME implement rather than fall through NoAction
        case Cascade: // FIXME implement rather than fall through NoAction
        case Unknown:
        case NoAction:
          if (!sources.isEmpty()) {
            *errorString = "Cannot change "+idQualifier+" \""+oldItem.id()
                +"\" because it is stil referenced by "
                +QString::number(sources.size())+" "+fk._sourceQualifier
                +"(s).";
            return false;
          }
          break;
        }
      }
    }
  }
  return true;
}

bool SharedUiItemDocumentManager::processBeforeCreate(
    CoreUndoCommand *command, SharedUiItem *newItem, QString idQualifier,
    QString *errorString) {
  Q_UNUSED(command)
  if (!checkIdsConstraints(*newItem, SharedUiItem(), idQualifier, errorString))
    return false;
  return true;
}

bool SharedUiItemDocumentManager::processBeforeDelete(
    CoreUndoCommand *command, SharedUiItem oldItem, QString idQualifier,
    QString *errorString) {
  Q_UNUSED(command)
  if (!checkIdsConstraints(SharedUiItem(), oldItem, idQualifier, errorString))
    return false;
  foreach (const ForeignKey &fk, _foreignKeys) {
    // foreign keys from this item : nothing to do
    // foreign keys to this item
    if (fk._referenceQualifier == idQualifier) {
      // on delete policy
      SharedUiItemList<> references =
          foreignKeySources(this, fk._sourceQualifier, fk._sourceSection, oldItem.id());
      switch (fk._onDeletePolicy) {
      case SetNull: // FIXME implement rather than fall through NoAction
      case Cascade: // FIXME implement rather than fall through NoAction
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
      }
    }
  }
  return true;
}

bool SharedUiItemDocumentManager::processAfterUpdate(
    CoreUndoCommand *command, SharedUiItem newItem, SharedUiItem oldItem,
    QString idQualifier, QString *errorString) {
  Q_UNUSED(command)
  Q_UNUSED(newItem)
  Q_UNUSED(oldItem)
  Q_UNUSED(idQualifier)
  Q_UNUSED(errorString)
  return true;
}

bool SharedUiItemDocumentManager::processAfterCreate(
    CoreUndoCommand *command, SharedUiItem newItem, QString idQualifier,
    QString *errorString) {
  Q_UNUSED(command)
  Q_UNUSED(newItem)
  Q_UNUSED(idQualifier)
  Q_UNUSED(errorString)
  return true;
}

bool SharedUiItemDocumentManager::processAfterDelete(
    CoreUndoCommand *command, SharedUiItem oldItem, QString idQualifier,
    QString *errorString) {
  Q_UNUSED(command)
  Q_UNUSED(oldItem)
  Q_UNUSED(idQualifier)
  Q_UNUSED(errorString)
  return true;
}
