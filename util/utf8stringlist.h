/* Copyright 2023 Gregoire Barbier and others.
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
#ifndef UTF8STRINGLIST_H
#define UTF8STRINGLIST_H

#include "util/paramsprovider.h"
#include <set>

class Utf8StringSet;
class LogHelper;

class LIBP6CORESHARED_EXPORT Utf8StringList
    : public QList<Utf8String>, public ParamsProvider {
public:
  Utf8StringList() { }
  Utf8StringList(std::initializer_list<Utf8String> args)
    : QList<Utf8String>(args) { }
  // explicit Utf8StringList(const Utf8String &item) : QList<Utf8String>({item}) { }
  Utf8StringList(const QList<Utf8String> &list) : QList<Utf8String>(list) { }
  Utf8StringList(const QList<QByteArray> &list)
    : QList<Utf8String>(list.cbegin(), list.cend()) { }
  Utf8StringList(const QList<QString> &list)
    : QList<Utf8String>(list.cbegin(), list.cend()) { }
  Utf8StringList(const QSet<Utf8String> &set)
    : QList<Utf8String>(set.cbegin(), set.cend()) { }
  Utf8StringList(const QSet<QByteArray> &set)
    : QList<Utf8String>(set.cbegin(), set.cend()) { }
  Utf8StringList(const QSet<QString> &set)
    : QList<Utf8String>(set.cbegin(), set.cend()) { }
#if __cpp_concepts >= 201907
  template <std::input_iterator InputIterator>
  Utf8StringList(InputIterator i1, InputIterator i2)
    : QList<Utf8String>(i1, i2) { }
#endif
  Utf8String join(const Utf8String &separator) const;
  Utf8String join(const char separator) const;
  QStringList toStringList() const {
    return QStringList(cbegin(), cend()); }
  QByteArrayList toByteArrayList() const {
    return QByteArrayList(cbegin(), cend()); }
  inline Utf8StringSet toSet() const;
  inline Utf8StringList toSortedDeduplicatedList() const;
  std::set<Utf8String> toStdSet() const {
    return std::set<Utf8String>(cbegin(), cend()); }
  /** Return first string as value 1 and so on. Return join(' ') as value 0. */
  const QVariant paramRawValue(
      const Utf8String &key, const QVariant &def) const override;
  /** Return numbers from 0 to size(). */
  const Utf8StringSet paramKeys() const override;
};

Q_DECLARE_METATYPE(Utf8StringList)

class LIBP6CORESHARED_EXPORT Utf8StringSet : public QSet<Utf8String> {
public:
  Utf8StringSet() { }
  Utf8StringSet(std::initializer_list<Utf8String> args)
    : QSet<Utf8String>(args) { }
  Utf8StringSet(const QSet<Utf8String> &set)
    : QSet<Utf8String>(set) { }
  Utf8StringSet(const QSet<QByteArray> &set)
    : QSet<Utf8String>(set.cbegin(), set.cend()) { }
  Utf8StringSet(const QSet<QString> &set)
    : QSet<Utf8String>(set.cbegin(), set.cend()) { }
  Utf8StringSet(const QList<Utf8String> &set)
    : QSet<Utf8String>(set.cbegin(), set.cend()) { }
  Utf8StringSet(const QList<QByteArray> &set)
    : QSet<Utf8String>(set.cbegin(), set.cend()) { }
  Utf8StringSet(const QList<QString> &set)
    : QSet<Utf8String>(set.cbegin(), set.cend()) { }
  Utf8StringSet(const std::set<Utf8String> &set)
    : QSet<Utf8String>(set.cbegin(), set.cend()) { }
#if __cpp_concepts >= 201907
  template <std::input_iterator InputIterator>
  Utf8StringSet(InputIterator i1, InputIterator i2)
    : QSet<Utf8String>(i1, i2) { }
#endif
  Utf8String join(const Utf8String &separator) const;
  Utf8String join(const char separator) const;
  Utf8String sortedJoin(const Utf8String &separator) {
    return toSortedList().join(separator); }
  Utf8String sortedJoin(const char separator) {
    return toSortedList().join(separator); }
  Utf8StringList toList() const { return Utf8StringList(*this); }
  Utf8StringList toSortedList() const {
    auto list = toList(); std::sort(list.begin(), list.end()); return list; }
  std::set<Utf8String> toStdSet() const {
    return std::set<Utf8String>(cbegin(), cend()); }
};

Q_DECLARE_METATYPE(Utf8StringSet)

Utf8StringSet Utf8StringList::toSet() const {
  return Utf8StringSet(*this);
}

Utf8StringList Utf8StringList::toSortedDeduplicatedList() const {
  return toSet().toSortedList();
}

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const Utf8StringList &l);

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const Utf8StringSet &s);

LogHelper LIBP6CORESHARED_EXPORT operator<<(LogHelper lh,
                                             const Utf8StringList &list);

LogHelper LIBP6CORESHARED_EXPORT operator<<(LogHelper lh,
                                             const Utf8StringSet &set);

#endif // UTF8STRINGLIST_H
