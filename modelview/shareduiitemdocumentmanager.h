/* Copyright 2015-2024 Hallowyn, Gregoire Barbier and others.
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
#ifndef SHAREDUIITEMDOCUMENTMANAGER_H
#define SHAREDUIITEMDOCUMENTMANAGER_H

#include "shareduiitemdocumenttransaction.h"

/** Base class for SharedUiItem-based document managers.
 *
 * Quick implementation guide:
 * Document manager Implementations SHOULD implement/override this methods:
 * - prepareChangeItem()
 * - commitChangeItem()
 * - itemById() (taking care not to hide overloaded forms)
 * - itemsByQualifier() (taking care not to hide overloaded forms)
 *
 * @see SharedUiItem */
class LIBP6CORESHARED_EXPORT SharedUiItemDocumentManager : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemDocumentManager)

public:
  using Setter = std::function<bool(SharedUiItem *item, int section,
  const QVariant &value, QString *errorString,
  SharedUiItemDocumentTransaction *transaction, int role)>;
  // following adapter is needed because members pointer are not convertible
  // to base class member pointer, therefore &Foobar::setUiData cannot be
  // holded by function<bool(SharedUiItem*...)> even if Foobar is a subclass
  // of SharedUiItem
  template <class T>
  using MemberSetter = bool (T::*)(int section, const QVariant &value,
  QString *errorString, SharedUiItemDocumentTransaction *transaction,
  int role);
  using Creator = std::function<SharedUiItem(
  SharedUiItemDocumentTransaction *transaction, const Utf8String &id,
  QString *errorString)>;
  using SimplestCreator = std::function<SharedUiItem(const Utf8String &id)>;
  using PostCreationModifier =
  SharedUiItemDocumentTransaction::PostCreationModifier;
  using ChangeItemTrigger = std::function<bool(
  SharedUiItemDocumentTransaction *transaction, SharedUiItem *new_item,
  const SharedUiItem &old_item, const Utf8String &qualifier,
  QString *errorString)>;
  enum OnChangePolicy {
    /** no automatic action on update or delete */
    NoAction,
    /** set referencing section to {} on referencing item */
    SetNull,
    /** on update: update referencing section on referencing item,
     * on delete: delete referencing item */
    Cascade,
    /** on update: update referencing section on referencing item every time
     * referenced item changes, even if changing sections do not include
     * referenced key section,
     * on delete: same as Cascade */
    CascadeAnySection
  };
  enum TriggerFlag {
    NoTrigger = 0,
    BeforeUpdate = 1,
    AfterUpdate = 2,
    BeforeCreate = 4,
    AfterCreate = 8,
    BeforeDelete = 16,
    AfterDelete = 32
    // LATER add BeforeCommit trigger for integrity checks
  };
  Q_DECLARE_FLAGS(TriggerFlags, TriggerFlag)
  Q_FLAGS(TriggerFlags)

private:
  struct ForeignKey {
    Utf8String _sourceQualifier;
    int _sourceSection;
    Utf8String _referenceQualifier;
    int _referenceSection;
    OnChangePolicy _onDeletePolicy;
    OnChangePolicy _onUpdatePolicy;
    ForeignKey(const Utf8String &sourceQualifier, int sourceSection,
               const Utf8String &referenceQualifier, int referenceSection,
               OnChangePolicy onDeletePolicy, OnChangePolicy onUpdatePolicy)
      : _sourceQualifier(sourceQualifier), _sourceSection(sourceSection),
        _referenceQualifier(referenceQualifier),
        _referenceSection(referenceSection), _onDeletePolicy(onDeletePolicy),
        _onUpdatePolicy(onUpdatePolicy) { }
  };

protected:
  QHash<Utf8String,Setter> _setters;
  QHash<Utf8String,Creator> _creators;
  QList<ForeignKey> _foreignKeys;
  QMultiHash<Utf8String,ChangeItemTrigger> _triggersBeforeUpdate;
  QMultiHash<Utf8String,ChangeItemTrigger> _triggersAfterUpdate;
  QMultiHash<Utf8String,ChangeItemTrigger> _triggersBeforeCreate;
  QMultiHash<Utf8String,ChangeItemTrigger> _triggersAfterCreate;
  QMultiHash<Utf8String,ChangeItemTrigger> _triggersBeforeDelete;
  QMultiHash<Utf8String,ChangeItemTrigger> _triggersAfterDelete;

  explicit SharedUiItemDocumentManager(QObject *parent = nullptr);

public:
  /** Method that user interface should call to create a new default item with
   * an automatically generated unique id.
   *
   * Actually, this method create a Transaction and calls item type creator
   * which is the right place to customize item creation from document manager
   * implementations.
   * No other class than DtpDocumentManager should override createNewItem().
   */
  virtual SharedUiItem createNewItem(
      const Utf8String &qualifier, PostCreationModifier modifier = nullptr,
      QString *errorString = nullptr);
  /** Perform downcast, blindly trusting caller that qualifier implies T */
  template<class T>
  inline T createNewItem(
      const Utf8String &qualifier, PostCreationModifier modifier = nullptr,
      QString *errorString = nullptr) {
    SharedUiItem item = createNewItem(qualifier, modifier, errorString);
    return static_cast<T&>(item);
  }
  /** Convenience method with modifier = 0 */
  inline SharedUiItem createNewItem(
      const Utf8String &qualifier, QString *errorString) {
    return createNewItem(qualifier, nullptr, errorString);
  }
  /** Perform downcast, blindly trusting caller that qualifier implies T */
  template<class T>
  inline T createNewItem(const Utf8String &qualifier, QString *errorString) {
    SharedUiItem item = createNewItem(qualifier, nullptr, errorString);
    return static_cast<T&>(item);
  }
  /** Method that user interface should call to change an item, one field at a
   * time.
   *
   * Suited for model/view edition.
   *
   * Actually, this method creates a Transaction and calls
   * prepareChangeItem() which is the method to override by document manager
   * implementations.
   * No other class than DtpDocumentManager should override changeItem().
   */
  virtual bool changeItemByUiData(
      const SharedUiItem &old_item, int section, const QVariant &value,
      QString *errorString = nullptr);
  /** Method that user interface or non-interactive code should call to change
   *  an item at whole.
   *
   * It is up to the caller to warn the user on error.
   *
   * If oldItem is found in data and newItem.isNull(), delete old item.
   * If oldItem is found in data and newItem is not null, update the item,
   * taking care that old and new item ids may be different is the item is
   * being renamed.
   * If oldItem is not found in data and new item is not null, create it,
   * regardless oldItem is null or garbage or equals to newItem.
   *
   * This method perform change on actual data, while
   * SharedUiItemDocumentManager::changeItem() only perform change in a model
   * data (and thus should not be called directly but rather connected to
   * SharedUiItemDocumentManager::itemChanged() which is emited just after
   * actual data change).
   *
   * Actually, this method create a Transaction and calls
   * prepareChangeItem() which is the method to override by document manager
   * implementations.
   * No other class than DtpDocumentManager should override changeItem().
   */
  virtual bool changeItem(
      const SharedUiItem &newItem, const SharedUiItem &oldItem,
      const Utf8String &qualifier, QString *errorString = nullptr);
  virtual SharedUiItem itemById(
      const Utf8String &qualifier, const Utf8String &id) const = 0;
  /** Default: parses qualifiedId and calls itemById(Utf8String,Utf8String). */
  virtual SharedUiItem itemById(const Utf8String &qualified_id) const;
  /** Perform downcast, blindly trusting caller that qualifier implies T */
  template<class T>
  T itemById(const Utf8String &qualifier, const Utf8String &id) const {
    auto item = itemById(qualifier, id);
    return static_cast<T&>(item);
  }
  /** Perform downcast, blindly trusting caller that qualifier implies T */
  template<class T>
  T itemById(const Utf8String &qualified_id) const {
    auto item = itemById(qualified_id);
    return static_cast<T&>(item);
  }
  /** This method build a list of every item currently holded, given their
   *  qualifiedId, can bee very expensive depending of the data set size. */
  virtual SharedUiItemList itemsByQualifier(
      const Utf8String &qualifier) const = 0;
  /** Change items order.
   *
   * Items list may contain a mix of several items type (i.e. with different
   * id qualifiers) and may be partial (i.e. not contains all items of a given
   * type).
   * In most cases items order are not significant but some document managers
   * may keep memory of their order (or of the orders of some item types).
   * Default implementation does nothing.
   */
  virtual void reorderItems(const SharedUiItemList &items);
  /** This method must be called for every item type the document manager will
   * hold, to enable it to create and modify such items. */
  void registerItemType(
      const Utf8String &qualifier, Setter setter, Creator creator);
  /** Convenience method. */
  inline void registerItemType(
      const Utf8String &qualifier, Setter setter, SimplestCreator creator) {
    registerItemType(qualifier, setter, [creator](
                     SharedUiItemDocumentTransaction *, Utf8String id,
                     QString *) { return creator(id); });
  }
  /** Perform downcast, blindly trusting caller that qualifier implies T */
  template <class T>
  inline void registerItemType(
      const Utf8String &qualifier, MemberSetter<T> setter, Creator creator) {
    registerItemType(qualifier, [setter](SharedUiItem *item, int section,
                     const QVariant &value, QString *errorString,
                     SharedUiItemDocumentTransaction *transaction, int role ){
      return (item->*static_cast<MemberSetter<SharedUiItem>>(setter))(
            section, value, errorString, transaction, role);
    }, creator);
  }
  /** Perform downcast, blindly trusting caller that qualifier implies T */
  template <class T>
  void registerItemType(
      const Utf8String &qualifier, MemberSetter<T> setter,
      SimplestCreator creator) {
    registerItemType(qualifier, [setter](SharedUiItem *item, int section,
                     const QVariant &value, QString *errorString,
                     SharedUiItemDocumentTransaction *transaction, int role ){
      return (item->*static_cast<MemberSetter<SharedUiItem>>(setter))(
            section, value, errorString, transaction, role);
    }, [creator](SharedUiItemDocumentTransaction *, Utf8String id, QString *) {
      return creator(id);
    });
  }
  // FIXME doc
  void addForeignKey(
      const Utf8String &sourceQualifier, int sourceSection,
      const Utf8String &referenceQualifier, int referenceSection = 0,
      OnChangePolicy onUpdatePolicy = NoAction,
      OnChangePolicy onDeletePolicy = NoAction);
  // FIXME doc
  void addChangeItemTrigger(const Utf8String &qualifier, TriggerFlags flags,
      ChangeItemTrigger trigger);
  /** Can be called to generate a new id not currently in use for the given
   * qualifier item type.
   * Generate id of the form prefix+number (e.g. "foobar1"), most of the time
   * one should choose qualifier as prefix, which is the default (= if prefix
   * is left empty). */
  Utf8String generateNewId(const Utf8String &qualifier,
                           const Utf8String &prefix = {}) const {
    return generateNewId(nullptr, qualifier, prefix);
  }

signals:
  /** Emited whenever an item changes.
   *
   * When an item is created oldItem.isNull().
   * When an item is destroyed newItem.isNull().
   * When an item is renamed (changes id) newItem.id() != oldItem.id().
   * Otherwise the item is updated (changes anything apart its id).
   *
   * Notes: oldItem and newItem cannot be null at the same time, if none is null
   * then oldItem.qualifier() == newItem.qualifier().
   *
   * Subclasses may add specialized signals for their different item
   * types, e.g. "void customerChanged(Customer newItem, Customer oldItem)"
   * where Customer is a SharedUiItem, however using the generic itemChanged()
   * signal is often more powerful and therefore most of the time they should
   * not specialize signals.
   */
  void itemChanged(const SharedUiItem &new_item, const SharedUiItem &old_item,
                   const Utf8String &qualifier);
  /** Emited when all data is reset as a whole, for instance when switching
   * from a document to another one. */
  void dataReset();

protected:
  /** Can be called by createNewItem() implementations to generate a new id not
   * currently in use for the given qualifier item type.
   * Generate id of the form prefix+number (e.g. "foobar1"), most of the time
   * one should choose qualifier as prefix, which is the default (= if prefix
   * is left empty). */
  Utf8String generateNewId(const SharedUiItemDocumentTransaction *transaction,
      const Utf8String &qualifier, const Utf8String &prefix = {}) const;
  /** Prepare change: ensure that the change can be performed, record them
   * through transaction->changeItem(), and return true only on success.
   *
   * Although changeItem() is more tolerant, this method is not needed to
   * support createOrUpdate or deleteIfExist semantics, since it is called
   * by changeItem() which ensure that only the following parameters conditions
   * can occur for prepareChangeItem():
   * - delete: newItem.isNull() and !oldItem.isNull(), oldItem is a currently
   *     holded data item, not a placeholder only bearing ids.
   * - create: !newItem.isNull() and oldItem.isNull(), there is not currently
   *     any data item matching newItem.qualifiedId()
   * - update (incl. rename): neither is null, oldItem is a currently holded
   *     data item
   *
   * @param errorString is guaranted not to be null
   */
  virtual bool prepareChangeItem(
      SharedUiItemDocumentTransaction *transaction,
      const SharedUiItem &new_item, const SharedUiItem &old_item,
      const Utf8String &qualifier, QString *errorString) = 0;
  /** Perform actual change and emit itemChanged().
   *
   * Implementations MUST emit itemChanged() signal method after performing
   * actual change.
   *
   * This method is called just after prepareChangeItem() returned true, and
   * whenever the transaction (or associated QUndoCommand) is undone or redone.
   * Since both old and new items are kept in the transaction, commitChangeItem
   * is called with its parameters swapped to undo what has been (re)done.
   *
   * Method commitChangeItem() can even be called several times per
   * prepareChangeItem() call since prepareChangeItem() may add several changes,
   * and constraints (especially foreign keys) or triggers may also add
   * additional changes. */
  virtual void commitChangeItem(
      const SharedUiItem &new_item, const SharedUiItem &old_item,
      const Utf8String &qualifier) = 0;
  // FIXME doc
  void storeItemChange(
      SharedUiItemDocumentTransaction *transaction,
      const SharedUiItem &new_item,
      const SharedUiItem &old_item, const Utf8String &qualifier) {
    transaction->storeItemChange(new_item, old_item, qualifier);
  }

  /** To be called by createNewItem().
   * Should never be overriden apart by DtpDocumentManagerWrapper. */
  virtual SharedUiItemDocumentTransaction *internalCreateNewItem(
      SharedUiItem *newItem, Utf8String qualifier,
      PostCreationModifier modifier, QString *errorString);
  /** To be called by changeItem().
   * Should never be overriden apart by DtpDocumentManagerWrapper. */
  virtual SharedUiItemDocumentTransaction *internalChangeItem(
      SharedUiItem newItem, SharedUiItem oldItem, Utf8String qualifier,
      QString *errorString);
  /** To be called by changeItemByUiData().
   * Should never be overriden apart by DtpDocumentManagerWrapper. */
  virtual SharedUiItemDocumentTransaction *internalChangeItemByUiData(
      SharedUiItem oldItem, int section, const QVariant &value,
      QString *errorString);

private:
  // FIXME doc
  bool processConstraintsAndPrepareChangeItem(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
      SharedUiItem oldItem, Utf8String qualifier, QString *errorString);
  // FIXME doc×6
  bool processBeforeUpdate(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem *newItem,
      SharedUiItem oldItem, Utf8String qualifier, QString *errorString);
  bool processBeforeCreate(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem *newItem,
      Utf8String qualifier, QString *errorString);
  bool processBeforeDelete(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem oldItem,
      Utf8String qualifier, QString *errorString);
  bool processAfterUpdate(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
      SharedUiItem oldItem, Utf8String qualifier, QString *errorString);
  bool processAfterCreate(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem newItem,
      Utf8String qualifier, QString *errorString);
  bool processAfterDelete(
      SharedUiItemDocumentTransaction *transaction, SharedUiItem oldItem,
      Utf8String qualifier, QString *errorString);
  // id as primary key (uniqueness, not empty, same qualifiers before and after)
  bool checkIdsConstraints(SharedUiItemDocumentTransaction *transaction,
                           SharedUiItem newItem, SharedUiItem oldItem,
                           Utf8String qualifier, QString *errorString);
  /** Perform checks that are delayed until commit, such as referential
   * integrity checks. */
  bool delayedChecks(SharedUiItemDocumentTransaction *transaction,
                     QString *errorString);

  friend class SharedUiItemDocumentTransaction; // needed for many methods and fields
  friend class SharedUiItemDocumentTransaction::ChangeItemCommand; // needed to call back commitChangeItem()
  friend class DtpDocumentManagerWrapper; // needed to wrapp internalChangeItem
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SharedUiItemDocumentManager::TriggerFlags)

// LATER Q_DECLARE_TYPEINFO(SharedUiItemDocumentManager::ForeignKey, Q_RELOCATABLE_TYPE);

#endif // SHAREDUIITEMDOCUMENTMANAGER_H
