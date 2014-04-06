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

class LIBQTSSUSHARED_EXPORT SharedUiItemData : public QSharedData {
public:
  virtual ~SharedUiItemData();
  /** default: return QVariant() */
  virtual QVariant uiData(int section, int role) const;
  /** default: return uiData(0, Qt::DisplayRole).toString() */
  virtual QString id() const;
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
 * @see UiItemsTableModel
 */
class LIBQTSSUSHARED_EXPORT SharedUiItem {
  QSharedDataPointer<SharedUiItemData> d;

public:
  SharedUiItem() { }
  SharedUiItem(const SharedUiItem &other) : d(other.d) { }
  virtual ~SharedUiItem();
  SharedUiItem &operator=(const SharedUiItem &other) {
    if (this != &other)
      d = other.d;
    return *this; }
  bool isNull() const { return !d; }
  virtual QVariant uiHeaderData(int section, int role) const;
  /** Syntaxic sugar. */
  QString uiHeaderString(int section, int role = Qt::DisplayRole) const {
    return uiHeaderData(section, role).toString(); }
  virtual int uiDataCount() const;
  QVariant uiData(int section, int role = Qt::DisplayRole) const {
    return d ? d->uiData(section, role) : QVariant(); }
  /** Syntaxic sugar. */
  QString uiString(int section, int role = Qt::DisplayRole) const {
    return d ? d->uiData(section, role).toString() : QString(); }
  /** Item identifier. */
  QString id() const { return d ? d->id() : QString(); }
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

#endif // SHAREDUIITEM_H
