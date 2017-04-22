/* Copyright 2015-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef SHAREDUIITEMLIST_H
#define SHAREDUIITEMLIST_H

#include "shareduiitem.h"
#include <QList>
#include <QChar>
#include <QString>

/** Specializing QList for SharedUiItems, the same way QStringList does. */
template <class T = SharedUiItem>
class LIBPUMPKINSHARED_EXPORT SharedUiItemList : public QList<T> {
public:
  inline SharedUiItemList() {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
  }
  inline SharedUiItemList(const SharedUiItemList<T> &other)
    : QList<T>(other) {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
  }
  inline SharedUiItemList(const QList<T> &other) : QList<T>(other) {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
  }
  inline QString join(const QString &separator, bool qualified = false) const;
  inline QString join(const QChar separator, bool qualified = false) const;
  // conversion operator to enable upcasting any list to a SharedUiItem list
  operator SharedUiItemList<SharedUiItem> &() {
    return reinterpret_cast<SharedUiItemList<SharedUiItem>&>(*this);
  }
  // conversion operator to enable upcasting any list to a SharedUiItem list
  operator const SharedUiItemList<SharedUiItem> &() const {
    return reinterpret_cast<const SharedUiItemList<SharedUiItem>&>(*this);
  }
  // MAYDO add features
  //SharedUiItemList filterByQualifier(QString qualifier) const;
  //SharedUiItemList filterByQualifier(QRegularExpression qualifier) const;
  //operator<<(SharedUiItem)
  //operator<<(SharedUiItemList)
  //operator<<(QList<SharedUiItem>)
  //operator+=
};

/** Template specialization for SharedUiItemList<SharedUiItem> */
template <>
class LIBPUMPKINSHARED_EXPORT SharedUiItemList<SharedUiItem>
    : public QList<SharedUiItem> {
private:
  template <class S>
  inline QString generic_join(const S &separator, bool qualified) const {
    QString s;
    bool first = true;
    for (const SharedUiItem &item : *this) {
      if (first)
        first = false;
      else
        s += separator;
      if (qualified) {
        s += item.idQualifier();
        s += ':';
        s += item.id();
      } else
        s += item.id();
    }
    return s;
  }

public:
  inline SharedUiItemList() { }
  inline SharedUiItemList(const SharedUiItemList<SharedUiItem> &other)
    : QList<SharedUiItem>(other) { }
  inline SharedUiItemList(const QList<SharedUiItem> &other)
    : QList<SharedUiItem>(other) { }
  // upcasting constructor to convert any list to a SharedUiItem list
  template<class T>
  inline SharedUiItemList(const SharedUiItemList<T> &other)
    : QList<SharedUiItem>(reinterpret_cast<const QList<SharedUiItem>&>(other)) {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
  }
  // upcasting constructor to convert any list to a SharedUiItem list
  template<class T>
  inline SharedUiItemList(const QList<T>& other)
    : QList<SharedUiItem>(reinterpret_cast<const QList<SharedUiItem>&>(other)) {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
  }
  QString join(const QString &separator, bool qualified) const {
    return generic_join(separator, qualified);
  }
  QString join(const QChar separator, bool qualified) const {
    return generic_join(separator, qualified);
  }
};

Q_DECLARE_METATYPE(SharedUiItemList<>)

template <class T>
inline QString SharedUiItemList<T>::join(
    const QString &separator, bool qualified) const {
  const SharedUiItemList<SharedUiItem> &upcasted = *this;
  return upcasted.join(separator, qualified);
}

template <class T>
inline QString SharedUiItemList<T>::join(
    const QChar separator, bool qualified) const {
  const SharedUiItemList<SharedUiItem> &upcasted = *this;
  return upcasted.join(separator, qualified);
}

#endif // SHAREDUIITEMLIST_H
