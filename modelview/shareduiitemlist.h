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

#include "libqtssu_global.h"
#include <QList>
#include <QChar>
#include <QString>
#include <QRegularExpression>

class SharedUiItem;

/** Specializing QList for SharedUiItems, the same way QStringList does. */
class LIBQTSSUSHARED_EXPORT SharedUiItemList : public QList<SharedUiItem> {
public:
  inline SharedUiItemList() { }
  inline SharedUiItemList(const SharedUiItemList &other)
    : QList<SharedUiItem>(other) { }
  template <class T>
  inline SharedUiItemList(const QList<T> &other)
    : QList<SharedUiItem>(reinterpret_cast<const QList<SharedUiItem>&>(other)) {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
  }
  QString join(const QString &separator, bool qualified) const;
  QString join(const QChar separator, bool qualified) const;
  // TODO add features
  //SharedUiItemList filterByQualifier(QString qualifier) const;
  //SharedUiItemList filterByQualifier(QRegularExpression qualifier) const;
  //operator<<(SharedUiItem)
  //operator<<(SharedUiItemList)
  //operator<<(QList<SharedUiItem>)
  //operator+=
};

#endif // SHAREDUIITEMLIST_H
