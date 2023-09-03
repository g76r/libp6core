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
#include "utf8stringlist.h"
#include "utf8stringset.h"
#include "log/log.h"

template<typename C,typename T>
static inline Utf8String join(const C &container, const T &separator) {
  Utf8String joined;
  bool first = true;
  for (auto s: container) {
    if (first)
      first = false;
    else
      joined += separator;
    joined += s;
  }
  return joined;
}

Utf8String Utf8StringList::join(const Utf8String &separator) const {
  return ::join(*this, separator);
}

Utf8String Utf8StringList::join(const char separator) const {
  return ::join(*this, separator);
}

Utf8String Utf8StringSet::join(const Utf8String &separator) const {
  return ::join(*this, separator);
}

Utf8String Utf8StringSet::join(const char separator) const {
  return ::join(*this, separator);
}

QVariant Utf8StringList::paramRawValue(
    const Utf8String &key, const QVariant &def,
    const EvalContext &) const {
  bool ok;
  int i = key.toInt(&ok);
  if (!ok)
    return def;
  if (i == 0)
    return join(' ');
  if (i < 1 || i >= size()+1)
    return def;
  return operator[](i-1);
}

Utf8StringSet Utf8StringList::paramKeys(const EvalContext &) const {
  Utf8StringSet keys;
  qsizetype n = size();
  for (qsizetype i = 0; i <= n; ++i)
    keys << Utf8String::number(i+1);
  return keys;
}

QDebug operator<<(QDebug dbg, const Utf8StringList &list) {
  auto s = "{ "_u8;
  if (list.size())
    s += "\""_u8+list.join("\", \""_u8)+"\" "_u8;
  s += "}"_u8;
  return dbg.noquote() << s.toString();
}

LogHelper operator<<(LogHelper lh, const Utf8StringList &list) {
  lh << "{"_u8;
  if (list.size())
    lh << "\""_u8+list.join("\", \""_u8)+"\" }"_u8;
  return lh;
}

QDebug operator<<(QDebug dbg, const Utf8StringSet &set) {
  return dbg << set.toSortedList();
}

Utf8StringSet Utf8StringList::toSet() const {
  return Utf8StringSet(*this);
}

Utf8StringList Utf8StringList::toSortedDeduplicatedList() const {
  return toSet().toSortedList();
}

Utf8String Utf8StringSet::sortedJoin(const Utf8String &separator) {
  return toSortedList().join(separator);
}

Utf8String Utf8StringSet::sortedJoin(const char separator) {
  return toSortedList().join(separator);
}

Utf8StringList Utf8StringSet::toList() const {
  return Utf8StringList(*this);
}

Utf8StringList Utf8StringSet::toSortedList() const {
  auto list = toList(); std::sort(list.begin(), list.end()); return list;
}
