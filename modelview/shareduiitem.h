/* Copyright 2014 Hallowyn and others.
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

class LIBQTSSUSHARED_EXPORT SharedUiItemData : public QSharedData {
public:
  virtual ~SharedUiItemData();
  /** default: return QVariant()
   * Note that IdRole, IdQualifierRole and QualifiedIdRole won't be queried
   * from SharedUiItemData::uiData() but are directly handled in
   * SharedUiItem::uiData() as calls to SharedUiItemData::id() and
   * SharedUiItemData::idQualifier() instead, regardless the section. */
  virtual QVariant uiData(int section, int role) const;
  virtual QVariant uiHeaderData(int section, int role) const;
  virtual int uiDataCount() const;
  /** default: return uiData(0, Qt::DisplayRole).toString() */
  virtual QString id() const;
  /** default: return QString() */
  virtual QString idQualifier() const;
};

/** Parent class for implicitely shared data classes to be queried as a user
 * interface item, i.e. an object with uiDataCount() sections that can be
 * queried as QVariant via uiData() and uiHeaderData().
 * This is usefull used both in QAbstractItemModel implementation as data items
 * and in custom forms.
 * Warning: d.detach() must never be called since ImplicitlySharedUiItemData's
 * copy constructor is not able to copy the real object, therefore it is up to
 * subclasses to re-implement a custom non-const data accessor of their own,
 * calling protected method detach<UiItemImplementation>() to have the implicit
 * sharing copy work.
 * @see UiItemsModel
 * @see UiItemsTableModel */
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
  bool isNull() const { return !d; }
  QVariant uiHeaderData(int section, int role) const {
    return d ? d->uiHeaderData(section, role) : QVariant() ; }
  /** Syntaxic sugar. */
  QString uiHeaderString(int section, int role = Qt::DisplayRole) const {
    return uiHeaderData(section, role).toString(); }
  int uiDataCount() const { return d ? d->uiDataCount() : 0; }
  /** Provides ui data for this item. */
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
  /** Syntaxic sugar. */
  QString uiString(int section, int role = Qt::DisplayRole) const {
    return d ? d->uiData(section, role).toString() : QString(); }
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
  bool operator==(const SharedUiItem &other) const;
  bool operator<(const SharedUiItem &other) const;

protected:
  const SharedUiItemData *constData() const { return d.constData(); }
  void setData(SharedUiItemData *data) { d = data; }
  template <class T> void detach() {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItemData*>(dummy)); // ensure T is a SharedUiItemData
    reinterpret_cast<QSharedDataPointer<T>*>(&d)->detach();
  }
};

inline uint qHash(const SharedUiItem &i) { return qHash(i.id()); }
QDebug LIBQTSSUSHARED_EXPORT operator<<(QDebug dbg, const SharedUiItem &i);

#endif // SHAREDUIITEM_H
