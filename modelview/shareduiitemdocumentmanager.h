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
#ifndef SHAREDUIITEMDOCUMENTMANAGER_H
#define SHAREDUIITEMDOCUMENTMANAGER_H

#include <QObject>
#include "libqtssu_global.h"
#include "modelview/shareduiitem.h"
#include "shareduiitemlist.h"
#include <functional>
#include "util/coreundocommand.h"
#include <QPointer>

/** Base class for SharedUiItem-based document managers.
 *
 * Quick implementation guide:
 * Document manager Implementations SHOULD implement/override this methods:
 * - prepareChangeItem()
 * - commitChangeItem()
 * - itemById() (taking care not to hide overloaded forms)
 * - itemsByIdQualifier() (taking care not to hide overloaded forms)
 *
 * @see SharedUiItem */
class LIBQTSSUSHARED_EXPORT SharedUiItemDocumentManager : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(SharedUiItemDocumentManager)

public:
  using Setter = bool (SharedUiItem::*)(int, const QVariant &, QString *, int,
  const SharedUiItemDocumentManager *);
  using Creator = std::function<SharedUiItem(QString)>;
  enum OnChangePolicy { Unknown, NoAction, SetNull, Cascade };

private:
  struct ForeignKey {
    QString _sourceQualifier;
    int _sourceSection;
    QString _referenceQualifier;
    int _referenceSection;
    OnChangePolicy _onDeletePolicy;
    OnChangePolicy _onUpdatePolicy;
    bool _nullable;
    ForeignKey(QString sourceQualifier, int sourceSection,
               QString referenceQualifier, int referenceSection,
               OnChangePolicy onDeletePolicy, OnChangePolicy onUpdatePolicy,
               bool nullable)
      : _sourceQualifier(sourceQualifier), _sourceSection(sourceSection),
        _referenceQualifier(referenceQualifier),
        _referenceSection(referenceSection), _onDeletePolicy(onDeletePolicy),
        _onUpdatePolicy(onUpdatePolicy), _nullable(nullable) { }
  };

protected:
  QHash<QString,Setter> _setters;
  QHash<QString,Creator> _creators;
  QList<ForeignKey> _foreignKeys;

public:
  explicit SharedUiItemDocumentManager(QObject *parent = 0);
  /** Method that user interface should call to create a new default item with
   * an automatically generated unique id.
   *
   * Actually, this method create a CoreUndoCommand and calls item type creator
   * which is the right place to customize item creation from document manager
   * implementations.
   * No other class than DtpDocumentManager should override createNewItem().
   */
  virtual SharedUiItem createNewItem(
      QString idQualifier, QString *errorString = 0);
  /** Method that user interface should call to change an item, one field at a
   * time.
   *
   * Suited for model/view edition.
   *
   * Actually, this method create a CoreUndoCommand and calls
   * prepareChangeItem() which is the method to override by document manager
   * implementations.
   * No other class than DtpDocumentManager should override changeItem().
   */
  virtual bool changeItemByUiData(
      SharedUiItem oldItem, int section, const QVariant &value,
      QString *errorString = 0);
  /** Method that user interface or non-interactive code should call to change
   *
   * an item at whole.
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
   * Actually, this method create a CoreUndoCommand and calls
   * prepareChangeItem() which is the method to override by document manager
   * implementations.
   * No other class than DtpDocumentManager should override changeItem().
   */
  virtual bool changeItem(SharedUiItem newItem, SharedUiItem oldItem,
                          QString idQualifier, QString *errorString = 0);
  virtual SharedUiItem itemById(QString idQualifier, QString id) const = 0;
  /** Default: parses qualifiedId and calls itemById(QString,QString). */
  virtual SharedUiItem itemById(QString qualifiedId) const;
  /** Convenience template performing downcast. */
  template<class T>
  T itemById(QString idQualifier, QString id) const {
    SharedUiItem item = itemById(idQualifier, id);
    return static_cast<T&>(item);
  }
  /** Convenience template performing downcast. */
  template<class T>
  T itemById(QString qualifiedId) const {
    SharedUiItem item = itemById(qualifiedId);
    return static_cast<T&>(item);
  }
  /** This method build a list of every item currently holded, given their
   * qualifiedId. */
  virtual SharedUiItemList<SharedUiItem> itemsByIdQualifier(
      QString idQualifier) const = 0;
  /** This method build a list of every item currently holded, given their class
   * (T) and qualifiedId. */
  template<class T>
  SharedUiItemList<T> itemsByIdQualifier(QString idQualifier) const {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
    SharedUiItemList<SharedUiItem> list = itemsByIdQualifier(idQualifier);
    if (!list.isEmpty() && list[0].idQualifier() != idQualifier) {
      // LATER output warning
      //qWarning() << "SharedUiItemList<T>::itemsByIdQualifier called with "
      //              "inconsistent types and qualifier";
      return SharedUiItemList<T>();
    }
    return *reinterpret_cast<SharedUiItemList<T>*>(&list);
  }
  /** Change items order.
   *
   * Items list may contain a mix of several items type (i.e. with different
   * id qualifiers) and may be partial (i.e. not contains all items of a given
   * type).
   * In most cases items order are not significant but some document managers
   * may keep memory of their order (or of the orders of some item types).
   * Default implementation does nothing.
   */
  virtual void reorderItems(QList<SharedUiItem> items);
  /** This method must be called for every item type the document manager will
   * hold, to enable it to create and modify such items. */
  void registerItemType(QString idQualifier, Setter setter, Creator creator);
  // FIXME doc
  void addForeignKey(QString sourceQualifier, int sourceSection,
                     QString referenceQualifier, int referenceSection = 0,
                     OnChangePolicy onDeletePolicy = NoAction,
                     OnChangePolicy onUpdatePolicy = NoAction,
                     bool nullable = true);
signals:
  /** Emited whenever an item changes.
   *
   * When an item is created oldItem.isNull().
   * When an item is destroyed newItem.isNull().
   * When an item is renamed (changes id) newItem.id() != oldItem.id().
   * Otherwise the item is updated (changes anything apart its id).
   *
   * Notes: oldItem and newItem cannot be null at the same time, if none is null
   * then oldItem.idQualifier() == newItem.idQualifier().
   *
   * Subclasses may (should?) add specialized signals for their different item
   * types, e.g. "void customerChanged(Customer newItem, Customer oldItem)"
   * where Customer is a SharedUiItem.
   */
  void itemChanged(SharedUiItem newItem, SharedUiItem oldItem,
                   QString idQualifier);

protected:
  /** Command to be used by changeItem()/prepareChangeItem()/commitChangeItem()
   * as child for command received as parameter. */
  class LIBQTSSUSHARED_EXPORT ChangeItemCommand : public CoreUndoCommand {
    QPointer<SharedUiItemDocumentManager> _dm;
    SharedUiItem _newItem, _oldItem;
    QString _idQualifier;
    bool _ignoreFirstRedo;

  public:
    /**
     * @param alreadyCommitedBeforeFirstRedo set to true if change has already
     *   been performed, in this case first redo() (i.e. the one performed when
     *   pushing QUndoCommand to QUndoStack) call will be ignored.
     */
    ChangeItemCommand(SharedUiItemDocumentManager *dm, SharedUiItem newItem,
                      SharedUiItem oldItem, QString idQualifier,
                      CoreUndoCommand *parent/*,
                      bool alreadyCommitedBeforeFirstRedo = false*/);
    void redo();
    void undo();
  };

  /** Can be called by createNewItem() implementations to generate a new id not
   * currently in use for the given idQualifier item type.
   * Generate id of the form idQualifier+number (e.g. "foobar1"). */
  QString genererateNewId(QString idQualifier);
  /** Prepare change: ensure that the change can be performed, add
   * ChangeItemCommand children as needed to command, and return true only on
   * success.
   * It is better for prepareChangeItem() no to perform any change and to
   * pospone all of them until commitChangeItem() is called. However, depending
   * on the way data is store, this may not be possible. This is why
   * ChangeItemCommand supports alreadyCommitedBeforeFirstRedo parameter.
   * If prepareChangeItem() perform changes and finaly detects that the whole
   * change is not possible, it must manage restore any changed data to its
   * previous state before returning false.
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
  virtual bool prepareChangeItem(CoreUndoCommand *command, SharedUiItem newItem,
                                 SharedUiItem oldItem, QString idQualifier,
                                 QString *errorString) = 0;
  /** Perform actual change and emit itemChanged().
   *
   * Default impl only emit signal. Implementations must called base class
   * method after performing actual change.
   *
   * This method is called just after prepareChangeItem() returned true, and
   * whenever associated undo command is undone or redone.
   *
   * commitChangeItem() can be called several times per prepareChangeItem()
   * call since prepareChangeItem() may add several commands and constraints
   * (especially foreign keys) may also add additional commands. */
  virtual void commitChangeItem(SharedUiItem newItem, SharedUiItem oldItem,
                                QString idQualifier);

protected:
  /** To be called by createNewItem().
   * Should never be overriden apart by DtpDocumentManagerWrapper. */
  virtual CoreUndoCommand *internalCreateNewItem(
      SharedUiItem *newItem, QString idQualifier, QString *errorString);
  /** To be called by changeItem().
   * Should never be overriden apart by DtpDocumentManagerWrapper. */
  virtual CoreUndoCommand *internalChangeItem(
      SharedUiItem newItem, SharedUiItem oldItem, QString idQualifier,
      QString *errorString);
  /** To be called by changeItemByUiData().
   * Should never be overriden apart by DtpDocumentManagerWrapper. */
  virtual CoreUndoCommand *internalChangeItemByUiData(
      SharedUiItem oldItem, int section, const QVariant &value,
      QString *errorString);

private:
  // FIXME doc
  bool processConstraintsAndPrepareChangeItem(
      CoreUndoCommand *command, SharedUiItem newItem, SharedUiItem oldItem,
      QString idQualifier, QString *errorString);
  // FIXME doc×6
  bool processBeforeUpdate(
      CoreUndoCommand *command, SharedUiItem *newItem, SharedUiItem oldItem,
      QString idQualifier, QString *errorString);
  bool processBeforeCreate(
      CoreUndoCommand *command, SharedUiItem *newItem, QString idQualifier,
      QString *errorString);
  bool processBeforeDelete(
      CoreUndoCommand *command, SharedUiItem oldItem, QString idQualifier,
      QString *errorString);
  bool processAfterUpdate(
      CoreUndoCommand *command, SharedUiItem newItem, SharedUiItem oldItem,
      QString idQualifier, QString *errorString);
  bool processAfterCreate(
      CoreUndoCommand *command, SharedUiItem newItem, QString idQualifier,
      QString *errorString);
  bool processAfterDelete(
      CoreUndoCommand *command, SharedUiItem oldItem, QString idQualifier,
      QString *errorString);

  friend class ChangeItemCommand; // needed to call back commitChangeItem()
  friend class DtpDocumentManagerWrapper; // needed to wrapp internalChangeItem
};

// LATER Q_DECLARE_TYPEINFO(SharedUiItemDocumentManager::ForeignKey, Q_MOVABLE_TYPE);

#endif // SHAREDUIITEMDOCUMENTMANAGER_H

