/* Copyright 2015-2023 Hallowyn, Gregoire Barbier and others.
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
#include "shareduiitemdocumentmanager.h"
#include <QRandomGenerator>
#include "log/log.h"

SharedUiItemDocumentManager::SharedUiItemDocumentManager(QObject *parent)
  : QObject(parent) {
}

SharedUiItem SharedUiItemDocumentManager::itemById(
    const Utf8String &qualified_id) const {
  int pos = qualified_id.indexOf(':');
  return (pos == -1) ? itemById(Utf8String{}, qualified_id)
                     : itemById(qualified_id.left(pos), qualified_id.mid(pos+1));
}

Utf8String SharedUiItemDocumentManager::generateNewId(
    const SharedUiItemDocumentTransaction *transaction,
    const Utf8String &qualifier,
    const Utf8String &prefix) const {
  Utf8String id;
  auto effective_prefix = prefix.isEmpty() ? qualifier : prefix;
  for (int i = 1; i < 100; ++i) {
    id = effective_prefix+Utf8String::number(i);
    if (transaction) {
      if (transaction->itemById(qualifier, id).isNull())
        return id;
    } else {
      if (itemById(qualifier, id).isNull())
        return id;
    }
  }
  forever {
    id = effective_prefix+Utf8String::number(
           QRandomGenerator::global()->generate());
    if (transaction) {
      if (transaction->itemById(qualifier, id).isNull())
        return id;
    } else {
      if (itemById(qualifier, id).isNull())
        return id;
    }
  }
}

void SharedUiItemDocumentManager::reorderItems(
    const SharedUiItemList<SharedUiItem> &) {
}

void SharedUiItemDocumentManager::registerItemType(
    const Utf8String &qualifier, SharedUiItemDocumentManager::Setter setter,
    SharedUiItemDocumentManager::Creator creator) {
  _setters.insert(qualifier, setter);
  _creators.insert(qualifier, creator);
  //qDebug() << "registered" << idQualifier << this;
}

bool SharedUiItemDocumentManager::changeItem(
    const SharedUiItem &newItem, const SharedUiItem &oldItem,
    const Utf8String &qualifier, QString *errorString) {
  Q_ASSERT(newItem.isNull() || newItem.idQualifier() == qualifier);
  Q_ASSERT(oldItem.isNull() || oldItem.idQualifier() == qualifier);
  QString reason;
  if (!errorString)
    errorString = &reason;
  SharedUiItemDocumentTransaction *transaction =
      internalChangeItem(newItem, oldItem, qualifier, errorString);
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
    Utf8String idQualifier, QString *errorString) {
  SharedUiItemDocumentTransaction *transaction =
      new SharedUiItemDocumentTransaction(this);
  if (transaction->changeItem(newItem, oldItem, idQualifier, errorString)
      && delayedChecks(transaction, errorString))
    return transaction;
  delete transaction;
  return 0;
}

void SharedUiItemDocumentManager::commitChangeItem(
    const SharedUiItem &new_item, const SharedUiItem &old_item,
    const Utf8String &qualifier) {
  // should never happen since methode is = 0
  Log::error() << "SharedUiItemDocumentManager::commitChangeItem called "
                  "instead of subclass: "
               << new_item.qualifiedId() << " " << old_item.qualifiedId()
               << " " << qualifier;
}

SharedUiItem SharedUiItemDocumentManager::createNewItem(
    const Utf8String &qualifier, PostCreationModifier modifier,
    QString *errorString) {
  QString reason;
  if (!errorString)
    errorString = &reason;
  SharedUiItem newItem;
  SharedUiItemDocumentTransaction *transaction =
      internalCreateNewItem(&newItem, qualifier, modifier, errorString);
  if (transaction) {
    transaction->redo();
    delete transaction;
    return newItem;
  }
  return SharedUiItem();
}

SharedUiItemDocumentTransaction
*SharedUiItemDocumentManager::internalCreateNewItem(
    SharedUiItem *newItem, Utf8String idQualifier,
    PostCreationModifier modifier, QString *errorString) {
  Q_ASSERT(newItem != 0);
  SharedUiItemDocumentTransaction *transaction =
      new SharedUiItemDocumentTransaction(this);
  *newItem = transaction->createNewItem(idQualifier, modifier, errorString);
  if (newItem->isNull() || !delayedChecks(transaction, errorString)) {
    delete transaction;
    return 0;
  }
  return transaction;
}

bool SharedUiItemDocumentManager::changeItemByUiData(
    const SharedUiItem &old_item, int section, const QVariant &value,
    QString *errorString) {
  QString reason;
  if (!errorString)
    errorString = &reason;
  SharedUiItemDocumentTransaction *transaction = internalChangeItemByUiData(
        old_item, section, value, errorString);
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
  if (transaction->changeItemByUiData(oldItem, section, value, errorString)
      && delayedChecks(transaction, errorString))
    return transaction;
  delete transaction;
  return 0;
}

// LATER add recursive context to detect constraints loops, or check recursive
// constraints when adding them (addFK()...)

bool SharedUiItemDocumentManager::processConstraintsAndPrepareChangeItem(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
    SharedUiItem oldItem, Utf8String idQualifier, QString *errorString) {
  // to support for transforming into update an createOrUpdate call
  // and ensure that, on update or delete, oldItem is the real one, not a
  // placeholder only bearing ids such as GenericSharedUiItem, we must replace
  // oldItem by trusting only its ids
  oldItem = transaction->itemById(idQualifier, oldItem.id());
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
      if (newItem.idQualifier() == "wfactionstriggergroup")
        qDebug() << "processConstraintsAndPrepareChangeItem" << newItem.idQualifier();
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
    const Utf8String &sourceQualifier, int sourceSection,
    const Utf8String &referenceQualifier, int referenceSection,
    OnChangePolicy onUpdatePolicy, OnChangePolicy onDeletePolicy) {
  _foreignKeys.append(
        ForeignKey(sourceQualifier, sourceSection, referenceQualifier,
                   referenceSection, onDeletePolicy, onUpdatePolicy));
}

void SharedUiItemDocumentManager::addChangeItemTrigger(
    const Utf8String &qualifier, TriggerFlags flags, ChangeItemTrigger trigger){
  if (flags & BeforeUpdate)
    _triggersBeforeUpdate.insert(qualifier, trigger);
  if (flags & AfterUpdate)
    _triggersAfterUpdate.insert(qualifier, trigger);
  if (flags & BeforeCreate)
    _triggersBeforeCreate.insert(qualifier, trigger);
  if (flags & AfterCreate)
    _triggersAfterCreate.insert(qualifier, trigger);
  if (flags & BeforeDelete)
    _triggersBeforeDelete.insert(qualifier, trigger);
  if (flags & AfterDelete)
    _triggersAfterDelete.insert(qualifier, trigger);
}

bool SharedUiItemDocumentManager::checkIdsConstraints(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
    SharedUiItem oldItem, Utf8String idQualifier, QString *errorString) {
  Q_ASSERT(errorString);
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
        && !transaction->itemById(idQualifier, newItem.id()).isNull()) {
      *errorString = "New id is already used by another "+idQualifier+": "
          +newItem.id();
      return false;
    }
  }
  return true;
}

bool SharedUiItemDocumentManager::processBeforeUpdate(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem *newItem,
    SharedUiItem oldItem, Utf8String idQualifier, QString *errorString) {
  Q_UNUSED(transaction)
  if (!checkIdsConstraints(transaction, *newItem, oldItem, idQualifier,
                           errorString))
    return false;
  foreach (ChangeItemTrigger trigger, _triggersBeforeUpdate.values(idQualifier))
    if (!trigger(transaction, newItem, oldItem, idQualifier, errorString))
      return false;
  return true;
}

bool SharedUiItemDocumentManager::processBeforeCreate(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem *newItem,
    Utf8String idQualifier, QString *errorString) {
  Q_UNUSED(transaction)
  if (!checkIdsConstraints(transaction, *newItem, SharedUiItem(), idQualifier,
                           errorString))
    return false;
  foreach (ChangeItemTrigger trigger, _triggersBeforeCreate.values(idQualifier))
    if (!trigger(transaction, newItem, SharedUiItem(), idQualifier,
                 errorString))
      return false;
  return true;
}

bool SharedUiItemDocumentManager::processBeforeDelete(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem oldItem,
    Utf8String idQualifier, QString *errorString) {
  Q_UNUSED(transaction)
  if (!checkIdsConstraints(transaction, SharedUiItem(), oldItem, idQualifier,
                           errorString))
    return false;
  foreach (ChangeItemTrigger trigger,
           _triggersBeforeDelete.values(idQualifier)) {
    SharedUiItem newItem;
    if (!trigger(transaction, &newItem, oldItem, idQualifier, errorString))
      return false;
  }
  return true;
}

bool SharedUiItemDocumentManager::processAfterUpdate(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
    SharedUiItem oldItem, Utf8String idQualifier, QString *errorString) {
  foreach (const ForeignKey &fk, _foreignKeys) {
    // foreign keys referencing this item
    if (fk._referenceQualifier == idQualifier) {
      auto newReferenceId = newItem.uiUtf8(fk._referenceSection);
      auto oldReferenceId = oldItem.uiUtf8(fk._referenceSection);
      if (newReferenceId != oldReferenceId
          || fk._onUpdatePolicy == CascadeAnySection) {
        // on update policy
        switch (fk._onUpdatePolicy) {
        case Cascade:
        case CascadeAnySection:
          foreach (const SharedUiItem &oldSource,
                   transaction->foreignKeySources(
                     fk._sourceQualifier, fk._sourceSection, oldItem.id())) {
            if (!transaction->changeItemByUiData(
                  oldSource, fk._sourceSection, newReferenceId, errorString))
              return false;
          }
          break;
        case SetNull:
          foreach (const SharedUiItem &oldSource,
                   transaction->foreignKeySources(
                     fk._sourceQualifier, fk._sourceSection, oldItem.id())) {
            if (!transaction->changeItemByUiData(
                  oldSource, fk._sourceSection, QVariant(), errorString))
              return false;
          }
          break;
        case NoAction:
          break;
        }
      }
    }
  }
  foreach (ChangeItemTrigger trigger,
           _triggersAfterUpdate.values(idQualifier)) {
    SharedUiItem tmpItem = newItem;
    if (!trigger(transaction, &tmpItem, oldItem, idQualifier, errorString))
      return false;
  }
  return true;
}

bool SharedUiItemDocumentManager::processAfterCreate(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
    Utf8String idQualifier, QString *errorString) {
  foreach (ChangeItemTrigger trigger,
           _triggersAfterCreate.values(idQualifier)) {
    SharedUiItem tmpItem = newItem;
    if (!trigger(transaction, &tmpItem, SharedUiItem(), idQualifier,
                 errorString))
      return false;
  }
  return true;
}

bool SharedUiItemDocumentManager::processAfterDelete(
    SharedUiItemDocumentTransaction *transaction, SharedUiItem oldItem,
    Utf8String idQualifier, QString *errorString) {
  foreach (const ForeignKey &fk, _foreignKeys) {
    // foreign keys referencing this item
    if (fk._referenceQualifier == idQualifier) {
      // on delete policy
      switch (fk._onDeletePolicy) {
      case Cascade:
      case CascadeAnySection:
        foreach (const SharedUiItem &oldSource,
                 transaction->foreignKeySources(
                   fk._sourceQualifier, fk._sourceSection, oldItem.id())) {
          if (!transaction->changeItem(
                SharedUiItem(), oldSource, fk._sourceQualifier, errorString))
            return false;
        }
        break;
      case SetNull:
        foreach (const SharedUiItem &oldSource,
                 transaction->foreignKeySources(
                   fk._sourceQualifier, fk._sourceSection, oldItem.id())) {
          if (!transaction->changeItemByUiData(
                oldSource, fk._sourceSection, QVariant(), errorString))
            return false;
        }
        break;
      case NoAction:
        break;
      }
    }
  }
  foreach (ChangeItemTrigger trigger,
           _triggersAfterDelete.values(idQualifier)) {
    SharedUiItem newItem;
    if (!trigger(transaction, &newItem, oldItem, idQualifier, errorString))
      return false;
  }
  return true;
}

bool SharedUiItemDocumentManager::delayedChecks(
    SharedUiItemDocumentTransaction *transaction, QString *errorString) {
  // referential integrity for foreign keys referenced by changing items
  foreach (const SharedUiItem &newItem, transaction->changingItems()) {
    auto idQualifier = newItem.idQualifier();
    foreach (const ForeignKey &fk, _foreignKeys) {
      if (fk._sourceQualifier == idQualifier) {
        auto referenceId = newItem.uiUtf8(fk._sourceSection);
        //qDebug() << "referential integrity check" << newItem.id()
        //         << fk._sourceSection << referenceId << fk._referenceQualifier;
        if (!referenceId.isEmpty()
            && transaction->itemById(
              fk._referenceQualifier, referenceId).isNull()) {
          // LATER oldItemIdByChangingItem instead of newItem
          *errorString = "Cannot change "+idQualifier+" \""+newItem.id()
              +"\" because there is no "+fk._referenceQualifier+" with id \""
              +referenceId+"\".";
          return false;
        }
      }
    }
  }
  // referential integrity for foreign keys referencing original items
  foreach (const SharedUiItem &oldItem, transaction->originalItems()) {
    auto idQualifier = oldItem.idQualifier();
    foreach (const ForeignKey &fk, _foreignKeys) {
      if (fk._referenceQualifier == idQualifier) {
        auto oldReferenceId = oldItem.uiString(fk._referenceSection);
        SharedUiItemList<> newItems =
            transaction->itemsByIdQualifier(idQualifier);
        // LATER optimize this: full scan of reference for each fk is just awful
        foreach (const SharedUiItem &newItem, newItems)
          if (newItem.uiString(fk._referenceSection) == oldReferenceId)
            goto reference_still_exists;
        SharedUiItemList<> sources = transaction->foreignKeySources(
              fk._sourceQualifier, fk._sourceSection, oldItem.id());
        if (!sources.isEmpty()) {
          *errorString = "Cannot change "+idQualifier+" \""+oldItem.id()
              +"\" because it is stil referenced by "
              +QString::number(sources.size())+" "+fk._sourceQualifier
              +"(s).";
          return false;
        }
      }
reference_still_exists:;
    }
  }
  return true;
}

SharedUiItem SharedUiItemDocumentManager::itemById(
    const Utf8String&, const Utf8String&) const {
  return {};
}

SharedUiItemList<SharedUiItem> SharedUiItemDocumentManager::itemsByIdQualifier(
    const Utf8String &) const {
  return {};
}

bool SharedUiItemDocumentManager::prepareChangeItem(
    SharedUiItemDocumentTransaction *transaction,
    const SharedUiItem &new_item, const SharedUiItem &old_item,
    const Utf8String &qualifier, QString *) {
  // should never happen since methode is = 0
  Log::error() << "SharedUiItemDocumentManager::prepareChangeItem called "
                  "instead of subclass: "
               << transaction << " " << new_item.qualifiedId() << " "
               << old_item.qualifiedId() << " " << qualifier;
  return false;
}
