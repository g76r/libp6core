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
#include <QtDebug>
#include "log/log.h"

const QList<char> Utf8String::AsciiWhitespace = { ' ', '\t', '\n', '\r', '\v' };
const Utf8String Utf8String::ReplacementCharacterUtf8 = "\xef\xbf\xbd"_u8;
const Utf8String Utf8String::Empty = ""_u8;

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
  if (s == "true") {
    b = true;
    goto found;
  }
  if (s == "false") {
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

static inline const char *first_char(const char *&s, const char *end) {
  while (s+3 < end && s[0] == (char)0xef && s[1] == (char)0xbb
         && s[2] == (char)0xbf) // BOM
    s += 3;
  return (s < end) ? s : 0;
}

static inline const char *next_char(const char *&s, const char *end) {
  while (s+3 < end && s[0] == (char)0xef && s[1] == (char)0xbb
         && s[2] == (char)0xbf) // BOM
    s += 3;
  if ((s[0]&0b10000000) == 0) {  // ascii 7 : one byte
    ++s;
    goto found;
  }
  ++s; // ignore leading bytes, should be 0b11xxxxxx but anyway we skip it
  while (s+1 < end && (s[0]&0b11000000) == 0b10000000)
    ++s; // skip all continuation (0b10xxxxxx) bytes, no matter their number
  // rationale: they should be 1 to 3 depending on the leading byte, but anyway
  // we are to ignore malformed sequences if any
  while (s+3 < end && s[0] == (char)0xef && s[1] == (char)0xbb
         && s[2] == (char)0xbf) // BOM
    s += 3;
found:
  return (s < end) ? s : 0;
}

static inline const char *prev_char(const char *&s, const char *begin) {
  --s;
  while (s-3 >= begin && s[-2] == (char)0xef && s[-1] == (char)0xbb
         && s[0] == (char)0xbf) // BOM
    s -= 3;
  while (s-1 >= begin && (s[0]&0b11000000) == 0b10000000)
    --s; // skip all continuation (0b10xxxxxx) bytes, no matter their number
  while (s-3 >= begin && s[-2] == (char)0xef && s[-1] == (char)0xbb
         && s[0] == (char)0xbf) // BOM
    s -= 3;
  return (s >= begin) ? s : 0;
}

qsizetype Utf8String::utf8Size() const {
  auto s = constData();
  auto end = s + size();
  if (!first_char(s, end))
    return 0;
  qsizetype i = 0;
  while (next_char(s, end))
    ++i;
  return i;
}

Utf8String Utf8String::utf8Value(
    qsizetype i, const char *s, qsizetype len, const Utf8String &def) {
  Q_ASSERT(s);
  Q_ASSERT(len >= 0);
  auto end = s + len;
  if (!first_char(s, end))
    return def;
  qsizetype j = 0;
  while (j < i && next_char(s, end))
    ++j;
  return Utf8String(s, end-s);
}

char32_t Utf8String::utf32Value(
    qsizetype i, const char *s, qsizetype len, const char32_t def) {
  Utf8String utf8 = utf8Value(i, s, len, {});
  return utf8.isNull() ? def : decodeUtf8(utf8);
}

Utf8String Utf8String::utf8Left(qsizetype len) const {
  auto s = constData();
  auto end = s + size();
  auto s0 = first_char(s, end);
  qsizetype i = 0;
  do
    ++i;
  while (i <= len && next_char(s, end));
  return Utf8String(s0, s-s0);
}

Utf8String Utf8String::utf8Right(qsizetype len) const {
  auto begin = constData();
  auto s = begin + size(), end = s;
  qsizetype i = 0;
  do
    ++i;
  while (i <= len && prev_char(s, begin));
  return Utf8String(s, end-s);
}

Utf8String Utf8String::utf8Mid(qsizetype pos, qsizetype len) const {
  auto s = constData(), s0 = s;
  auto end = s + size();
  first_char(s, end);
  qsizetype i = 0;
  do
    ++i;
  while (i <= pos && next_char(s, end));
  if (len < 0)
    return Utf8String(s, size()-(s-s0));
  s0 = s;
  i = 0;
  do
    ++i;
  while (i <= len && next_char(s, end));
  return Utf8String(s0, s-s0);
}

const Utf8StringList Utf8String::split(
    Utf8String sep, qsizetype offset, Qt::SplitBehavior behavior) const {
  Utf8StringList list;
  if (offset < 0)
    return {};
  auto data = constData(), sep_data = sep.constData();
  qsizetype imax = size()-sep.size()+1, w = sep.size(), i = offset, j = i;
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
  while (i < imax) {
    if (::strncmp(data+i, sep_data, w) == 0) {
      if (i-j >= (behavior == Qt::SkipEmptyParts ? 1 : 0))
        list += mid(j, i-j);
      i += w;
      j = i;
    } else {
      ++i;
    }
  }
  if (i-j >= (behavior == Qt::SkipEmptyParts ? 1 : 0))
    list += mid(j, i-j);
  return list;
}

const Utf8StringList Utf8String::split(
    QList<char> seps, qsizetype offset, Qt::SplitBehavior behavior) const {
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
  QChar z;
  if (offset < 0 || size() < offset+1)
    return {};
  auto s = constData(), orig = s;
  auto end = s + size();
  first_char(s, end);
  while (offset && next_char(s, end))
    --offset;
  auto s0 = s; // begin of separator
  auto s1 = next_char(s, end); // end of separator
  next_char(s, end); // begin of next char
  return split(Utf8String(s0, s1-s0), s1-orig);
}

const Utf8StringList Utf8String::split(
    QList<char> seps, Qt::SplitBehavior behavior) const {
  return split(seps, 0, behavior);
}

const Utf8StringList Utf8String::split(
    const char sep, const qsizetype offset, Qt::SplitBehavior behavior) const {
  return split(QList<char>{sep}, offset, behavior);
}

const Utf8StringList Utf8String::split(
    const char sep, Qt::SplitBehavior behavior) const {
  return split(sep, 0, behavior);
}

QDebug operator<<(QDebug dbg, const Utf8String &s) {
  return dbg << s.toString();
}

