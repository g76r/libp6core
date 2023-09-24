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

#define UTF8STRING_IMPL_CPP
#include "utf8string.h"
#include "utf8stringlist.h"
#include <set>
#include <QDateTime>

const QList<char> Utf8String::AsciiWhitespace = { ' ', '\t', '\n', '\r', '\v' };
const Utf8String Utf8String::ReplacementCharacterUtf8 = "\xef\xbf\xbd"_u8;
const Utf8String Utf8String::Empty = ""_u8;

#include "util/unicodedata.cpp"

static int staticInit() {
  qMetaTypeId<Utf8String>();
  QMetaType::registerConverter<Utf8String,QVariant>();
  QMetaType::registerConverter<QVariant,Utf8String>([](const QVariant &v) {
    return Utf8String(v); });
  QMetaType::registerConverter<Utf8String,QString>(&Utf8String::toString);
  QMetaType::registerConverter<Utf8String,QByteArray>();
  QMetaType::registerConverter<QString,Utf8String>([](const QString &s) {
    return Utf8String(s); });
  QMetaType::registerConverter<QByteArray,Utf8String>([](const QByteArray &s) {
    return Utf8String(s); });
  QMetaType::registerConverter<Utf8String,bool>(&Utf8String::convToBool);
  QMetaType::registerConverter<Utf8String,double>(&Utf8String::convToDouble);
  QMetaType::registerConverter<Utf8String,float>(&Utf8String::convToFloat);
  QMetaType::registerConverter<Utf8String,qlonglong>(&Utf8String::convToLongLong);
  QMetaType::registerConverter<Utf8String,qulonglong>(&Utf8String::convToULongLong);
  QMetaType::registerConverter<Utf8String,long>(&Utf8String::convToLong);
  QMetaType::registerConverter<Utf8String,ulong>(&Utf8String::convToULong);
  QMetaType::registerConverter<Utf8String,int>(&Utf8String::convToInt);
  QMetaType::registerConverter<Utf8String,uint>(&Utf8String::convToUInt);
  QMetaType::registerConverter<Utf8String,short>(&Utf8String::convToShort);
  QMetaType::registerConverter<Utf8String,ushort>(&Utf8String::convToUShort);
  QMetaType::registerConverter<bool,Utf8String>([](const bool &n) {
    return Utf8String::number(n); });
  QMetaType::registerConverter<double,Utf8String>([](const double &n) {
    return Utf8String::number(n); });
  QMetaType::registerConverter<float,Utf8String>([](const float &n) {
    return Utf8String::number(n); });
  QMetaType::registerConverter<qlonglong,Utf8String>([](const qlonglong &n) {
    return Utf8String::number(n); });
  QMetaType::registerConverter<qulonglong,Utf8String>([](const qulonglong &n) {
    return Utf8String::number(n); });
  QMetaType::registerConverter<long,Utf8String>([](const long &n) {
    return Utf8String::number(n); });
  QMetaType::registerConverter<ulong,Utf8String>([](const ulong &n) {
    return Utf8String::number(n); });
  QMetaType::registerConverter<int,Utf8String>([](const int &n) {
    return Utf8String::number(n); });
  QMetaType::registerConverter<uint,Utf8String>([](const uint &n) {
    return Utf8String::number(n); });
  QMetaType::registerConverter<short,Utf8String>([](const short &n) {
    return Utf8String::number(n); });
  QMetaType::registerConverter<ushort,Utf8String>([](const ushort &n) {
    return Utf8String::number(n); });
  QMetaType::registerConverter<QDateTime,Utf8String>([](const QDateTime &d) {
    return Utf8String(d.toString(Qt::ISODateWithMs)); });
  QMetaType::registerConverter<QDate,Utf8String>([](const QDate &d) {
    return Utf8String(d.toString(Qt::ISODateWithMs)); });
  QMetaType::registerConverter<QTime,Utf8String>([](const QTime &d) {
    return Utf8String(d.toString(Qt::ISODateWithMs)); });
  QMetaType::registerConverter<Utf8String,QDateTime>([](const Utf8String &s) {
    return QDateTime::fromString(s, Qt::ISODateWithMs); });
  QMetaType::registerConverter<Utf8String,QDate>([](const Utf8String &s) {
    return QDate::fromString(s, Qt::ISODateWithMs); });
  QMetaType::registerConverter<Utf8String,QTime>([](const Utf8String &s) {
    return QTime::fromString(s, Qt::ISODateWithMs); });
  // TODO there are more to map in QT_FOR_EACH_STATIC_CORE_CLASS
  // QT_FOR_EACH_STATIC_ALIAS_TYPE QT_FOR_EACH_STATIC_CORE_TEMPLATE
  // etc. (qmetatype.h)
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

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
    bool floating_point_enabled,
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
  if (!_ok && floating_point_enabled) {
    // try to convert to double and then truncate to integer part
    double d = toFloating<double>(s, &_ok, NAN, suffixes_enabled,
                                  [](const QByteArray &s, bool *ok) {
      return s.toDouble(ok);
    });
    if (_ok) { // use double value only if it fits in the integer type
      if (d >= std::numeric_limits<I>::min() &&
          d <= std::numeric_limits<I>::max())
        i = d;
      else
        _ok = false;
    }
  }
  if (ok)
    *ok = _ok;
  if (!_ok)
    return def;
  return i;
}

qlonglong Utf8String::toLongLong(
    bool *ok, int base, qlonglong def,
    bool suffixes_enabled, bool floating_point_enabled) const {
  return toInteger<qlonglong>(
        *this, ok, base, def, suffixes_enabled, floating_point_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toLongLong(ok, base);
  });
}

qulonglong Utf8String::toULongLong(
    bool *ok, int base, qulonglong def,
    bool suffixes_enabled, bool floating_point_enabled) const {
  return toInteger<qulonglong>(
        *this, ok, base, def, suffixes_enabled, floating_point_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toULongLong(ok, base);
  });
}

long Utf8String::toLong(
    bool *ok, int base, long def,
    bool suffixes_enabled, bool floating_point_enabled) const {
  return toInteger<long>(
        *this, ok, base, def, suffixes_enabled, floating_point_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toLong(ok, base);
  });
}

ulong Utf8String::toULong(
    bool *ok, int base, ulong def,
    bool suffixes_enabled, bool floating_point_enabled) const {
  return toInteger<ulong>(
        *this, ok, base, def, suffixes_enabled, floating_point_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toULong(ok, base);
  });
}

int Utf8String::toInt(
    bool *ok, int base, int def,
    bool suffixes_enabled, bool floating_point_enabled) const {
  return toInteger<int>(
        *this, ok, base, def, suffixes_enabled, floating_point_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toInt(ok, base);
  });
}

uint Utf8String::toUInt(
    bool *ok, int base, uint def,
    bool suffixes_enabled, bool floating_point_enabled) const {
  return toInteger<uint>(
        *this, ok, base, def, suffixes_enabled, floating_point_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toUInt(ok, base);
  });
}

short Utf8String::toShort(
    bool *ok, int base, short def,
    bool suffixes_enabled, bool floating_point_enabled) const {
  return toInteger<short>(
        *this, ok, base, def, suffixes_enabled, floating_point_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toShort(ok, base);
  });
}

ushort Utf8String::toUShort(
    bool *ok, int base, ushort def,
    bool suffixes_enabled, bool floating_point_enabled) const {
  return toInteger<ushort>(
        *this, ok, base, def, suffixes_enabled, floating_point_enabled,
        [](const QByteArray &s, bool *ok, int base) {
    return s.toUShort(ok, base);
  });
}

bool Utf8String::toBool(bool *ok, bool def) const {
  bool b = def;
  auto s = trimmed().toLower();
  bool _ok = true;
  qlonglong i;
  // text constants
  if (s == "true"_u8) {
    b = true;
    goto found;
  }
  if (s == "false"_u8) {
    b = false;
    goto found;
  }
  // integer numbers
  i = toLongLong(&_ok);
  if (_ok)
    b = !!i;
  // *ok
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

static inline Utf8String foldCaseWithHoles(
    const char *s, const char *end, std::function<char32_t(char32_t)> fold,
    char32_t replacement) {
  Q_ASSERT(s);
  Q_ASSERT(end);
  Utf8String folded;
  char32_t previous = 0;
  for (; align_on_char(s, end); ++s) {
    auto c = fold(Utf8String::decodeUtf8(s, end));
    if (c == Utf8String::ReplacementCharacter) {
      if (previous != Utf8String::ReplacementCharacter)
        folded += replacement;
    } else
      folded += c;
    previous = c;
  }
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

Utf8String Utf8String::toIdentifier(bool allow_non_ascii) const {
  auto s = constData();
  auto end = s + size();
  auto f = [&allow_non_ascii](char32_t c) {
    if (::isalnum(c))
      return c;
    if (allow_non_ascii && c > 0x7f)
      return c;
    return Utf8String::ReplacementCharacter;
  };
  if (s && (::isdigit(*s) || (allow_non_ascii && (*s & 0x80) == 0)))
    return "_"_u8+foldCaseWithHoles(s, end, f, '_');
  else
    return foldCaseWithHoles(s, end, f, '_');
}

Utf8String Utf8String::toInternetHeaderName(bool ignore_trailing_colon) const {
  auto s = constData();
  auto end = s + size();
  if (ignore_trailing_colon && s < end && end[-1] == ':')
    --end;
  auto f = [](char32_t c) {
    // rfc5322 states that a header may contain any ascii printable char but ':'
    if (c >= 0x21 && c <= 0x7f && c != ':')
      return c;
    return Utf8String::ReplacementCharacter;
  };
  return foldCaseWithHoles(s, end, f, '_');
}

Utf8String Utf8String::toInternetHeaderCase() const {
  auto s = constData();
  auto end = s + size();
  bool leading = true;
  auto f = [&leading](char32_t c) -> char32_t {
    // TODO also (conditionaly) support lowerUpper as a separator
    // TODO include in a more general case conversion scheme
    // with snake, camel, pascal, allcaps, alllower
    if (c == '-' || c == '_' || c == '.' || c == ':' || ::isspace(c)) {
      leading = true;
      return '-';
    }
    if (!leading)
      return Utf8String::toLower(c);
    leading = false;
    return Utf8String::toUpper(c);
  };
  return foldCase(s, end, f);
}

Utf8String Utf8String::toLower() const {
  auto s = constData();
  auto end = s + size();
  return foldCase(s, end, [](char32_t c) { return Utf8String::toLower(c); });
}

Utf8String Utf8String::toTitle() const {
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

const Utf8StringList Utf8String::split_after(
    Utf8String sep, qsizetype offset, Qt::SplitBehavior behavior) const {
  Utf8StringList list;
  auto n = size(), w = sep.size();
  if (offset < 0 || n == 0)
    return {};
  auto data = constData(), sep_data = sep.constData();
  qsizetype imax = n-w+1, i = offset, j = i;
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
      if (i-j > 0 || behavior == Qt::KeepEmptyParts)
        list += mid(j, i-j);
      i += w;
      j = i;
    } else {
      ++i;
    }
  }
  if (i-j > 0 || behavior == Qt::KeepEmptyParts)
    list += mid(j, i-j);
  return list;
}

const Utf8StringList Utf8String::split_after(
    QList<char> seps, qsizetype offset, Qt::SplitBehavior behavior) const {
  Utf8StringList list;
  qsizetype n = size(), i = offset, j = offset;
  if (offset < 0 || n == 0)
    return {};
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
      if (i-j > 0 || behavior == Qt::KeepEmptyParts)
        list += mid(j, i-j);
      j = i+1;
    }
  }
  if (i-j > 0 || behavior == Qt::KeepEmptyParts)
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
  return split_after(Utf8String(sep, eos-sep), csv-begin);
}

const Utf8StringList Utf8String::split_after(
    const char sep, const qsizetype offset, Qt::SplitBehavior behavior) const {
  return split_after(Utf8String(&sep, 1), offset, behavior);
}

const Utf8StringList Utf8String::split(
    QList<char> seps, Qt::SplitBehavior behavior) const {
  return split_after(seps, 0, behavior);
}

const Utf8StringList Utf8String::split(
    const char sep, Qt::SplitBehavior behavior) const {
  return split_after(Utf8String(&sep, 1), 0, behavior);
}

const Utf8StringList Utf8String::split(
    Utf8String sep, Qt::SplitBehavior behavior) const{
  return split_after(sep, 0, behavior);
}

QDebug operator<<(QDebug dbg, const Utf8String &s) {
  return dbg << s.toString();
}

QList<char> Utf8String::toBytesSortedList() const {
  std::set<char> set;
  auto n = size();
  for (int i = 0; i < n; ++i)
    set.insert(at(i));
  return QList<char>(set.cbegin(), set.cend());
}

Utf8String &Utf8String::remove(const char *needle, qsizetype len) {
  Utf8String result;
  if (!*needle || !len)
    return *this;
  auto s = constData();
  auto end = s + size();
  qsizetype i = 0;
  for (; s < end; ++s) {
    if (*s != needle[i]) { // not a match
      result.append(s-i, i+1); // copy partial match + current char
      i = 0;
      continue;
    }
    if (++i == len) // matched
      i = 0; // skip matched bytes and reset
  }
  result.append(s-i, i); // copy partial match
  return *this = result;
}

Utf8String &Utf8String::remove_ascii_chars(QList<char> chars) {
  Utf8String result;
  auto s = constData();
  auto end = s + size();
  for (; s < end; ++s)
    if (!chars.contains(*s))
      result += *s;
  return *this = result;
}

inline bool isoctal(const char c) {
  return c >= '0' && c <= '7';
}

static const qint8 _hexdigits[] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

inline qsizetype decode_oct_char(const char *s, const char *end, char32_t *c) {
  *c = 0;
  auto begin = s, max = std::min(end, s+3);
  for (; s < max && isoctal(*s); ++s)
    *c = (*c << 3) + (*s - '0');
  return s-begin;
}

inline qsizetype decode_hex_char(
    const char *s, const char *end, qsizetype digits, char32_t *c) {
  *c = 0;
  if (s+digits >= end)
    return 0;
  auto begin = s;
  if (digits)
    end = s+digits;
  for (; s < end; ++s) {
    auto i = _hexdigits[static_cast<unsigned char>(*s)];
    if (i < 0)
      return digits ? 0 : s-begin;
    *c = (*c << 4) + i;
  }
  return digits;
}

Utf8String Utf8String::fromCEscaped(const char *s, qsizetype len) {
  if (!s) [[unlikely]]
    return {};
  Utf8String result;
  auto end = s + len;
  char32_t c;
  qsizetype taken;
  for (; s < end; ++s) {
    if (*s == '\\') {
      ++s;
      if (s == end)
        break;
      switch (*s) {
        case 'a':
          result += '\a';
          break;
        case 'b':
          result += '\b';
          break;
        case 'f':
          result += '\f';
          break;
        case 'n':
          result += '\n';
          break;
        case 'r':
          result += '\r';
          break;
        case 't':
          result += '\t';
          break;
        case 'v':
          result += '\v';
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          // TODO hande o{} C++23
          // TODO hande N{} C++23
          if ((taken = decode_oct_char(s, end, &c)))
            result += c < 0x80 ? c : ReplacementCharacter;
          s += taken-1;
          break;
        case 'x':
          // TODO hande x{} C++23
          if ((taken = decode_hex_char(++s, end, 0, &c)))
            result += c < 0x80 ? c : ReplacementCharacter;
          s += taken-1;
          break;
        case 'u':
          // TODO hande u{} C++23
          if (decode_hex_char(++s, end, 4, &c))
            result += c;
          s += 3;
          break;
        case 'U':
          // TODO hande U{} C++23
          if (decode_hex_char(++s, end, 8, &c))
            result += c;
          s += 7;
          break;
        default:
          // \ ? ' " are standard; anything else is implementation defined, we
          // decided to handle this way too
          goto as_is;
      }
      continue;
    }
as_is:
    result += *s;
  }
  return result;
}
