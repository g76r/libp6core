/* Copyright 2023-2024 Gregoire Barbier and others.
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
#include "utf8stringlist.h"
#include <set>
#include <QDateTime>
#include <cfloat>

const QList<char> Utf8String::AsciiWhitespace = {
  ' ', '\t', '\n', '\r', '\v', '\f',
};
const QList<char32_t> Utf8String::UnicodeWhitespace = {
  ' ', '\t', '\n', '\r', '\v', '\f',
  0x85, 0xa0, 0x1680,
  0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006, 0x2007,
  0x2008, 0x2009, 0x200a, 0x2028, 0x2029, 0x202f, 0x205f, 0x3000,
};
const Utf8String Utf8String::ReplacementCharacterUtf8 = "\xef\xbf\xbd"_u8;
const Utf8String Utf8String::Empty = ""_u8;
const Utf8String Utf8String::DefaultEllipsis = "..."_u8;
const Utf8String Utf8String::DefaultPadding = " "_u8;

#include "util/unicodedata.cpp"

static int staticInit() {
  qRegisterMetaType<Utf8String>();
  QMetaType::registerConverter<Utf8String,QVariant>();
  QMetaType::registerConverter<QVariant,Utf8String>([](const QVariant &v) {
    return Utf8String(v); });
  QMetaType::registerConverter<Utf8String,QString>(&Utf8String::toUtf16);
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
    return Utf8String::number((double)n); });
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
    QByteArray ba, bool *ok, F def, bool suffixes_enabled,
    std::function<F(const QByteArray &,bool*)> wrapped) {
  Utf8String s = ba.trimmed();
  F mul = 1.0;
  if (auto len = s.utf8size(); suffixes_enabled && len >= 2) {
    switch (s.utf32value(len-1)) {
      case 'k':
        mul = 1e3;
        break;
      case 'M':
        mul = 1e6;
        break;
      case 'G':
        mul = 1e9;
        break;
      case 'T':
        mul = 1e12;
        break;
      case 'P':
        mul = 1e15;
        break;
        // cannot go further 'P' because 'E' means exponent
      case 'm':
        mul = 1e-3;
        break;
      case 'u':
      case 0xb5: // µ (micro symbol)
      case 0x3bc: // μ (mu greek letter)
        mul = 1e-6;
        break;
      case 'n':
        mul = 1e-9;
        break;
      case 'p':
        mul = 1e-12;
        break;
      case 'f':
        mul = 1e-15;
        break;
        // won't go further 'f' by consistency with 'P': 1e15~1e-15 range
      default:
        goto no_suffix;
    }
    s.utf8chop(1);
  }
no_suffix:
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
inline bool double_fits_integer(double d) {
  if (std::numeric_limits<I>::digits >= DBL_MANT_DIG) {
    // use double value only if it's within DBL_MANT_DIG bits integer range
    // (if double is IEE754 double precision, DBL_MANT_DIG == 53)
    return d >= -(1LL<<DBL_MANT_DIG) && d <= (1LL<<DBL_MANT_DIG);
  }
  // use double value only if it fits in the integer type
  return d >= std::numeric_limits<I>::min() &&
      d <= std::numeric_limits<I>::max();
}

template<typename I>
static inline I toInteger(
    QByteArray s, bool *ok, int base, I def, bool suffixes_enabled,
    bool floating_point_enabled,
    std::function<I(const QByteArray &,bool*,int)> wrapped) {
  s = s.trimmed();
  // accept suffixes only in base 10 (otherwise 0x1b would be 1 billion)
  if (auto len = s.size(); suffixes_enabled && len >= 2
      && (base == 10 || (base == 0 && (s.at(0) != '0'||len == 2)))) {
    switch(s.at(len-1)) {
      case 'k':
        s = s.left(len-1)+"000";
        break;
      case 'm':
      case 'M':
        s = s.left(len-1)+"000000";
        break;
      case 'b':
      case 'G':
        s = s.left(len-1)+"000000000";
        break;
      case 'T':
        s = s.left(len-1)+"000000000000";
        break;
      case 'P':
        s = s.left(len-1)+"000000000000000";
        break;
        // won't go further 'P' because 'E' means exponent
    }
  }
  bool _ok;
  I i = wrapped(s, &_ok, base);
  if (!_ok && floating_point_enabled) {
    // try to convert to double and then truncate to integer part
    double d = toFloating<double>(s, &_ok, (double)NAN, suffixes_enabled,
                                  [](const QByteArray &s, bool *ok) {
      return s.toDouble(ok);
    });
    if(_ok && double_fits_integer<I>(d))
      i = d;
    else
      _ok = false;
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

static inline Utf8String foldCase(
    const char *s, const char *end, std::function<char32_t(char32_t)> fold) {
  Utf8String folded;
  for (; Utf8String::go_forward_to_utf8_char(&s, end); ++s) {
    folded += fold(Utf8String::decode_utf8(s, end));
  }
  return folded;
}

static inline Utf8String foldCaseWithHoles(
    const char *s, const char *end, std::function<char32_t(char32_t)> fold,
    char32_t replacement) {
  Utf8String folded;
  char32_t previous = 0;
  for (; Utf8String::go_forward_to_utf8_char(&s, end); ++s) {
    auto c = fold(Utf8String::decode_utf8(s, end));
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
  for (; Utf8String::go_forward_to_utf8_char(&s, end); ++s) {
    char32_t orig = Utf8String::decode_utf8(s, end);
    if (orig != fold(orig))
      return false;
  }
  return true;
}

Utf8String Utf8String::toUpper() const {
  auto s = constData();
  auto end = s + size();
  return foldCase(s, end, [](char32_t u) { return Utf8String::toUpper(u); });
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
  auto f = [](char32_t u) {
    // rfc5322 states that a header may contain any ascii printable char but ':'
    if (u >= 0x21 && u <= 0x7f && u != ':')
      return u;
    return Utf8String::ReplacementCharacter;
  };
  return foldCaseWithHoles(s, end, f, '_');
}

Utf8String Utf8String::toInternetHeaderCase() const {
  auto s = constData();
  auto end = s + size();
  bool leading = true;
  auto f = [&leading](char32_t u) -> char32_t {
    // TODO also (conditionaly) support lowerUpper as a separator
    // TODO include in a more general case conversion scheme
    // with snake, camel, pascal, allcaps, alllower
    if (u == '-' || u == '_' || u == '.' || u == ':' || ::isspace(u)) {
      leading = true;
      return '-';
    }
    if (!leading)
      return Utf8String::toLower(u);
    leading = false;
    return Utf8String::toUpper(u);
  };
  return foldCase(s, end, f);
}

Utf8String Utf8String::toLower() const {
  auto s = constData();
  auto end = s + size();
  return foldCase(s, end, [](char32_t u) { return Utf8String::toLower(u); });
}

Utf8String Utf8String::toTitle() const {
  auto s = constData();
  auto end = s + size();
  return foldCase(s, end, [](char32_t u) { return Utf8String::toTitle(u); });
}

bool Utf8String::isUpper() const {
  auto s = constData();
  auto end = s + size();
  return testCase(s, end, [](char32_t u) { return Utf8String::toUpper(u); });
}

bool Utf8String::isLower() const {
  auto s = constData();
  auto end = s + size();
  return testCase(s, end, [](char32_t u) { return Utf8String::toLower(u); });
}

bool Utf8String::isTitle() const {
  auto s = constData();
  auto end = s + size();
  return testCase(s, end, [](char32_t u) { return Utf8String::toTitle(u); });
}

Utf8StringList Utf8String::split_after(
    Utf8String sep, qsizetype offset, Qt::SplitBehavior behavior) const {
  Utf8StringList list;
  auto n = size(), w = sep.size();
  if (offset < 0 || n == 0)
    return {};
  auto data = constData(), sep_data = sep.constData();
  qsizetype imax = n-w+1, i = offset, j = i;
  while (i < n) {
    if (i < imax && ::strncmp(data+i, sep_data, w) == 0) {
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

Utf8StringList Utf8String::split_after(
    QList<char> seps, qsizetype offset, Qt::SplitBehavior behavior) const {
  Utf8StringList list;
  qsizetype n = size(), i = offset, j = offset;
  if (offset < 0 || n == 0)
    return {};
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

Utf8StringList Utf8String::split_headed_list(qsizetype offset) const {
  auto begin = constData();
  auto s = begin + offset;
  auto end = begin + size();
  auto sep = go_forward_to_utf8_char(&s, end); // begin of leading separator
  auto csv = go_forward_to_utf8_char(&++s, end); // begin of char separated values
  auto eos = sep+1; // end of separator (byte after the end of the separator)
  for (; eos < csv && (eos[0]&0b11000000) == 0b10000000; ++eos)
    ; // go forward over continuation bytes
  return split_after(Utf8String(sep, eos-sep), csv-begin);
}

Utf8StringList Utf8String::splitByLeadingChar(qsizetype offset) const {
  return split_headed_list(offset);
}

Utf8StringList Utf8String::split_after(
    const char sep, const qsizetype offset, Qt::SplitBehavior behavior) const {
  return split_after(Utf8String(&sep, 1), offset, behavior);
}

Utf8StringList Utf8String::split(
    QList<char> seps, Qt::SplitBehavior behavior) const {
  return split_after(seps, 0, behavior);
}

Utf8StringList Utf8String::split(
    const char sep, Qt::SplitBehavior behavior) const {
  return split_after(Utf8String(&sep, 1), 0, behavior);
}

Utf8StringList Utf8String::split(
    Utf8String sep, Qt::SplitBehavior behavior) const{
  return split_after(sep, 0, behavior);
}

QDebug operator<<(QDebug dbg, const Utf8String &s) {
  return dbg << s.toUtf16();
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

inline qsizetype decode_oct_char(const char *s, const char *end, char32_t *u) {
  *u = 0;
  auto begin = s, max = std::min(end, s+3);
  for (; s < max && isoctal(*s); ++s)
    *u = (*u << 3) + (*s - '0');
  return s-begin;
}

inline qint8 hex_digit_value(char c) {
  switch(c) {
    case '0':
      return 0;
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    case '7':
      return 7;
    case '8':
      return 8;
    case '9':
      return 9;
    case 'a':
    case 'A':
      return 10;
    case 'b':
    case 'B':
      return 11;
    case 'c':
    case 'C':
      return 12;
    case 'd':
    case 'D':
      return 13;
    case 'e':
    case 'E':
      return 14;
    case 'f':
    case 'F':
      return 15;
  };
  return -1;
}

inline qsizetype decode_hex_char(
    const char *s, const char *end, qsizetype digits, char32_t *u) {
  *u = 0;
  if (s+digits >= end)
    return 0;
  auto begin = s;
  if (digits)
    end = s+digits;
  for (; s < end; ++s) {
    auto i = hex_digit_value(*s);
    if (i < 0)
      return digits ? 0 : s-begin;
    *u = (*u << 4) + i;
  }
  return digits;
}

Utf8String Utf8String::fromCEscaped(const char *s, qsizetype len) {
  if (!s) [[unlikely]]
    return {};
  Utf8String result;
  auto end = s + len;
  char32_t u;
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
          // LATER handle o{} C++23
          // LATER handle N{} C++23
          if ((taken = decode_oct_char(s, end, &u)))
            result += u < 0x80 ? u : ReplacementCharacter;
          s += taken-1;
          break;
        case 'x':
          // LATER handle x{} C++23
          if ((taken = decode_hex_char(++s, end, 0, &u)))
            result += u < 0x80 ? u : ReplacementCharacter;
          s += taken-1;
          break;
        case 'u':
          // LATER handle u{} C++23
          if (decode_hex_char(++s, end, 4, &u))
            result += u;
          s += 3;
          break;
        case 'U':
          // LATER handle U{} C++23
          if (decode_hex_char(++s, end, 8, &u))
            result += u;
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

Utf8String &Utf8String::replace(
    const QRegularExpression &re, const Utf8String &after) {
  return *this = toUtf16().replace(re, after);
}
