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
  SharedUiItemDocumentTransaction *transaction =
      internalChangeItem(newItem, oldItem, idQualifier, errorString);
  //qDebug() << "SharedUiItemDocumentManager::changeItem" << transaction;
  if (transaction) {
    transaction->redo();
    delete transaction;
    return true;
  }
  return false;
}

SharedUiItemDocumentTransaction
*SharedUiItemDocumentManager::internalChangeItem(
    SharedUiItem newItem, SharedUiItem oldItem,
    QString idQualifier, QString *errorString) {
  SharedUiItemDocumentTransaction *transaction = new SharedUiItemDocumentTransaction(this);
  QString reason;
  if (!errorString)
    errorString = &reason;
  if (processConstraintsAndPrepareChangeItem(
        transaction, newItem, oldItem, idQualifier, errorString)) {
    return transaction;
  }
  delete transaction;
  return 0;
}

void SharedUiItemDocumentManager::commitChangeItem(
    SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier) {
  emit itemChanged(newItem, oldItem, idQualifier);
}

SharedUiItem SharedUiItemDocumentManager::createNewItem(
    QString idQualifier, QString *errorString) {
  SharedUiItem newItem;
  SharedUiItemDocumentTransaction *transaction =
      internalCreateNewItem(&newItem, idQualifier, errorString);
  if (transaction) {
    transaction->redo();
    delete transaction;
    return newItem;
  }
  return SharedUiItem();
}

SharedUiItemDocumentTransaction
*SharedUiItemDocumentManager::internalCreateNewItem(
    SharedUiItem *newItem, QString idQualifier, QString *errorString) {
  Q_ASSERT(newItem != 0);
  SharedUiItemDocumentTransaction *transaction = new SharedUiItemDocumentTransaction(this);
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
            transaction, *newItem, oldItem, idQualifier, errorString)) {
        return transaction;
      }
    } else {
      *errorString = "creation of item of type "+idQualifier+" failed";
    }
  } else {
    *errorString = "no creator registered for item of type "+idQualifier;
  }
  delete transaction;
  return 0;
}

bool SharedUiItemDocumentManager::changeItemByUiData(
    SharedUiItem oldItem, int section, const QVariant &value,
    QString *errorString) {
  SharedUiItemDocumentTransaction *transaction = internalChangeItemByUiData(
        oldItem, section, value, errorString);
  if (transaction) {
    transaction->redo();
    delete transaction;
    return true;
  }
  return false;
}

SharedUiItemDocumentTransaction
*SharedUiItemDocumentManager::internalChangeItemByUiData(
    SharedUiItem oldItem, int section, const QVariant &value,
    QString *errorString) {
  SharedUiItemDocumentTransaction *transaction =
      new SharedUiItemDocumentTransaction(this);
  QString reason;
  if (!errorString)
    errorString = &reason;
  QString idQualifier = oldItem.idQualifier();
  Setter setter = _setters.value(idQualifier);
  SharedUiItem newItem = oldItem;
  if (setter) {
    // LATER always EditRole ?
    // LATER simplify constraints processing since only one section is touched
    if (setter(&newItem, section, value, errorString, transaction,
               Qt::EditRole)) {
      if (processConstraintsAndPrepareChangeItem(
            transaction, newItem, oldItem, idQualifier, errorString)) {
        return transaction;
      }
    }
  } else {
    *errorString = "No setter registred for item type "+oldItem.idQualifier();
  }
  delete transaction;
  return 0;
}

bool SharedUiItemDocumentManager::processConstraintsAndPrepareChangeItem(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem, SharedUiItem oldItem,
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
      return processBeforeCreate(transaction, &newItem, idQualifier, errorString)
          && prepareChangeItem(transaction, newItem, oldItem, idQualifier,
                               errorString)
          && processAfterCreate(transaction, newItem, idQualifier, errorString);
    }
  } else {
    if (newItem.isNull()) { // delete
      return processBeforeDelete(transaction, oldItem, idQualifier, errorString)
          && prepareChangeItem(transaction, newItem, oldItem, idQualifier,
                               errorString)
          && processAfterDelete(transaction, oldItem, idQualifier, errorString);
    } else { // update (incl. renaming)
      return processBeforeUpdate(transaction, &newItem, oldItem, idQualifier,
                                 errorString)
          && prepareChangeItem(transaction, newItem, oldItem, idQualifier,
                               errorString)
          && processAfterUpdate(transaction, newItem, oldItem, idQualifier,
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
    SharedUiItemDocumentTransaction *transaction, SharedUiItem *newItem, SharedUiItem oldItem,
    QString idQualifier, QString *errorString) {
  Q_UNUSED(transaction)
  if (!checkIdsConstraints(*newItem, oldItem, idQualifier, errorString))
    return false;
  foreach (const ForeignKey &fk, _foreignKeys) {
    // foreign keys from this item
    if (fk._sourceQualifier == idQualifier) {
      // referential integrity
      QString referenceId = newItem->uiString(fk._sourceSection);
      if (!referenceId.isEmpty()
          && transaction->itemById(
            fk._referenceQualifier, referenceId).isNull()) {
        *errorString = "Cannot change "+idQualifier+" \""+oldItem.id()
            +"\" because there is no "+fk._referenceQualifier+" with id \""
            +referenceId+"\".";
        return false;
      }
    }
    // foreign keys to this item
    if (fk._referenceQualifier == idQualifier) {
      QString newReferenceId = newItem->uiString(fk._referenceSection);
      QString oldReferenceId = oldItem.uiString(fk._referenceSection);
      if (newReferenceId != oldReferenceId) {
        // on update policy
        SharedUiItemList<> sources = transaction->foreignKeySources(
              fk._sourceQualifier, fk._sourceSection, oldItem.id());
        switch (fk._onUpdatePolicy) {
        case CascadeReferencedKey:
        case CascadeAnySection:
          break;
        case SetNull:
          // LATER implement rather than fall through NoAction
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
    SharedUiItemDocumentTransaction *transaction, SharedUiItem *newItem, QString idQualifier,
    QString *errorString) {
  Q_UNUSED(transaction)
  if (!checkIdsConstraints(*newItem, SharedUiItem(), idQualifier, errorString))
    return false;
  return true;
}

bool SharedUiItemDocumentManager::processBeforeDelete(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem oldItem, QString idQualifier,
    QString *errorString) {
  Q_UNUSED(transaction)
  if (!checkIdsConstraints(SharedUiItem(), oldItem, idQualifier, errorString))
    return false;
  foreach (const ForeignKey &fk, _foreignKeys) {
    // foreign keys from this item : nothing to do
    // foreign keys to this item
    if (fk._referenceQualifier == idQualifier) {
      // on delete policy
      SharedUiItemList<> sources = transaction->foreignKeySources(
            fk._sourceQualifier, fk._sourceSection, oldItem.id());
      switch (fk._onDeletePolicy) {
      case SetNull:
        // LATER implement rather than fall through NoAction
      case CascadeReferencedKey:
      case CascadeAnySection:
        // LATER implement rather than fall through NoAction
      case Unknown:
      case NoAction:
        if (!sources.isEmpty()) {
          *errorString = "Cannot delete "+idQualifier+" \""+oldItem.id()
              +"\" because it is stil referenced by "
              +QString::number(sources.size())+" "+fk._sourceQualifier
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
    SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem, SharedUiItem oldItem,
    QString idQualifier, QString *errorString) {
  foreach (const ForeignKey &fk, _foreignKeys) {
    // foreign keys to this item
    if (fk._referenceQualifier == idQualifier) {
      QString newReferenceId = newItem.uiString(fk._referenceSection);
      QString oldReferenceId = oldItem.uiString(fk._referenceSection);
      if (newReferenceId != oldReferenceId
          || fk._onUpdatePolicy == CascadeAnySection) {
        // on update policy
        SharedUiItemList<> sources = transaction->foreignKeySources(
              fk._sourceQualifier, fk._sourceSection, oldItem.id());
        switch (fk._onUpdatePolicy) {
        case CascadeReferencedKey:
        case CascadeAnySection:
          foreach (const SharedUiItem &oldSource, sources) {
            SharedUiItem newSource = oldSource;
            Setter setter = _setters.value(fk._sourceQualifier);
            if (!setter) {
              *errorString = "No setter registred for item type "
                  +fk._sourceQualifier;
              return false;
            }
            if (!setter(&newSource, fk._sourceSection, newReferenceId,
                        errorString, transaction, Qt::EditRole))
              return false;
            if (!processConstraintsAndPrepareChangeItem(
                  transaction, newSource, oldSource, fk._sourceQualifier,
                  errorString))
              return false;
          }
          break;
        case SetNull:
          // LATER implement rather than fall through NoAction
        case Unknown:
        case NoAction:
          break;
        }
      }
    }
  }
  return true;
}

bool SharedUiItemDocumentManager::processAfterCreate(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem, QString idQualifier,
    QString *errorString) {
  Q_UNUSED(transaction)
  Q_UNUSED(newItem)
  Q_UNUSED(idQualifier)
  Q_UNUSED(errorString)
  return true;
}

bool SharedUiItemDocumentManager::processAfterDelete(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem oldItem, QString idQualifier,
    QString *errorString) {
  Q_UNUSED(transaction)
  Q_UNUSED(oldItem)
  Q_UNUSED(idQualifier)
  Q_UNUSED(errorString)
  return true;
}
