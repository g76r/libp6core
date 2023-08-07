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

#include "utf8string.h"
#include <functional>

const QList<char> Utf8String::Whitespace = { ' ', '\t', '\n', '\r', '\v' };

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

template<typename F>
static inline F toFloating(
    QByteArray s, bool *ok, F def, bool suffixes_enabled,
    std::function<F(const QByteArray &,bool*)> wrapped) {
  // cannot go further 'P' because 'E' means exponent
  // won't go further 'f' by consistency: 1e15~1e-15
  static const F _multipliers[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1e9, 0, 0, 0, 0, 0, 1e6, 0, 0,
    1e15, 0, 0, 0, 1e12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1e-15, 0, 0, 0, 0, 1e3, 0, 1e-3, 1e-9, 0,
    1e-12, 0, 0, 0, 0, 1e-6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  //for (unsigned char i = 64; i < 124; ++i) {
  //  qDebug() << (char)i << " ->" << _multipliers[i];
  //}
  s = s.trimmed();
  F mul = 1.0;
  if (suffixes_enabled) {
    auto len = s.size();
    if (len >= 2) {
      auto m = _multipliers[static_cast<unsigned char>(s.at(len-1))];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
      if (m != 0) {
#pragma GCC diagnostic pop
        mul = m;
        s.chop(1);
      }
    }
  }
  bool _ok;
  F f = wrapped(s, &_ok);
  if (ok)
    *ok = _ok;
  if (!_ok)
    return def;
  return suffixes_enabled ? f*mul : f;
}

double Utf8String::toDouble(bool *ok, double def, bool suffixes_enabled) const {
  return toFloating<double>(
        *this, ok, def, suffixes_enabled, [](const QByteArray &s, bool *ok) {
    return s.toDouble(ok);
  });
}

float Utf8String::toFloat(bool *ok, float def, bool suffixes_enabled) const {
  return toFloating<float>(
        *this, ok, def, suffixes_enabled, [](const QByteArray &s, bool *ok) {
    return s.toFloat(ok);
  });
}

template<typename I>
static inline I toInteger(
    QByteArray s, bool *ok, int base, I def, bool suffixes_enabled,
    std::function<I(const QByteArray &,bool*,int)> wrapped) {
  s = s.trimmed();
  // won't go further 'P' because 'E' means exponent
  static const QByteArray _multipliers[] {
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, "000000000", {}, {}, {}, {}, {}, "000000", {}, {},
    "000000000000000", {}, {}, {}, "000000000000", {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, "000000000", {}, {}, {}, {}, {}, {}, {}, {}, "000", {}, "000000", {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
    {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
  };
  //for (unsigned char i = 64; i < 124; ++i) {
  //  qDebug() << (char)i << " ->" << _multipliers[i];
  //}
  if (suffixes_enabled) {
    auto len = s.size();
    if (len >= 2) {
      auto m = _multipliers[static_cast<unsigned char>(s.at(len-1))];
      if (!m.isNull())
        s = s.left(len-1)+m;
    }
  }
  bool _ok;
  I i = wrapped(s, &_ok, base);
  if (ok)
    *ok = _ok;
  if (!_ok)
    return def;
  return i;
}

qlonglong Utf8String::toLongLong(bool *ok, int base, qlonglong def,
                                 bool suffixes_enabled) const {
  return toInteger<qlonglong>(
        *this, ok, base, def, suffixes_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toLongLong(ok, base);
  });
}

qulonglong Utf8String::toULongLong(bool *ok, int base, qulonglong def,
                                   bool suffixes_enabled) const {
  return toInteger<qulonglong>(
        *this, ok, base, def, suffixes_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toULongLong(ok, base);
  });
}

long Utf8String::toLong(bool *ok, int base, long def,
                        bool suffixes_enabled) const {
  return toInteger<long>(
        *this, ok, base, def, suffixes_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toLong(ok, base);
  });
}

ulong Utf8String::toULong(bool *ok, int base, ulong def,
                          bool suffixes_enabled) const {
  return toInteger<ulong>(
        *this, ok, base, def, suffixes_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toULong(ok, base);
  });
}

int Utf8String::toInt(bool *ok, int base, int def,
                      bool suffixes_enabled) const {
  return toInteger<int>(
        *this, ok, base, def, suffixes_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toInt(ok, base);
  });
}

uint Utf8String::toUInt(bool *ok, int base, uint def,
                        bool suffixes_enabled) const {
  return toInteger<uint>(
        *this, ok, base, def, suffixes_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toUInt(ok, base);
  });
}

short Utf8String::toShort(bool *ok, int base, short def,
                          bool suffixes_enabled) const {
  return toInteger<short>(
        *this, ok, base, def, suffixes_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toShort(ok, base);
  });
}

ushort Utf8String::toUShort(bool *ok, int base, ushort def,
                            bool suffixes_enabled) const {
  return toInteger<ushort>(
        *this, ok, base, def, suffixes_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toUShort(ok, base);
  });
}

bool Utf8String::toBool(bool *ok, bool def) const {
  bool b = def;
  auto s = static_cast<QByteArray>(trimmed()).toLower();
  bool _ok = true;
  qlonglong i;
  if (s == "true"_ba) {
    b = true;
    goto found;
  }
  if (s == "false"_ba) {
    b = false;
    goto found;
  }
  i = toLongLong(&_ok);
  if (_ok)
    b = !!i;
found:
  if (ok)
    *ok = _ok;
  return b;
}

const Utf8StringList Utf8String::split(
    QList<char> seps, qsizetype offset, Qt::SplitBehavior behavior) const {
// LATER beter support UTF-8, with multi-byte sequences separators
  Utf8StringList list;
  if (offset < 0)
    return {};
  qsizetype n = size(), i = offset, j = offset;
  //  xx,y,zzzz
  //  012345678
  //  4-3=1
  //  2-0=2
  //  ,x,,zzzz
  //  01234567
  //  0-0=0
  //  3-3=0
  //  x,zzzz
  //  012345
  //  6-2=4
  for (; i < n; ++i) {
    if (seps.contains(at(i))) {
      if (i-j >= (behavior == Qt::SkipEmptyParts ? 1 : 0))
        list += mid(j, i-j);
      j = i+1;
    }
  }
  if (i-j >= (behavior == Qt::SkipEmptyParts ? 1 : 0))
    list += mid(j, i-j);
  return list;
}

const Utf8StringList Utf8String::splitByLeadingChar(qsizetype offset) const {
  if (offset < 0 || size() < offset+1)
    return {};
  return mid(offset+1).split(at(offset));
}
