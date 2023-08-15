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
#include "utf8stringlist.h"

const QList<char> Utf8String::AsciiWhitespace = { ' ', '\t', '\n', '\r', '\v' };
const Utf8String Utf8String::ReplacementCharacterUtf8 = "\xef\xbf\xbd"_u8;
const Utf8String Utf8String::Empty = ""_u8;

#include "util/unicodedata.cpp"

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

/** Increment s until a start of char sequence is reached.
 *  If s is already at a char start, do nothing.
 *  Actually: skips BOMs and (erroneous) out of sequence bytes.
 *  Returns 0 if end is reached.
 */
static inline const char *align_on_char(const char *&s, const char *end) {
  // skip every BOM and every unexpected continuation (0b10xxxxxx) byte
  // being it at the end of previous sequence or unexpected
  while ((s+3 <= end && (s[0]&0xff) == 0xef && (s[1]&0xff) == 0xbb &&
          (s[2]&0xff) == 0xbf) || // BOM
         (s+1 <= end && (s[0]&0b11000000) == 0b10000000)) // continuation byte
    ++s;
  return s < end ? s : 0;
}

/** Decrement s until a start of char sequence is reached.
 *  If s is already at a char start, do nothing.
 *  Actually: skips BOMs and (erroneous) out of sequence bytes.
 *  Returns 0 if begin is exceeded.
 */
static inline const char *backward_align_on_char(
    const char *&s, const char *begin) {
  // skip every BOM and every unexpected continuation (0b10xxxxxx) byte
  // being it at the end of previous sequence or unexpected
  while ((s-3 >= begin && (s[-2]&0xff) == 0xef && (s[-1]&0xff) == 0xbb
          && (s[0]&0xff) == 0xbf) || // BOM
         (s-1 >= begin && (s[0]&0b11000000) == 0b10000000)) // continuation byte
    --s;
  return s >= begin ? s : 0;
}

qsizetype Utf8String::utf8Size() const {
  auto s = constData();
  auto end = s + size();
  qsizetype i = 0;
  for (; align_on_char(s, end); ++s)
    ++i;
  return i;
}

Utf8String Utf8String::cleaned(const char *s, const char *end) {
  Q_ASSERT(s);
  Q_ASSERT(end);
  Utf8String cleaned;
  for (; align_on_char(s, end); ++s) {
    auto c = decodeUtf8(s, end);
    auto u = encodeUtf8(c);
    cleaned += u;
  }
  return cleaned;
}

static inline Utf8String foldCase(
    const char *s, const char *end, std::function<char32_t(char32_t)> fold) {
  Q_ASSERT(s);
  Q_ASSERT(end);
  Utf8String folded;
  for (; align_on_char(s, end); ++s)
    folded += fold(Utf8String::decodeUtf8(s, end));
  return folded;
}

static inline bool testCase(
    const char *s, const char *end, std::function<char32_t(char32_t)> fold) {
  Q_ASSERT(s);
  Q_ASSERT(end);
  for (; align_on_char(s, end); ++s) {
    char32_t orig = Utf8String::decodeUtf8(s, end);
    if (orig != fold(orig))
      return false;
  }
  return true;
}

Utf8String Utf8String::toUpper() const {
  auto s = constData();
  auto end = s + size();
  return foldCase(s, end, [](char32_t c) { return Utf8String::toUpper(c); });
}

Utf8String Utf8String::toLower() const {
  auto s = constData();
  auto end = s + size();
  return foldCase(s, end, [](char32_t c) { return Utf8String::toLower(c); });
}

Utf8String Utf8String::toTitle() {
  auto s = constData();
  auto end = s + size();
  return foldCase(s, end, [](char32_t c) { return Utf8String::toTitle(c); });
}

bool Utf8String::isUpper() const {
  auto s = constData();
  auto end = s + size();
  return testCase(s, end, [](char32_t c) { return Utf8String::toUpper(c); });
}

bool Utf8String::isLower() const {
  auto s = constData();
  auto end = s + size();
  return testCase(s, end, [](char32_t c) { return Utf8String::toLower(c); });
}

bool Utf8String::isTitle() const {
  auto s = constData();
  auto end = s + size();
  return testCase(s, end, [](char32_t c) { return Utf8String::toTitle(c); });
}

Utf8String Utf8String::utf8Value(
    qsizetype i, const char *s, qsizetype len, const Utf8String &def) {
  Q_ASSERT(s);
  Q_ASSERT(len >= 0);
  auto end = s + len;
  for (qsizetype j = 0; j < i && align_on_char(s, end); ++j, ++s)
    ;
  return s < end ? Utf8String(s, end-s) : def;
}

char32_t Utf8String::utf32Value(
    qsizetype i, const char *s, qsizetype len, const char32_t def) {
  Utf8String utf8 = utf8Value(i, s, len, {});
  return utf8.isNull() ? def : decodeUtf8(utf8);
}

Utf8String Utf8String::utf8Left(qsizetype len) const {
  auto s = constData();
  auto end = s + size();
  auto begin = align_on_char(s, end);
  for (qsizetype i = 0; i < len && align_on_char(s, end); ++s, ++i)
    ;
  return Utf8String(begin, s-begin);
}

Utf8String Utf8String::utf8Right(qsizetype len) const {
  auto begin = constData();
  auto s = begin + size(), end = s;
  for (qsizetype i = 0; i < len && backward_align_on_char(s, begin); --s, ++i)
    ;
  if (s < begin)
    s  = begin;
  return Utf8String(s, end-s);
}

Utf8String Utf8String::utf8Mid(qsizetype pos, qsizetype len) const {
  auto s = constData(), begin = s;
  auto end = s + size();
  for (qsizetype i = 0; i < pos && align_on_char(s, end); ++s, ++i)
    ;
  if (len < 0)
    return Utf8String(s, size()-(s-begin));
  begin = s;
  for (qsizetype i = 0; i < len && align_on_char(s, end); ++s, ++i)
    ;
  return Utf8String(begin, s-begin);
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
  auto begin = constData();
  auto s = begin + offset;
  auto end = begin + size();
  auto sep = align_on_char(s, end); // begin of leading separator
  auto csv = align_on_char(++s, end); // begin of char separated values
  auto eos = sep+1; // end of separator (byte after the end of the separator)
  for (; eos < csv && (eos[0]&0b11000000) == 0b10000000; ++eos)
    ; // go forward over continuation bytes
  //qDebug() << "splitByLeadingChar" << offset << Utf8String(sep, eos-sep).toHex() << csv;
  return split(Utf8String(sep, eos-sep), csv-begin);
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
