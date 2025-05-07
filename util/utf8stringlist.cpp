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
#include "utf8stringlist.h"
#include "utf8stringset.h"
#include "log/log.h"

static int staticInit() {
  qRegisterMetaType<Utf8StringList>();
  qRegisterMetaType<Utf8StringSet>();
  qRegisterMetaType<Utf8StringIndexedConstList>();
  QMetaType::registerConverter<Utf8StringList,QVariant>();
  QMetaType::registerConverter<Utf8StringSet,QVariant>();
  QMetaType::registerConverter<Utf8StringIndexedConstList,QVariant>();
  QMetaType::registerConverter<Utf8StringList,Utf8String>(
        [](const Utf8StringList &v) STATIC_LAMBDA -> Utf8String {
    return v.join(' ');
  });
  QMetaType::registerConverter<Utf8StringSet,Utf8String>(
        [](const Utf8StringSet &v) STATIC_LAMBDA -> Utf8String {
    return v.sorted_join(' '); });
  QMetaType::registerConverter<Utf8StringIndexedConstList,Utf8String>(
        [](const Utf8StringIndexedConstList &v) STATIC_LAMBDA -> Utf8String {
    return v.join(' '); });
  QMetaType::registerConverter<Utf8StringList,QString>(
        [](const Utf8StringList &v) STATIC_LAMBDA -> QString {
    return v.join(' '); });
  QMetaType::registerConverter<Utf8StringSet,QString>(
        [](const Utf8StringSet &v) STATIC_LAMBDA -> QString {
    return v.sorted_join(' '); });
  QMetaType::registerConverter<Utf8StringIndexedConstList,QString>(
        [](const Utf8StringIndexedConstList &v) STATIC_LAMBDA -> QString {
    return v.join(' '); });
#if REGISTER_CONVERSION_FROM_UTF8STRINGCONTAINERS_TO_QBYTEARRAY
  // not sure it's semantically correct to convert to a binary data type
  QMetaType::registerConverter<Utf8StringList,QByteArray>(
        [](const Utf8StringList &v) STATIC_LAMBDA -> QByteArray {
    return v.join(' '); });
  QMetaType::registerConverter<Utf8StringSet,QByteArray>(
        [](const Utf8StringSet &v) STATIC_LAMBDA -> QByteArray {
    return v.sorted_join(' '); });
  QMetaType::registerConverter<Utf8StringIndexedConstList,QByteArray>(
        [](const Utf8StringIndexedConstList &v) STATIC_LAMBDA -> QByteArray {
    return v.join(' '); });
#endif
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

template<typename C,typename T>
static inline Utf8String join(const C &container, const T &separator) {
  Utf8String joined;
  bool first = true;
  for (const auto &s: container) {
    if (first)
      first = false;
    else
      joined += separator;
    joined += s;
  }
  return joined;
}

template<typename C,typename T>
static inline Utf8String headed_join(const C &container, const T &separator) {
  Utf8String joined;
  for (const auto &s: container) {
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

Utf8String Utf8StringList::headed_join(const Utf8String &separator) const {
  return ::headed_join(*this, separator);
}

Utf8String Utf8StringList::headed_join(const char separator) const {
  return ::headed_join(*this, separator);
}

Utf8String Utf8StringSet::join(const Utf8String &separator) const {
  return ::join(*this, separator);
}

Utf8String Utf8StringSet::join(const char separator) const {
  return ::join(*this, separator);
}

Utf8String Utf8StringSet::headed_join(const Utf8String &separator) const {
  return ::headed_join(*this, separator);
}

Utf8String Utf8StringSet::headed_join(const char separator) const {
  return ::headed_join(*this, separator);
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

Utf8String Utf8StringList::human_readable() const {
  auto s = "{ "_u8;
  if (size())
    s += "\""_u8+join("\", \""_u8)+"\" "_u8;
  s += "}"_u8;
  return s;
}

QDebug operator <<(QDebug dbg, const Utf8StringList &list) {
  return dbg.noquote() << list.human_readable().toUtf16();
}

p6::log::LogHelper operator<<(
    p6::log::LogHelper lh, const Utf8StringList &list) {
  lh << list.human_readable();
  return lh;
}

QDebug operator<<(QDebug dbg, const Utf8StringSet &set) {
  return dbg << set.toList();
}

p6::log::LogHelper operator<<(p6::log::LogHelper lh, const Utf8StringSet &set) {
  return lh << set.toList();
}

Utf8StringSet Utf8StringList::toSet() const {
  return Utf8StringSet(*this);
}

Utf8StringList Utf8StringList::toSortedDeduplicatedList() const {
  return toSet().toSortedList();
}

Utf8String Utf8StringSet::sorted_join(const Utf8String &separator) const {
  return toSortedList().join(separator);
}

Utf8String Utf8StringSet::sorted_join(const char separator) const {
  return toSortedList().join(separator);
}

Utf8String Utf8StringSet::headed_sorted_join(const Utf8String &separator) {
  return toSortedList().headed_join(separator);
}

Utf8String Utf8StringSet::headed_sorted_join(const char separator) {
  return toSortedList().headed_join(separator);
}

Utf8StringList Utf8StringSet::toList() const {
  return Utf8StringList(*this);
}

Utf8StringList Utf8StringSet::toSortedList() const {
  auto list = toList(); std::sort(list.begin(), list.end()); return list;
}
