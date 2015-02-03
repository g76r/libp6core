/* Copyright 2014-2015 Hallowyn and others.
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
#ifndef SHAREDUIITEM_H
#define SHAREDUIITEM_H

#include "libqtssu_global.h"
#include <QVariant>
#include <QString>
#include <QSharedData>
#include <QSharedDataPointer>

class QDebug;
class SharedUiItemDocumentManager;

/** Parent class for SharedUiItem implementation data classes.
 *
 * Subclassing guidelines:
 * - A subclass MUST implement id() and idQualifier().
 * - As soon as it contains data and wants it displayed, which is very likely,
 *   a subclass MUST implement uiData() and uiSectionCount() and SHOULD
 *   implement uiHeaderData().
 * - When planning to have generic UI edition features, a subclass MUST
 *   implement uiFlags() (to add Qt::ItemIsEditable flag for editable sections)
 *   and setUiData().
 * - There MUST NOT be several level of subclasses, i.e. you must not subclass
 *   SharedUiItemData subclasses.
 * @see SharedUiItem */
// LATER update guidelines with ways to have several level of subclasses, at
// less this should require to have constructors protected in non-final levels
// of subclasses
class LIBQTSSUSHARED_EXPORT SharedUiItemData : public QSharedData {
public:
  virtual ~SharedUiItemData();
  /** Return a string identifying the object among all other SharedUiItems
   * sharing the same idQualifier().
   * The id SHOULD be unique, if it is not, keep in mind that many algorithms
   * will assume that it is, such as searching, sorting, etc. Especially within
   * Model/View classes.
   * Default: return uiData(0, Qt::DisplayRole).toString() */
  virtual QString id() const;
  /** Return a string identifiying the data type represented within the
   * application, e.g. "student", "calendar", "quote".
   * SHOULD be directly related to the class name, e.g. "foobar" for FoobarData.
   * Default: return QString() */
  virtual QString idQualifier() const;
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
  /** Return UI item flags, like QAbstractItemModel::flags().
   * Default: return Qt::ItemIsEnabled | Qt::ItemIsSelectable */
  virtual Qt::ItemFlags uiFlags(int section) const;
  /** Set data from a UI point of view, i.e. called by a QAbstractItemModel
   * after user edition.
   * Default: return false
   * @return true on success, false otherwise */
  virtual bool setUiData(int section, const QVariant &value,
                         QString *errorString, int role,
                         const SharedUiItemDocumentManager *dm);

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
 * interface item, i.e. an object with uiDataCount() sections that can be
 * queried as QVariant via uiData() and uiHeaderData().
 * This is usefull used both in QAbstractItemModel implementation as data items
 * and in custom forms.
 *
 * Subclassing guidelines:
 * - A subclass MUST implement default and copy constructors
 * - A subclass SHOULD implement operator=() for its own type
 * - A subclass MUST NOT access d in non-const methods since SharedUiItemData's
 *   copy constructor is not able to copy the real object.
 * - Often, a subclass SHOULD implement a detach() method, in such a way:
 *     // in .h
 *     void detach();
 *     // in .cpp
 *     void Foobar::detach() {
 *       SharedUiItem::detach<FoobarData>();
 *     }
 * - Therefore, as soon as at less one non-const method has to access data,
 *   a subclass MUST implement d accessors such as these and every non-const
 *   method must use them instead of d.data():
 *     // in .h
 *     FoobarData *fd();
 *     const FoobarData *fd() const { return (const FoobarData*)constData(); }
 *     // in .cpp
 *     FoobarData *Foobar::fd() {
 *       SharedUiItem::detach<FoobarData>();
 *       return (FoobarData*)constData();
 *     }
 * - When planning to have generic UI edition features, a subclass MUST
 *   reimplement setUiData() method, in such a way:
 *     // in .h
 *     bool setUiData(int section, const QVariant &value, QString *errorString,
 *                    int role, const SharedUiItemDocumentManager *dm);
 *     // in .cpp
 *     bool Foobar::setUiData(int section, const QVariant &value,
 *                            QString *errorString = 0, int role,
 *                            const SharedUiItemDocumentManager *dm) {
 *       if (isNull())
 *         return false;
 *       SharedUiItem::detach<FoobarData>();
 *       return ((FoobarData*)constData())->setUiData(section, value,
 *                                                    errorString, role, dm);
 *     }
 * - There MUST NOT be several level of subclasses, i.e. you must not subclass
 *   SharedUiItem subclasses.
 *
 * @see SharedUiItemsModel
 * @see SharedUiItemsTableModel
 * @see SharedUiItemsTreeModel
 * @see SharedUiItemWidgetMapper
 * @see QStandardItem
 * @see SharedUiItemData */
//
// TODO provides guidelines for building template objects for models init
// (such as Task::templateTask or TaskGroup::TaskGroup(QString))
//
// This class must have no method that calls d.detach(), therefore no
// non-const method that accesses d, apart from operator=() and constructors,
// otherwise SharedUiItemData would be copied by QSharedData::clone() instead
// of a relevant subclass.
//
// This class must have no virtual method, since it would prevent polymorphism
// to be handled by SharedUiItemData subclasses instead (e.g. given FooItem a
// subclass of SharedUiItem, SharedUiItem(aFooItem).aVirtualMethod() would not
// call FooItem::aVirtualMethod() but SharedUiItem::aVirtualMethod()).
//
class LIBQTSSUSHARED_EXPORT SharedUiItem {
  QSharedDataPointer<SharedUiItemData> d;

protected:
  explicit SharedUiItem(SharedUiItemData *data) : d(data) { }

public:
  enum SharedUiItemRole {
    IdRole = Qt::UserRole+784,
    IdQualifierRole,
    QualifiedIdRole
  };

  SharedUiItem() { }
  SharedUiItem(const SharedUiItem &other) : d(other.d) { }
  SharedUiItem &operator=(const SharedUiItem &other) {
    if (this != &other)
      d = other.d;
    return *this; }
  /** Compares identifers (idQualifier() then id()), not full content, therefore
   * two versions of an object with same identifiers will be equal. */
  bool operator==(const SharedUiItem &other) const {
    return idQualifier() == other.idQualifier() && id() == other.id(); }
  /** Compares identifers (idQualifier() then id()), not full content, therefore
   * two versions of an object with same identifiers will have same order. */
  bool operator<(const SharedUiItem &other) const {
    return idQualifier() < other.idQualifier() || id() < other.id(); }
  bool isNull() const { return !d; }
  /** Item identifier.
   * By convention, identifier must be unique for the same type of item within
   * the same document. */
  QString id() const { return d ? d->id() : QString(); }
  /** Item identifier qualifier, e.g. item type such as "invoice" for an
   * invoice data ui object, maybe QString() depending on items */
  QString idQualifier() const { return d ? d->idQualifier() : QString(); }
  /** Qualified item identifier.
   * By convention, qualified identifier must be unique for any type of item
   * within the same document.
   * @return idQualifier()+":"+id() if idQualifier is not empty, id() otherwise.
   */
  QString qualifiedId() const {
    if (d) {
      QString qualifier = d->idQualifier();
      return qualifier.isEmpty() ? d->id() : qualifier+":"+d->id();
    }
    return QString();
  }
  /** Return UI sections count, like QAbstractItemModel::columnCount() does for
   * columns (anyway SharedUiItem sections are likely to be presented as
   * columns by a Model and displayed aas columns by a View). */
  int uiSectionCount() const { return d ? d->uiSectionCount() : 0; }
  /** Return UI data, like QAbstractItemModel::data().
   * Using IdRole, IdQualifierRole and QualifiedIdRole make query to
   * SharedUiItemData::id() and/or SharedUiItemData::idQualifier() instead
   * of SharedUiItem::uiData(), regardless the section. */
  QVariant uiData(int section, int role = Qt::DisplayRole) const {
    if (d) {
      switch (role) {
      case IdRole:
        return d->id();
      case IdQualifierRole:
        return d->idQualifier();
      case QualifiedIdRole: {
        QString qualifier = d->idQualifier();
        return qualifier.isEmpty() ? d->id() : qualifier+":"+d->id();
      }
      default:
        return d->uiData(section, role);
      }
    }
    return QVariant();
  }
  /** Convenience method for uiData(...).toString(). */
  QString uiString(int section, int role = Qt::DisplayRole) const {
    return d ? d->uiData(section, role).toString() : QString(); }
  /** Return UI header data, like QAbstractItemModel::headerData(). */
  QVariant uiHeaderData(int section, int role) const {
    return d ? d->uiHeaderData(section, role) : QVariant() ; }
  /** Convenience method for uiHeaderData(...).toString(). */
  QString uiHeaderString(int section, int role = Qt::DisplayRole) const {
    return uiHeaderData(section, role).toString(); }
  /** Return UI item flags, like QAbstractItemModel::flags().
   * Default: return Qt::ItemIsEnabled | Qt::ItemIsSelectable */
  Qt::ItemFlags uiFlags(int section) const {
    return d ? d->uiFlags(section) : Qt::NoItemFlags; }

protected:
  const SharedUiItemData *constData() const { return d.constData(); }
  void setData(SharedUiItemData *data) { d = data; }
  template <class T>
  void detach() {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItemData*>(dummy)); // ensure T is a SharedUiItemData
    reinterpret_cast<QSharedDataPointer<T>*>(&d)->detach();
  }
  /** Set data from a UI point of view, i.e. called by a QAbstractItemModel
   * after user edition.
   * This method must be reimplemented and made public by subclasses in order
   * to be usable.
   * It cannot be done in a generic manner in base class because non-const
   * access to d mustn't be performed in base class.
   * @return true on success, false otherwise */
  bool setUiData(int section, const QVariant &value, QString *errorString = 0,
                 int role = Qt::EditRole,
                 const SharedUiItemDocumentManager *dm = 0);
};

inline uint qHash(const SharedUiItem &i) { return qHash(i.id()); }
QDebug LIBQTSSUSHARED_EXPORT operator<<(QDebug dbg, const SharedUiItem &i);

#endif // SHAREDUIITEM_H
