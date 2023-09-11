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
#ifndef SHAREDUIITEMDOCUMENTTRANSACTION_H
#define SHAREDUIITEMDOCUMENTTRANSACTION_H

#include "shareduiitemlist.h"
#include "util/coreundocommand.h"
#include <QPointer>

class SharedUiItemDocumentManager;

/** Transaction that can be used by changeItem()/prepareChangeItem()/
 * commitChangeItem() to create ChangeItemCommands and access to changes
 * performed within the transaction but not yet commited to the DM. */
class LIBP6CORESHARED_EXPORT SharedUiItemDocumentTransaction
    : public CoreUndoCommand {
  SharedUiItemDocumentManager *_dm;
  QHash<Utf8String,QHash<Utf8String,SharedUiItem>> _changingItems,
  _originalItems;

public:
  using PostCreationModifier = std::function<void(
  SharedUiItemDocumentTransaction *transaction, SharedUiItem *newItem,
  QString *errorString)>;
  class LIBP6CORESHARED_EXPORT ChangeItemCommand : public CoreUndoCommand {
    QPointer<SharedUiItemDocumentManager> _dm;
    SharedUiItem _newItem, _oldItem;
    Utf8String _qualifier;

  public:
    ChangeItemCommand(
        SharedUiItemDocumentManager *dm, const SharedUiItem &newItem,
        const SharedUiItem &oldItem, const Utf8String &qualifier,
        CoreUndoCommand *parent);
    void redo() override;
    void undo() override;
    int	id() const override;
    bool mergeWith(const CoreUndoCommand *command) override;
  };

  SharedUiItemDocumentTransaction(SharedUiItemDocumentManager *dm) : _dm(dm) { }
  SharedUiItem itemById(
      const Utf8String &qualifier, const Utf8String &id) const;
  /** Downcast blindly trusting caller that qualifier implies T */
  template <class T>
  inline T itemById(const Utf8String &qualifier, const Utf8String &id) const {
    SharedUiItem item = itemById(qualifier, id);
    return static_cast<T&>(item);
  }
  SharedUiItem itemById(const Utf8String &qualifiedId) const {
    int colon = qualifiedId.indexOf(':');
    if (colon >= 0)
        return itemById(qualifiedId.left(colon), qualifiedId.mid(colon+1));
    return SharedUiItem();
  }
  /** Perform downcast, blindly trusting caller that qualifier implies T */
  template <class T>
  inline T itemById(const Utf8String &qualifiedId) const {
    SharedUiItem item = itemById(qualifiedId);
    return static_cast<T&>(item);
  }
  SharedUiItemList itemsByQualifier(const Utf8String &qualifier) const;
  SharedUiItemList foreignKeySources(
      const Utf8String &sourceQualifier, int sourceSection,
      const Utf8String &referenceId) const;

  bool changeItemByUiData(
      const SharedUiItem &oldItem, int section, const QVariant &value,
      QString *errorString);
  bool changeItem(const SharedUiItem &newItem, const SharedUiItem &oldItem,
                  const Utf8String &qualifier, QString *errorString);
  SharedUiItem createNewItem(
      const Utf8String &qualifier, PostCreationModifier modifier,
      QString *errorString);
  /** @see SharedUiItemDocumentManager::generateNewId() */
  Utf8String generateNewId(
      const Utf8String &qualifier, const Utf8String &prefix = {}) const;

private:
  void storeItemChange(const SharedUiItem &newItem, const SharedUiItem &oldItem,
                       const Utf8String &qualifier);
  SharedUiItemList changingItems() const;
  SharedUiItemList originalItems() const;
  //SharedUiItem oldItemIdByChangingItem(SharedUiItem changingItem) const;
  friend class SharedUiItemDocumentManager;
};


#endif // SHAREDUIITEMDOCUMENTTRANSACTION_H
