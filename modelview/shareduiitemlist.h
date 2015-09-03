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
#ifndef SHAREDUIITEMLIST_H
#define SHAREDUIITEMLIST_H

#include "shareduiitem.h"
#include <QList>
#include <QChar>
#include <QString>
#include <QRegularExpression>

/** Specializing QList for SharedUiItems, the same way QStringList does. */
template <class T = SharedUiItem>
class LIBQTSSUSHARED_EXPORT SharedUiItemList : public QList<T> {
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
  //QString join<SharedUiItem>(const QString &separator, bool qualified) const;
  //QString join<SharedUiItem>(const QChar separator, bool qualified) const;
  QString join(const QString &separator, bool qualified) const {
    SharedUiItemList<SharedUiItem> *upcasted =
        (SharedUiItemList<SharedUiItem>*)this;
    return upcasted->join(separator, qualified);
  }
  QString join(const QChar separator, bool qualified) const {
    SharedUiItemList<SharedUiItem> *upcasted =
        (SharedUiItemList<SharedUiItem>*)this;
    return upcasted->join(separator, qualified);
  }
  // conversion operator to enable upcasting an list to to SharedUiItem list
  operator SharedUiItemList<SharedUiItem> &() {
    return reinterpret_cast<SharedUiItemList<SharedUiItem>&>(*this);
  }
  // conversion operator to enable upcasting an list to to SharedUiItem list
  operator const SharedUiItemList<SharedUiItem> &() const {
    return reinterpret_cast<const SharedUiItemList<SharedUiItem>&>(*this);
  }
  // TODO add features
  //SharedUiItemList filterByQualifier(QString qualifier) const;
  //SharedUiItemList filterByQualifier(QRegularExpression qualifier) const;
  //operator<<(SharedUiItem)
  //operator<<(SharedUiItemList)
  //operator<<(QList<SharedUiItem>)
  //operator+=
};

template<>
QString LIBQTSSUSHARED_EXPORT SharedUiItemList<SharedUiItem>::join(
    const QString &separator, bool qualified) const;

template<>
QString LIBQTSSUSHARED_EXPORT SharedUiItemList<SharedUiItem>::join(
    const QChar separator, bool qualified) const;

// LATER find a way to convert QList<T> to SharedUiItemList<SharedUiItem> with any T : SharedUiItem
/**
template <class T>
inline operator SharedUiItemList<SharedUiItem>(QList<T> &other) {
  T *dummy;
  Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
  return SharedUiItemList<SharedUiItem>(
        reinterpret_cast<const QList<SharedUiItem>&>(other));
}*/

#endif // SHAREDUIITEMLIST_H
