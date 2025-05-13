/* Copyright 2014-2025 Hallowyn, Gregoire Barbier and others.
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

#include <QSharedDataPointer>
#include <QJsonObject>
#include "util/paramsprovider.h"
#include "util/containerutils.h"
#include "util/utf8stringlist.h"
#include "util/utf8stringset.h"
#include "util/radixtree.h"

class QDebug;
class SharedUiItemDocumentTransaction;
class SharedUiItem;
class SharedUiItemData;

template <typename T>
concept shareduiitemdata_subclass = std::is_base_of_v<SharedUiItemData, T>;
template <typename T>
concept shareduiitem_subclass = std::is_base_of_v<SharedUiItem, T>;

/** Parent class for SharedUiItem implementation data classes.
 *
 * Subclassing guidelines:
 * - TL;DR: inherit from TemplatedSharedUiItem this will help you to follow
 *   most of the rules above.
 * - A subclass MUST implement qualifier().
 * - The qualifier MUST only contains ascii letters, digits and underscore (_)
 *   and MUST start with a letter. It SHOULD be directly related to the class
 *   name, e.g. "foobar" for FoobarData. Within a given application, all
 *   possible qualifier SHOULD NOT need case-sensitivity to be distinguished
 *   one from another.
 * - The id MUST be unique in the scope of the document/document manager for a
 *   given id qualifier and MUST NOT be empty. It MAY contain any utf8 encoded
 *   unicode character even though one may find convenient to restrict to ascii
 *   for cases the id is present in logs and so on.
 * - One of the section SHOULD represent the id for both Qt::DisplayRole and
 *   SharedUiItem::ExternalDataRole roles (this is e.g. mandatory to use
 *   SimpleDatabaseDocumentManager but is usefull in many other cases).
 *   It is convenient to use section 0 as id because in many case section 0 will
 *   be displayed by default (e.g. that's what QListView does) or as first value
 *   (e.g. QTreeView, QTableView, QColumnView).
 * - uiSectionCount(), uiSectionName() and uiSectionByName() MUST
 *   be implemented and SHOULD have consistent results. It's a good idea to
 *   use a static Utf8StringList from which one can return the size and names,
 *   plus an index generated using ContainerUtils::index() for reverse mapping.
 * - As it is needed to display or persist data, a subclass MUST implement
 *   uiData() and SHOULD
 * - As the default implementation of uiHeaderData() is to use section names
 *   many application will want to override uiHeaderData() with more human
 *   friendly names, maybe using capitalized or event internationalized versions
 *   of section names.
 * - uiData() SHOULD handle Qt::EditRole, Qt::DisplayRole and
 *   SharedUiItem::ExternalDataRole, on read-only UI sections, Qt::EditRole
 *   SHOULD be treated as an equivalent of Qt::DisplayRole.
 *   Most of the time, SharedUiItem::ExternalDataRole SHOULD be treated as an
 *   equivalent to Qt::EditRole, however there are cases where a
 *   different format is needed for external API format (storage, transfer...)
 *   than for user interface edition, for instance with escape sequences.
 * - When planning to have generic UI edition features, a subclass MUST
 *   implement uiFlags() (to add Qt::ItemIsEditable flag for editable sections)
 *   and setUiData() which MUST handle Qt::EditRole and
 *   SharedUiItem::ExternalDataRole and MAY process any role as if it were
 *   Qt::EditRole (in other words: ignore role value).
 * - Subclasses MAY override operator< (iff C++ < 20) or operator<=> (iff C++
 *   >= 20) to provide a more natural order than qualified_id utf8 order.
 *   Whatever the C++ version no other comparation operator must be overriden.
 * - There MAY be several level of subclasses, i.e. you can subclass
 *   SharedUiItemData subclasses, however in this case they must have their
 *   common sections before specific sections, because sections are what really
 *   matters from outside the SharedUiItemData implementation.
 * @see TemplatedSharedUiItem
 * @see SharedUiItem */
class LIBP6CORESHARED_EXPORT SharedUiItemData
    : public QSharedData, public ParamsProvider {
public:
  virtual ~SharedUiItemData() = default;

  // identity
  /** Return a string identifying the object among all other SharedUiItems
   * sharing the same qualifier().
   * Default: return uiData(0, Qt::DisplayRole) */
  [[nodiscard]] virtual Utf8String id() const;
  /** Return a string identifiying the data type represented within the
   * application, e.g. "student", "calendar", "quote". */
  [[nodiscard]] virtual Utf8String qualifier() const = 0;

  // ui read
  /** Return UI sections count, like QAbstractItemModel::columnCount() does for
   * columns. */
  [[nodiscard]] virtual int uiSectionCount() const = 0;
  /** Return UI name for the section, in the sense of
   * QAbstractItemsModel::roleNames() (so it's what is mapped to sections if
   * the SharedUiItem is accessed from within QML/JS).
   * Name must be unique to this section (it's an identifier).
   * It's also used as the key when accessed through ParamsSet interface. */
  [[nodiscard]] virtual Utf8String uiSectionName(int section) const = 0;
  /** Return UI section number given the section name or -1 if not found. */
  [[nodiscard]] virtual int uiSectionByName(Utf8String sectionName) const = 0;
  /** Return UI data, like QAbstractItemModel::data().
   *
   * Note that IdRole, QualifierRole, QualifiedIdRole and won't be queried
   * from SharedUiItemData::uiData() but are directly handled in
   * SharedUiItem::uiData() as calls to SharedUiItemData::id() and
   * SharedUiItemData::qualifier() instead, regardless the section. */
  [[nodiscard]] virtual QVariant uiData(
      int section, int role = Qt::DisplayRole) const = 0;
  /** Return UI header data, like QAbstractItemModel::headerData().
   * Default: uiSectionName(section) for DisplayRole, EditRole and
   * ExternalDataRole, otherwise {} */
  [[nodiscard]] virtual QVariant uiHeaderData(
      int section, int role = Qt::DisplayRole) const;

  // ui write
  /** Return UI item flags, like QAbstractItemModel::flags().
   * Default: return Qt::ItemIsEnabled | Qt::ItemIsSelectable */
  [[nodiscard]] virtual Qt::ItemFlags uiFlags(int section) const;
  /** Set data from a UI point of view, i.e. called by a QAbstractItemModel
   * after user edition.
   * Default: return false, setting the errorString to something explicit
   * @return errorString must not be null
   * @return transaction must not be null
   * @return true on success, false otherwise */
  virtual bool setUiData(
      int section, const QVariant &value, QString *errorString,
      SharedUiItemDocumentTransaction *transaction, int role = Qt::EditRole);

  // comparison
#if __cpp_impl_three_way_comparison >= 201711
  /** By default: compares identifers (qualifier() then id(), which may
   *  lead to inconsistency if comaring two versions of an object with same
   *  identifiers).
   *  Implementation can rely on SharedUiItem::operator<=> not calling
   *  SharedUiItemData::operator<=> with this == 0 or &that == 0 (and so
   *  can assume they are not null). */
  virtual std::strong_ordering operator <=>(const SharedUiItemData &that) const;
  inline bool operator ==(const SharedUiItemData &that) const {
    return *this <=> that == std::strong_ordering::equal; }
#else
  /** By default: compares identifers (qualifier() then id(), which may
   *  lead to inconsistency if comaring two versions of an object with same
   *  identifiers).
   *  Implementation can rely on SharedUiItem::operator< not calling
   *  SharedUiItemData::operator< with this == 0 or &that == 0 (and so
   *  can assume they are not null). */
  virtual bool operator<(const SharedUiItemData &that) const;
  /** Calls operator<. Do not override. Override operator< instead. */
  inline bool operator>(const SharedUiItemData &that) const {
    return that<*this; }
  /** Calls operator<. Do not override. Override operator< instead. */
  inline bool operator<=(const SharedUiItemData &that) const {
    return !(that<*this); }
  /** Calls operator<. Do not override. Override operator< instead. */
  inline bool operator>=(const SharedUiItemData &that) const {
    return !(*this<that); }
#endif // C++ 20: spaceship op
  [[nodiscard]] virtual QVariantHash toVariantHash(int role) const;
  virtual bool setFromVariantHash(
      const QVariantHash &hash, QString *errorString,
      SharedUiItemDocumentTransaction *transaction,
      const QSet<Utf8String> &ignoredSections, int role);

  // ParamSet interface
  /** ParamsProvider interface, overridable by SharedUiData implementation.
   *  Default: provide values calling uiData() with role Qt::DisplayRole,
   *  supporting named keys, section numbers, and special values "id"... */
  [[nodiscard]] TypedValue paramRawValue(
      const Utf8String &key, const TypedValue &def = {},
      const EvalContext &context = {}) const override;
  /** ParamsProvider interface, overridable by SharedUiData implementation.
   *  Default: return every section names (named keys, section numbers and
   *  special values "id"...) */
  [[nodiscard]] Utf8StringSet paramKeys(
      const EvalContext &context = {}) const override;
  /** ParamsProvider interface, overridable by SharedUiData implementation.
   *  Default: return the id qualifier.
   *  Implementation may want to return qualifiedId() or something specific to
   *  an item instance instead. */
  [[nodiscard]] Utf8String paramScope() const override;

protected:
  // both default and copy constructor are declared protected to avoid being
  // called directly from SharedUiItem base class within a non-const method
  // that would access d (or more precisely: that would call d.detach()),
  // because such methods would be buggy since SharedUiItem must not detach
  // directly since it's up to real data subclass to detach and copy the real
  // data class (a SharedUiItemData subclass)
  SharedUiItemData() noexcept : QSharedData() { }
  SharedUiItemData(const SharedUiItemData &other) noexcept
    : QSharedData(other) { }
  SharedUiItemData(SharedUiItemData &&other) noexcept
    : QSharedData(std::move(other)) { }
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
 * - A subclass MUST be declared as Q_RELOCATABLE_TYPE, in such a way:
 *     // in .h, after class definition
 *     Q_DECLARE_TYPEINFO(Foobar, Q_RELOCATABLE_TYPE);
 *   Otherwise, the application will crash, e.g. as soon as you store subclasses
 *   in a QList<SharedUiItem>. This because SharedUiItem itself is declared
 *   Q_RELOCATABLE_TYPE (which is the right thing to do anyway, since it actually
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
 * - Any other generic edition features SHOULD also be implemented as needed,
 *   such as setFromVariantHash() or setFromJsonObject()
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
// This class has no non-final virtual method and cannot have any, since it
// would prevent polymorphism to be handled by SharedUiItemData subclasses
// instead (e.g. given FooItem a subclass of SharedUiItem,
// SharedUiItem(aFooItem).aVirtualMethod() would not call
// FooItem::aVirtualMethod() but SharedUiItem::aVirtualMethod()).
//
class LIBP6CORESHARED_EXPORT SharedUiItem : public ParamsProvider {
  QSharedDataPointer<SharedUiItemData> _data;

protected:
  explicit SharedUiItem(SharedUiItemData *data) : _data(data) { }

public:
  enum SharedUiItemRole {
    IdRole = Qt::UserRole+784,
    QualifierRole,
    QualifiedIdRole,
    ExternalDataRole // for file/database storage or network transfer
  };

  SharedUiItem() noexcept {}
  ~SharedUiItem() noexcept {}
  SharedUiItem(const SharedUiItem &other) noexcept : _data(other._data) { }
  SharedUiItem(SharedUiItem &&other) noexcept
    : _data(std::move(other._data)) { }
  SharedUiItem &operator=(const SharedUiItem &other) noexcept {
    if (this != &other)
      _data = other._data;
    return *this; }
  SharedUiItem &operator=(SharedUiItem &&other) noexcept {
    if (this != &other)
      _data = std::move(other._data);
    return *this; }
#if __cpp_impl_three_way_comparison >= 201711
  inline std::strong_ordering operator<=>(const SharedUiItem &that) const {
    auto *x = _data.constData(), *y = that._data.constData();
    if (!x)
      return y ? std::strong_ordering::less : std::strong_ordering::equal;
    if (!y)
      return std::strong_ordering::greater;
    return *x <=> *y;
  }
  inline bool operator==(const SharedUiItem &that) const {
    return *this <=> that == std::strong_ordering::equal; }
#else
  inline bool operator==(const SharedUiItem &other) const {
    return qualifier() == other.qualifier() && id() == other.id(); }
  inline bool operator!=(const SharedUiItem &other) const {
    return !(*this == other); }
  inline bool operator<(const SharedUiItem &other) const {
    return other._data && (_data ? _data->operator<(*(other._data)) : true); }
  inline bool operator>(const SharedUiItem &other) const { return other<*this; }
  inline bool operator<=(const SharedUiItem &other) const {
    return !(other<*this); }
  inline bool operator>=(const SharedUiItem &other) const {
    return !(*this<other); }
#endif // C++ 20: spaceship op
  [[nodiscard]] inline bool isNull() const { return !_data; }
  [[nodiscard]] inline bool operator!() const { return isNull(); }
  /** Item identifier.
   * The identifier MUST be unique for the same type of item within the same
   * document. It can be any non empty utf8 string. */
  [[nodiscard]] inline Utf8String id() const {
    return _data ? _data->id() : Utf8String{}; }
  /** Item identifier qualifier, e.g. item type such as "invoice" for an
   * invoice data ui object. */
  [[nodiscard]] inline Utf8String qualifier() const {
    return _data ? _data->qualifier() : Utf8String{}; }
  /** Qualified item identifier.
   * The qualified identifier MUST be unique for any type of item within the
   * same document.
   * @return qualifier+':'+id if qualifier is not empty, id otherwise. */
  [[nodiscard]] inline static Utf8String qualifiedId(
      const Utf8String &qualifier, const Utf8String &id) {
    Utf8String s{qualifier}; s += ':'; s += id; return s; }
  /** Qualified item identifier.
   * The qualified identifier MUST be unique for any type of item within the
   * same document.
   * @return qualifier+':'+id if qualifier is not empty, id otherwise. */
  [[nodiscard]] inline Utf8String qualifiedId() const {
    return _data ? qualifiedId(_data->qualifier(), _data->id()) : Utf8String{};}
  /** Return UI sections count, like QAbstractItemModel::columnCount() does for
   * columns (anyway SharedUiItem sections are likely to be presented as
   * columns by a Model and displayed aas columns by a View). */
  [[nodiscard]] inline int uiSectionCount() const {
    return _data ? _data->uiSectionCount() : 0; }
  /** Return UI data, like QAbstractItemModel::data().
   *
   * Using IdRole, QualifierRole and QualifiedIdRole query
   * SharedUiItemData::id() and/or SharedUiItemData::qualifier() instead of
   * SharedUiItem::uiData(), regardless the section. */
  [[nodiscard]] inline QVariant uiData(
      int section, int role = Qt::DisplayRole) const {
    if (!_data)
      return {};
    switch (role) {
      case IdRole:
        return _data->id();
      case QualifierRole:
        return _data->qualifier();
      case QualifiedIdRole: {
        auto qualifier = _data->qualifier();
        return qualifier.isEmpty() ? _data->id() : qualifier+":"+_data->id();
      }
      default:
        return _data->uiData(section, role);
    }
  }
  /** @return section number knowing its name, or -1 */
  [[nodiscard]] inline int uiSectionByName(Utf8String sectionName) const {
    return _data ? _data->uiSectionByName(sectionName) : -1;
  }
  /** Return UI data, like QAbstractItemModel::data().
   *
   * IdRole, QualifierRole and QualifiedIdRole query
   * SharedUiItemData::id() and/or SharedUiItemData::qualifier() instead of
   * SharedUiItem::uiData(), regardless the section.
   *
   * "id", "qualifier" and "qualified_id" section names query
   * SharedUiItemData::id() and/or SharedUiItemData::qualifier()
   * instead of SharedUiItem::uiSectionByName() and SharedUiItem::uiData(). */
  [[nodiscard]] QVariant uiDataBySectionName(
      Utf8String sectionName, int role = Qt::DisplayRole) const {
    if (!_data)
      return {};
    if (role == IdRole)
      return _data->id();
    if (role == QualifierRole)
      return _data->qualifier();
    if (role == QualifiedIdRole)
      return qualifiedId();
    if (sectionName == "id"_u8)
      return _data->id();
    if (sectionName == "qualifier"_u8)
      return _data->qualifier();
    if (sectionName == "qualified_id"_u8)
      return qualifiedId();
    int section = uiSectionByName(sectionName);
    if (section < 0)
      return {};
    return uiData(section, role);
  }
  /** Convenience method for uiData(...).toString(). */
  [[nodiscard]] inline QString uiString(
      int section, int role = Qt::DisplayRole) const {
    return uiData(section, role).toString(); }
  /** Convenience method for uiData(...).toString().toUtf8(). */
  [[nodiscard]] inline Utf8String uiUtf8(
      int section, int role = Qt::DisplayRole) const {
    return Utf8String(uiData(section, role)); }
  /** Return UI header data, like QAbstractItemModel::headerData(). */
  [[nodiscard]] inline QVariant uiHeaderData(int section, int role) const {
    return _data ? _data->uiHeaderData(section, role) : QVariant() ; }
  /** Convenience method for uiHeaderData(...).toString(). */
  [[nodiscard]] inline QString uiHeaderString(
      int section, int role = Qt::DisplayRole) const {
    return uiHeaderData(section, role).toString(); }
  /** Convenience method for uiHeaderData(...).toString().toUtf8(). */
  [[nodiscard]] inline Utf8String uiHeaderUtf8(
      int section, int role = Qt::DisplayRole) const{
    return Utf8String(uiHeaderData(section, role)); }
  /** Return UI name (id) for the section, in the sense of
   * QAbstractItemsModel::roleNames() */
  [[nodiscard]] inline Utf8String uiSectionName(int section) const {
    return _data ? _data->uiSectionName(section) : Utf8String{}; }
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
  [[nodiscard]] Qt::ItemFlags uiFlags(int section) const {
    return _data ? _data->uiFlags(section) : Qt::NoItemFlags; }
  /** Convert to QVariantHash. */
  [[nodiscard]] inline QVariantHash toVariantHash(
      int role = Qt::DisplayRole) const  {
    return _data ? _data->toVariantHash(role) : QVariantHash();
  }
  /** Convert to JSON object. */
  [[nodiscard]] inline QJsonObject toJsonObject(
      int role = Qt::DisplayRole) const {
    return _data ? QJsonObject::fromVariantHash(_data->toVariantHash(role))
                 : QJsonObject();
  }
  /** Copy a SharedUiItem using uiData() and setUiData() for every section but
   * the one that are specified to be ignored.
   * @param ignoredSections sections not to be copied, e.g. { 0 } */
#ifdef __cpp_concepts
  template <shareduiitem_subclass T>
#else
  template<class T, std::enable_if_t<std::is_base_of_v<SharedUiItem,T>,bool> = true>
#endif
  static inline bool copy(
      T *dest, const T &source,
      QString *errorString, SharedUiItemDocumentTransaction *transaction,
      const QSet<int> &ignoredSections, int role = Qt::DisplayRole) {
    int n = source.uiSectionCount();
    for (int i = 0; i < n; ++i) {
      if (ignoredSections.contains(i))
        continue;
      if (!dest->setUiData(i, source.uiData(i, role), errorString, transaction,
                           role))
        return false;
    }
    return true;
  }
  /** Copy a SharedUiItem using uiData() and setUiData() for every section but
   * the one that are specified to be ignored.
   * @param ignoredSections sections not to be copied, e.g. { "id" } */
#ifdef __cpp_concepts
  template <shareduiitem_subclass T>
#else
  template<class T, std::enable_if_t<std::is_base_of_v<SharedUiItem,T>,bool> = true>
#endif
  static inline bool copyBySectionName(
      T *dest, const T &source,
      QString *errorString, SharedUiItemDocumentTransaction *transaction,
      const QSet<Utf8String> &ignoredSections = { },
      int role = Qt::DisplayRole) {
    int n = source.uiSectionCount();
    for (int i = 0; i < n; ++i) {
      auto name = source.uiSectionName(i);
      if (ignoredSections.contains(name))
        continue;
      if (!dest->setUiData(i, source.uiData(i, role), errorString, transaction,
                           role))
        return false;
    }
    return true;
  }
  /** Copy a SharedUiItem using uiData() and setUiData() for every section but
   * the one that are specified to be ignored, supporting different kind of
   * item, mapping their section names.
   * @param ignoredSections sections not to be copied, e.g. { "id" } */
#ifdef __cpp_concepts
  template <shareduiitem_subclass D, shareduiitem_subclass S>
#else
  template<class D, class S,
           std::enable_if_t<std::is_base_of_v<SharedUiItem,D>,bool> = true,
           std::enable_if_t<std::is_base_of_v<SharedUiItem,S>,bool> = true>
#endif
  static inline bool copyBySectionName(
      D *dest, const S &source,
      QString *errorString, SharedUiItemDocumentTransaction *transaction,
      const QSet<Utf8String> &ignoredSections = { },
      int role = Qt::DisplayRole) {
    int n = source.uiSectionCount();
    for (int i = 0; i < n; ++i) {
      auto name = source.uiSectionName(i);
      if (ignoredSections.contains(name))
        continue;
      int j = dest->uiSectionByName(name);
      if (!dest->setUiData(j, source.uiData(i, role), errorString, transaction,
                           role))
        return false;
    }
    return true;
  }
  /** Downcast blindly trusting caller that qualifier implies T */
#ifdef __cpp_concepts
  template <shareduiitem_subclass T>
#else
  template<class T,
           std::enable_if_t<std::is_base_of_v<SharedUiItem,T>,bool> = true>
#endif
  [[nodiscard]] const T &casted() const {
    return static_cast<const T&>(*this);
  }
  /** Downcast blindly trusting caller that qualifier implies T */
#ifdef __cpp_concepts
  template <shareduiitem_subclass T>
#else
  template<class T,
           std::enable_if_t<std::is_base_of_v<SharedUiItem,T>,bool> = true>
#endif
  [[nodiscard]] T &casted() {
    return static_cast<T&>(*this);
  }

  // ParamSet interface
  using ParamsProvider::paramRawValue;
  /** ParamsProvider interface, overridable by SharedUiData implementation.
   *  Default: provide values calling uiData() with role Qt::DisplayRole,
   *  supporting named keys, section numbers, and special values "id"... */
  [[nodiscard]] TypedValue paramRawValue(
      const Utf8String &key, const TypedValue &def = {},
      const EvalContext &context = {}) const override;
  using ParamsProvider::paramKeys;
  /** ParamsProvider interface, overridable by SharedUiData implementation.
   *  Default: return every section names (named keys, section numbers and
   *  special values "id"...) */
  [[nodiscard]] Utf8StringSet paramKeys(
      const EvalContext &context) const override;
  /** ParamsProvider interface, overridable by SharedUiData implementation. */
  [[nodiscard]] bool paramContains(
      const Utf8String &key, const EvalContext &context = {}) const override;
  using ParamsProvider::paramScope;
  /** ParamsProvider interface, overridable by SharedUiData implementation.
   *  Default: return the id qualifier. */
  [[nodiscard]] Utf8String paramScope() const override;

protected:
  const SharedUiItemData *data() const { return _data.data(); }
  void setData(SharedUiItemData *data) { _data = data; }
  /** Helper template to provide a const pointer to specialized data,
   * e.g. const FooBarData *data = foobar.specializedData<FooBarData>();
   * useful to declare a const FooBarData *data() const method in FooBar class,
   * see subclassing guidelines above */
#ifdef __cpp_concepts
  template <shareduiitemdata_subclass T>
#else
  template <class T,
            std::enable_if_t<std::is_base_of_v<SharedUiItemData,T>,bool> = true>
#endif
  inline const T *specializedData() const {
    return static_cast<const T*>(_data.constData());
  }
  /** Helper template to detach calling the specialized data class' copy
   * constructor rather than base SharedUiItemData's one, and return a non-const
   * pointer to specialized data,
   * e.g. FooBarData *data = foobar.detachedData<FooBarData>();
   * useful to declare a "FooBarData *data()" non const method in FooBar class,
   * see subclassing guidelines above */
#ifdef __cpp_concepts
  template <shareduiitemdata_subclass T>
#else
  template <class T,
            std::enable_if_t<std::is_base_of_v<SharedUiItemData,T>,bool> = true>
#endif
  T *detachedData() {
    auto ptr = reinterpret_cast<QSharedDataPointer<T>*>(&_data);
    // data() must be called after type conversion in order to detach() to be
    // able to call actual copy constructor TData::TData() rather than base
    // class' SharedUiItemData::SharedUiItemData()
    return ptr->data();
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
#if 0
  /** Make uiData() available through ParamsProvider interface, for any given
   *  role (whereas ParamsProvider direct implementation uses only
   *  Qt::DisplayRole).
   * @see SharedUiItemParamsProvider */
  inline SharedUiItemParamsProvider toParamsProvider(
      int role = Qt::DisplayRole) const;
#endif
  /** Set ui data according to QVariantHash<sectionName,value> values.
   * This method must be reimplemented and made public by subclasses in order
   * to be usable.
   * It cannot be done in a generic manner in base class because non-const
   * access to data mustn't be performed in base class.
   * @return true on success, false otherwise */
  inline bool setFromVariantHash(
      const QVariantHash &hash, QString *errorString,
      SharedUiItemDocumentTransaction *transaction,
      const QSet<Utf8String> &ignoredSections = { },
      int role = Qt::EditRole);
#if 0
  /** Set ui data according to JSON object values.
   * This method must be reimplemented and made public by subclasses in order
   * to be usable.
   * It cannot be done in a generic manner in base class because non-const
   * access to data mustn't be performed in base class.
   * @return true on success, false otherwise */
  inline bool setFromJsonObject(
      const QJsonObject &json, QString *errorString,
      SharedUiItemDocumentTransaction *transaction,
      const QSet<Utf8String> &ignoredSections = { },
      int role = Qt::EditRole);
#endif
};

Q_DECLARE_METATYPE(SharedUiItem)
Q_DECLARE_TYPEINFO(SharedUiItem, Q_RELOCATABLE_TYPE);

inline uint qHash(const SharedUiItem &i) { return qHash(i.id()); }

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const SharedUiItem &i);

#endif // SHAREDUIITEM_H
