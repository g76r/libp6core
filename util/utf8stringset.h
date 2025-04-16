/* Copyright 2023-2025 Gregoire Barbier and others.
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

#ifndef UTF8STRINGSET_H
#define UTF8STRINGSET_H

#include "utf8string.h"
#include <set>

namespace p6::log {
class LogHelper;
}

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

  Utf8StringSet &operator +=(const QSet<Utf8String> &set) {
    QSet<Utf8String>::operator +=(set); return *this; }
  Utf8StringSet &operator +=(const Utf8String &s) {
    QSet<Utf8String>::operator +=(s); return *this; }
#if __cpp_concepts >= 201907
  Utf8StringSet &operator +=(const QSet<QByteArray> &set) {
    QSet<Utf8String>::operator +=(Utf8StringSet(set.cbegin(), set.cend()));
    return *this; }
  Utf8StringSet &operator +=(const QSet<QString> &set) {
    QSet<Utf8String>::operator +=(Utf8StringSet(set.cbegin(), set.cend()));
    return *this; }
  Utf8StringSet &operator +=(const QList<Utf8String> &set) {
    QSet<Utf8String>::operator +=(Utf8StringSet(set.cbegin(), set.cend()));
    return *this; }
  Utf8StringSet &operator +=(const QList<QByteArray> &set) {
    QSet<Utf8String>::operator +=(Utf8StringSet(set.cbegin(), set.cend()));
    return *this; }
  Utf8StringSet &operator +=(const QList<QString> &set) {
    QSet<Utf8String>::operator +=(Utf8StringSet(set.cbegin(), set.cend()));
    return *this; }
  Utf8StringSet &operator +=(const std::set<Utf8String> &set) {
    QSet<Utf8String>::operator +=(Utf8StringSet(set.cbegin(), set.cend()));
    return *this; }
#endif

  Utf8String join(const Utf8String &separator) const;
  Utf8String join(const char separator) const;
  Utf8String sorted_join(const Utf8String &separator) const;
  Utf8String sorted_join(const char separator) const;
  Utf8String headed_join(const Utf8String &separator) const;
  Utf8String headed_join(const char separator) const;
  Utf8String headed_sorted_join(const Utf8String &separator);
  Utf8String headed_sorted_join(const char separator);
  Utf8StringList toList() const;
  Utf8StringList toSortedList() const;
  std::set<Utf8String> toStdSet() const;
  inline operator QVariant() const {
    return QVariant::fromValue(*this); }
};

Q_DECLARE_METATYPE(Utf8StringSet)

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const Utf8StringSet &s);

p6::log::LogHelper LIBP6CORESHARED_EXPORT operator<<(
    p6::log::LogHelper lh, const Utf8StringSet &set);

#endif // UTF8STRINGSET_H
