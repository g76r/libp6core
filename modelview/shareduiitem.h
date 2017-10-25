/* Copyright 2014-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef SHAREDUIITEM_H
#define SHAREDUIITEM_H

#include "util/paramsprovider.h"
#include <QVariant>
#include <QString>
#include <QSharedData>
#include <QSharedDataPointer>

class QDebug;
class SharedUiItemDocumentTransaction;
class SharedUiItemParamsProvider;

/** Parent class for SharedUiItem implementation data classes.
 *
 * Subclassing guidelines:
 * - A subclass MUST implement id() and idQualifier().
 * - The id MUST be unique in the scope of the document/document manager for a
 *   given id qualifier.
 * - One of the section SHOULD represent the id for both Qt::DisplayRole and
 *   SharedUiItem::ExternalDataRole roles (this is e.g. mandatory to use
 *   SimpleDatabaseDocumentManager but is usefull in many other cases).
 *   It is convenient to use section 0 as id because in many case section 0 will
 *   be displayed by default (e.g. that's what QListView does) or as first value
 *   (e.g. QTreeView, QTableView, QColumnView).
 * - The idQualifier MUST only contains ascii letters, digits and underscore (_)
 *   and MUST start with a letter. It SHOULD be directly related to the class
 *   name, e.g. "foobar" for FoobarData. Within a given application, all
 *   possible idQualifier SHOULD NOT need case-sensitivity to be distinguished
 *   one from another.
 * - As soon as it contains data and wants it displayed, which is very likely,
 *   a subclass MUST implement uiData() and uiSectionCount() and SHOULD
 *   implement uiHeaderData() and uiSectionName(), although uiSectionName() MAY
 *   NOT be implemented since it can be deduced from uiHeaderData() as long as
 *   uiHeaderData() always return the same values (i.e. the header labels are
 *   not translated into several language).
 * - uiData() MUST handle Qt::EditRole, Qt::DisplayRole and
 *   SharedUiItem::ExternalDataRole.
 *   When planning to have only read-only UI features, Qt::EditRole SHOULD be
 *   treated as a strict equivalent of Qt::DisplayRole.
 *   Most of the time, SharedUiItem::ExternalDataRole SHOULD be treated as a
 *   strict equivalent to Qt::EditRole, however there are cases where a
 *   different format is needed for external API format (storage, transfer...)
 *   than for user interface edition.
 * - When planning to have generic UI edition features, a subclass MUST
 *   implement uiFlags() (to add Qt::ItemIsEditable flag for editable sections)
 *   and setUiData() and setUiData() MUST handle Qt::EditRole and
 *   SharedUiItem::ExternalDataRole and MAY process any role as if it were
 *   Qt::EditRole (in other words: ignore role).
 * - Subclasses MAY override operator< to provide a more natural order than
 *   qualifiedId ascii order.
 * - There MUST NOT be several level of subclasses, i.e. you must not subclass
 *   SharedUiItemData subclasses.
 * @see SharedUiItem */
// LATER update guidelines with ways to have several level of subclasses, at
// less this should require to have constructors protected in non-final levels
// of subclasses
class LIBPUMPKINSHARED_EXPORT SharedUiItemData : public QSharedData {
public:
  virtual ~SharedUiItemData();
  /** Return a string identifying the object among all other SharedUiItems
   * sharing the same idQualifier().
   *
   * The id SHOULD be unique, if it is not, keep in mind that many algorithms
   * will assume that it is, such as searching, sorting, etc. Especially within
   * Model/View classes.
   *
   * Id must not contains whitespace (regular space, newline, etc.).
   *
   * Default: return uiData(0, Qt::DisplayRole).toString() */
  virtual QString id() const;
  /** Return a string identifiying the data type represented within the
   * application, e.g. "student", "calendar", "quote". */
  virtual QString idQualifier() const = 0;
  /** Return UI sections count, like QAbstractItemModel::columnCount() does for
   * columns (anyway SharedUiItem sections are likely to be presented as
   * columns by a Model and displayed aas columns by a View).
   * Default: 0 */
  virtual int uiSectionCount() const;
  /** Return UI data, like QAbstractItemModel::data().
   * Default: QVariant()
   * Note that IdRole, IdQualifierRole and QualifiedIdRole won't be queried
   * from SharedUiItemData::uiData() but are directly handled in
   * SharedUiItem::uiData() as calls to SharedUiItemData::id() and
   * SharedUiItemData::idQualifier() instead, regardless the section. */
  virtual QVariant uiData(int section, int role) const;
  /** Return UI header data, like QAbstractItemModel::headerData().
   * Default: QVariant() */
  virtual QVariant uiHeaderData(int section, int role) const;
  /** Return UI name (id) for the section, in the sense of
   * QAbstractItemsModel::roleNames().
   * Defaults to uiHeaderData() with some cleanup processing (lowercases,
   * non alphanum to underscores...) */
  virtual QString uiSectionName(int section) const;
  /** Return UI item flags, like QAbstractItemModel::flags().
   * Default: return Qt::ItemIsEnabled | Qt::ItemIsSelectable */
  virtual Qt::ItemFlags uiFlags(int section) const;
  /** Set data from a UI point of view, i.e. called by a QAbstractItemModel
   * after user edition.
   * Default: return false
   * @return errorString must not be null
   * @return dm must not be null
   * @return true on success, false otherwise */
  virtual bool setUiData(
      int section, const QVariant &value, QString *errorString,
      SharedUiItemDocumentTransaction *transaction, int role);
  /** By default: compares identifers (idQualifier() then id()), not full
   * content, therefore two versions of an object with same identifiers will
   * have same order.
   * Implementation can rely on SharedUiItem::operator< not calling
   * SharedUiItemData::operator< with this == 0 or &other == 0. */
  virtual bool operator<(const SharedUiItemData &other) const;
  /** Calls operator<. Do not override. Override operator< instead. */
  bool operator>(const SharedUiItemData &other) const { return other<*this; }
  /** Calls operator<. Do not override. Override operator< instead. */
  bool operator<=(const SharedUiItemData &other) const { return !(other<*this); }
  /** Calls operator<. Do not override. Override operator< instead. */
  bool operator>=(const SharedUiItemData &other) const { return !(*this<other); }

protected:
  // both default and copy constructor are declared protected to avoid being
  // called directly from SharedUiItem base class within a non-const method
  // that would access d (or more precisely: that would call d.detach()),
  // because such methods would be buggy since SharedUiItem must not detach
  // directly since it's up to real data subclass to detach and copy the real
  // data class (a SharedUiItemData subclass)
  SharedUiItemData() : QSharedData() { }
  SharedUiItemData(const SharedUiItemData &other) : QSharedData(other) { }
};

/** Parent class for implicitely shared data classes to be queried as a user
 * interface item consisting of numbered sections.
 *
 * This is usefull used as data items:
 * - in QAbstractItemModel implementations targeted at table or tree views by
 *   mapping sections to columns (using one of SharedUiItemsModel
 *   implementations),
 * - in custom forms by mapping sections to widgets (using
 *   SharedUiItemWidgetMapper),
 * - in QGraphicsScene with a custom QGraphicsItem that maps sections to
 *   graphics elements, e.g. section 0 to a label title, section 3 to a
 *   subtitle, background color depending on section 7 being equal to "foo"...
 *
 * The main concept is to have all data object providing user interface info
 * through a unique contract consisting of uiDataCount() sections that can be
 * queried as QVariant through uiData() and uiHeaderData(), and, optionaly,
 * that can be user-edited through setUiData().
 *
 * SharedUiItem can be regarded as an alternative to Qt's generic UI items like
 * QTreeWidgetItem or QStandardItem, in a more model-oriented design.
 *
 * Subclassing guidelines:
 * - A subclass MUST implement default and copy constructors
 * - A subclass SHOULD implement operator=(const Foobar&) for its own type, in
 *   such a way:
 *     // in .h
 *     Foobar &operator=(const Foobar &other) {
 *       SharedUiItem::operator=(other); return *this; }
 * - A subclass MUST NOT override comparison operators (==, <, etc.)
 * - A subclass MUST be declared as Q_MOVABLE_TYPE, in such a way:
 *     // in .h, after class definition
 *     Q_DECLARE_TYPEINFO(Foobar, Q_MOVABLE_TYPE);
 *   Otherwise, the application will crash, e.g. as soon as you store subclasses
 *   in a QList<SharedUiItem>. This because SharedUiItem itself is declared
 *   Q_MOVABLE_TYPE (which is the right thing to do anyway, since it actually
 *   can be moved using memcpy()).
 * - A subclass MAY also be declared as metatype e.g. if it is intended to be
 *   sent through a signal, in such a way:
 *     // in .h, after class definition
 *     Q_DECLARE_METATYPE(Foobar)
 * - A subclass CANNOT access directly its data class and must use
 *   specializedData<>() for that. As it is tedious it SHOULD implement a const
 *   data() method that way and then use data() for short:
 *     // in .h
 *     private:
 *     const FoobarData *data() const { return specializedData<FoobarData>(); }
 * - If a subclass is not read-only, it will also need to access data through
 *   detachedData<>(), and SHOULD then implement a non-const data() method this
 *   way:
 *     // in .h
 *     private:
 *     FoobarData *data();
 *     // in .cpp
 *     FoobarData *Foobar::data() { return detachedData<FoobarData>(); }
 * - To provide a public way to detach the data, it MAY also implement a detach
 *   method in such a way:
 *     // in .h
 *     public:
 *     void detach();
 *     // in .cpp
 *     FoobarData *Foobar::detach() { detachedData<FoobarData>(); }
 * - When planning to have generic UI edition features, a subclass MUST
 *   reimplement setUiData() method, in such a way:
 *     // in .h
 *     bool setUiData(int section, const QVariant &value, QString *errorString,
 *                    SharedUiItemDocumentTransaction *transaction,
 *                    int role = Qt::EditRole);
 *     // in .cpp
 *     bool Foobar::setUiData(int section, const QVariant &value,
 *           QString *errorString, SharedUiItemDocumentTransaction *transaction,
 *           int role) {
 *       if (isNull())
 *         return false;
 *       SharedUiItem::detach<FoobarData>();
 *       return ((FoobarData*)constData())->setUiData(section, value,
 *                                              errorString, transaction, role);
 *     }
 * - There MUST NOT be several level of subclasses, i.e. you MUST NOT subclass
 *   SharedUiItem subclasses.
 *
 * @see SharedUiItemsModel
 * @see SharedUiItemsTableModel
 * @see SharedUiItemsTreeModel
 * @see SharedUiItemWidgetMapper
 * @see QStandardItem
 * @see QTreeWidgetItem
 * @see SharedUiItemData */
//
// TODO provides guidelines for building template objects for models init
// (such as Task::templateTask or TaskGroup::TaskGroup(QString))
//
// This class must have no method that calls _data.detach(), therefore no
// non-const method that accesses d, apart from operator=() and constructors,
// otherwise SharedUiItemData would be copied by QSharedData::clone() instead
// of a relevant subclass.
//
// This class has no virtual method and cannot have any, since it would prevent
// polymorphism to be handled by SharedUiItemData subclasses instead (e.g. given
// FooItem a subclass of SharedUiItem, SharedUiItem(aFooItem).aVirtualMethod()
// would not call FooItem::aVirtualMethod() but SharedUiItem::aVirtualMethod()).
//
class LIBPUMPKINSHARED_EXPORT SharedUiItem {
  QSharedDataPointer<SharedUiItemData> _data;

protected:
  explicit SharedUiItem(SharedUiItemData *data) : _data(data) { }

public:
  enum SharedUiItemRole {
    IdRole = Qt::UserRole+784,
    IdQualifierRole,
    QualifiedIdRole,
    HeaderDisplayRole,
    ExternalDataRole // for file/database storage or network transfer
  };

  SharedUiItem() { }
  SharedUiItem(const SharedUiItem &other) : _data(other._data) { }
  SharedUiItem &operator=(const SharedUiItem &other) {
    if (this != &other)
      _data = other._data;
    return *this; }
  /** Compares identifers (idQualifier() then id()), not full content, therefore
   * two versions of an object with same identifiers will be equal. */
  bool operator==(const SharedUiItem &other) const {
    return idQualifier() == other.idQualifier() && id() == other.id(); }
  bool operator!=(const SharedUiItem &other) const { return !(*this == other); }
  bool operator<(const SharedUiItem &other) const {
    return other._data && (_data ? _data->operator<(*(other._data)) : true); }
  bool operator>(const SharedUiItem &other) const { return other<*this; }
  bool operator<=(const SharedUiItem &other) const { return !(other<*this); }
  bool operator>=(const SharedUiItem &other) const { return !(*this<other); }
  bool isNull() const { return !_data; }
  /** @return !isNull() */
  operator bool() const { return !!_data; }
  /** Item identifier.
   * By convention, identifier must be unique for the same type of item within
   * the same document. */
  QString id() const { return _data ? _data->id() : QString(); }
  /** Item identifier qualifier, e.g. item type such as "invoice" for an
   * invoice data ui object, maybe QString() depending on items */
  QString idQualifier() const { return _data ? _data->idQualifier() : QString(); }
  /** Qualified item identifier.
   * By convention, qualified identifier must be unique for any type of item
   * within the same document.
   * @return idQualifier+':'+id if idQualifier is not empty, id otherwise.
   */
  static QString qualifiedId(QString idQualifier, QString id) {
    return idQualifier.isEmpty() ? id : idQualifier+':'+id; }
  /** Qualified item identifier.
   * By convention, qualified identifier must be unique for any type of item
   * within the same document.
   * @return idQualifier()+':'+id() if idQualifier is not empty, id() otherwise.
   */
  QString qualifiedId() const {
    return _data ? qualifiedId(_data->idQualifier(), _data->id()) : QString(); }
  /** Return UI sections count, like QAbstractItemModel::columnCount() does for
   * columns (anyway SharedUiItem sections are likely to be presented as
   * columns by a Model and displayed aas columns by a View). */
  int uiSectionCount() const { return _data ? _data->uiSectionCount() : 0; }
  /** Return UI data, like QAbstractItemModel::data().
   * Using IdRole, IdQualifierRole and QualifiedIdRole query
   * SharedUiItemData::id() and/or SharedUiItemData::idQualifier() instead of
   * SharedUiItem::uiData(), regardless the section.
   * Using HeaderDisplayRole query
   * SharedUiItemData::uiHeaderData(section, Qt::DisplayRole) instead of
   * SharedUiItem::uiData(). */
  QVariant uiData(int section, int role = Qt::DisplayRole) const {
    if (_data) {
      switch (role) {
      case IdRole:
        return _data->id();
      case IdQualifierRole:
        return _data->idQualifier();
      case QualifiedIdRole: {
        QString qualifier = _data->idQualifier();
        return qualifier.isEmpty() ? _data->id() : qualifier+":"+_data->id();
      }
      case HeaderDisplayRole:
        return _data->uiHeaderData(section, Qt::DisplayRole);
      default:
        return _data->uiData(section, role);
      }
    }
    return QVariant();
  }
  QVariant uiDataBySectionName(const QString &sectionName,
                               int role = Qt::DisplayRole) const {
    // LATER optimize
    if (!_data)
      return QVariant();
    int count = uiSectionCount();
    for (int section = 0; section < count; ++section) {
      QString name = uiSectionName(section);
      if (name == sectionName)
        return uiData(section, role);
    }
    return QVariant();
  }
  /** Convenience method for uiData(...).toString(). */
  QString uiString(int section, int role = Qt::DisplayRole) const {
    return uiData(section, role).toString(); }
  /** Return UI header data, like QAbstractItemModel::headerData(). */
  QVariant uiHeaderData(int section, int role) const {
    return _data ? _data->uiHeaderData(section, role) : QVariant() ; }
  /** Convenience method for uiHeaderData(...).toString(). */
  QString uiHeaderString(int section, int role = Qt::DisplayRole) const {
    return uiHeaderData(section, role).toString(); }
  /** Return UI name (id) for the section, in the sense of
   * QAbstractItemsModel::roleNames() */
  QString uiSectionName(int section) const {
    return _data ? _data->uiSectionName(section) : QString(); }
  /** Return UI item flags, like QAbstractItemModel::flags().
   *
   * Apart from very special cases, item should only set following flags:
   * Qt::ItemIsEnabled, Qt::ItemIsEditable, Qt::ItemIsUserCheckable,
   * Qt::ItemIsTristate.
   *
   * Following flags should not be set, since its the responsibility of model
   * (e.g. SharedUiItemsTableModel) to set them depending on the meaning or
   * place of the item within the model or on the capabilities of the model
   * itself: Qt::ItemNeverHasChildren, Qt::ItemIsSelectable,
   * Qt::ItemIsDragEnabled, Qt::ItemIsDropEnabled.
   *
   * Default: return Qt::ItemIsEnabled */
  Qt::ItemFlags uiFlags(int section) const {
    return _data ? _data->uiFlags(section) : Qt::NoItemFlags; }

protected:
  const SharedUiItemData *data() const { return _data.data(); }
  void setData(SharedUiItemData *data) { _data = data; }
  /** Helper template to provide a const pointer to specialized data,
   * e.g. const FooBarData *data = foobar.specializedData<FooBarData>();
   * useful to declare a const FooBarData *data() const method in FooBar class,
   * see subclassing guidelines above */
  template <class T>
  const T *specializedData() const {
    union {
      const QSharedDataPointer<SharedUiItemData> *generic;
      const QSharedDataPointer<T> *specialized;
    } pointer_alias_friendly_union;
    // the implicit reinterpret_cast done through the union is safe because the
    // static_cast at the end would fail if T wasn't a ShareUiItemData
    // reinterpret_cast mustn't be used since it triggers a "dereferencing
    // type-punned pointer will break strict-aliasing rules" warning, hence
    // using a union instead, for explicit (or gcc-friendly) aliasing
    pointer_alias_friendly_union.generic = &_data;
    return static_cast<const T*>(
          pointer_alias_friendly_union.specialized->data());
  }
  /** Helper template to detach calling the specialized data class' copy
   * constructor rather than base SharedUiItemData's one, and return a non-const
   * pointer to specialized data,
   * e.g. FooBarData *data = foobar.detachedData<FooBarData>();
   * useful to declare a "FooBarData *data()" non const method in FooBar class,
   * see subclassing guidelines above */
  template <class T>
  T *detachedData() {
    union {
      QSharedDataPointer<SharedUiItemData> *generic;
      QSharedDataPointer<T> *specialized;
    } pointer_alias_friendly_union;
    // the implicit reinterpret_cast done through the union is safe because the
    // static_cast at the end would fail if T wasn't a ShareUiItemData
    // reinterpret_cast mustn't be used since it triggers a "dereferencing
    // type-punned pointer will break strict-aliasing rules" warning, hence
    // using a union instead, for explicit (or gcc-friendly) aliasing
    pointer_alias_friendly_union.generic = &_data;
    pointer_alias_friendly_union.specialized->detach();
    return static_cast<T*>(pointer_alias_friendly_union.specialized->data());
  }
  /** Set data from a UI point of view, i.e. called by a QAbstractItemModel
   * after user edition.
   * This method must be reimplemented and made public by subclasses in order
   * to be usable.
   * It cannot be done in a generic manner in base class because non-const
   * access to data mustn't be performed in base class.
   * @return true on success, false otherwise */
  bool setUiData(int section, const QVariant &value, QString *errorString,
                 SharedUiItemDocumentTransaction *transaction,
                 int role = Qt::EditRole);
  /** Make uiData() available through ParamsProvider interface.
   * @see SharedUiItemParamsProvider */
  inline SharedUiItemParamsProvider toParamsProvider() const;
};

Q_DECLARE_METATYPE(SharedUiItem)
Q_DECLARE_TYPEINFO(SharedUiItem, Q_MOVABLE_TYPE);

/** ParamsProvider wrapper for SharedUiItem.
 * Its paramValue() implementation returns uiData(key.toInt()).
 */
class LIBPUMPKINSHARED_EXPORT SharedUiItemParamsProvider
    : public ParamsProvider {
  SharedUiItem _item;
  int _role;

public:
  inline SharedUiItemParamsProvider(
      SharedUiItem item, int role = Qt::DisplayRole)
    : _item(item), _role(role) { }
  QVariant paramValue(QString key, QVariant defaultValue = QVariant(),
                      QSet<QString> alreadyEvaluated = QSet<QString>()) const;
};

inline SharedUiItemParamsProvider SharedUiItem::toParamsProvider() const {
  return SharedUiItemParamsProvider(*this);
}

inline uint qHash(const SharedUiItem &i) { return qHash(i.id()); }

QDebug LIBPUMPKINSHARED_EXPORT operator<<(QDebug dbg, const SharedUiItem &i);

#endif // SHAREDUIITEM_H
