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

#define UTF8STRING_IMPL_CPP
#include "utf8stringlist.h"
#include <set>
#include <QDateTime>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QLineF>

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
const int Utf8String::MetaTypeId = qMetaTypeId<Utf8String>();

#include "util/unicodedata.cpp"

static int staticInit() {
  QMetaType::registerConverter<Utf8String,QVariant>();
  Q_ASSERT(QMetaType::canConvert(QMetaType::fromType<Utf8String>(),
                                 QMetaType::fromType<QVariant>()));
  QMetaType::registerConverter<QVariant,Utf8String>(
        [](const QVariant &v) STATIC_LAMBDA { return Utf8String(v); });
  Q_ASSERT(QMetaType::canConvert(QMetaType::fromType<QVariant>(),
                                 QMetaType::fromType<Utf8String>()));
  QMetaType::registerConverter<Utf8String,QString>(&Utf8String::toUtf16);
  QMetaType::registerConverter<Utf8String,QByteArray>();
  QMetaType::registerConverter<QString,Utf8String>(
        [](const QString &s) STATIC_LAMBDA { return Utf8String(s); });
  QMetaType::registerConverter<QByteArray,Utf8String>(
        [](const QByteArray &s) STATIC_LAMBDA { return Utf8String(s); });
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
  QMetaType::registerConverter<bool,Utf8String>(
        [](const bool &n) STATIC_LAMBDA { return Utf8String::number(n); });
  QMetaType::registerConverter<double,Utf8String>(
        [](const double &n) STATIC_LAMBDA { return Utf8String::number(n); });
  QMetaType::registerConverter<float,Utf8String>(
        [](const float &n) STATIC_LAMBDA { return Utf8String::number((double)n); });
  QMetaType::registerConverter<qlonglong,Utf8String>(
        [](const qlonglong &n) STATIC_LAMBDA { return Utf8String::number(n); });
  QMetaType::registerConverter<qulonglong,Utf8String>(
        [](const qulonglong &n) STATIC_LAMBDA {
    return Utf8String::number(n); });
  QMetaType::registerConverter<long,Utf8String>(
        [](const long &n) STATIC_LAMBDA { return Utf8String::number(n); });
  QMetaType::registerConverter<ulong,Utf8String>(
        [](const ulong &n) STATIC_LAMBDA { return Utf8String::number(n); });
  QMetaType::registerConverter<int,Utf8String>(
        [](const int &n) STATIC_LAMBDA { return Utf8String::number(n); });
  QMetaType::registerConverter<uint,Utf8String>(
        [](const uint &n) STATIC_LAMBDA { return Utf8String::number(n); });
  QMetaType::registerConverter<short,Utf8String>(
        [](const short &n) STATIC_LAMBDA { return Utf8String::number(n); });
  QMetaType::registerConverter<ushort,Utf8String>(
        [](const ushort &n) STATIC_LAMBDA { return Utf8String::number(n); });
  QMetaType::registerConverter<QDateTime,Utf8String>(
        [](const QDateTime &d) STATIC_LAMBDA { return Utf8String(d.toString(Qt::ISODateWithMs)); });
  QMetaType::registerConverter<QDate,Utf8String>(
        [](const QDate &d) STATIC_LAMBDA { return Utf8String(d.toString(Qt::ISODateWithMs)); });
  QMetaType::registerConverter<QTime,Utf8String>(
        [](const QTime &d) STATIC_LAMBDA { return Utf8String(d.toString(Qt::ISODateWithMs)); });
  QMetaType::registerConverter<Utf8String,QDateTime>(
        [](const Utf8String &s) STATIC_LAMBDA { return QDateTime::fromString(s, Qt::ISODateWithMs); });
  QMetaType::registerConverter<Utf8String,QDate>(
        [](const Utf8String &s) STATIC_LAMBDA { return QDate::fromString(s, Qt::ISODateWithMs); });
  QMetaType::registerConverter<Utf8String,QTime>(
        [](const Utf8String &s) STATIC_LAMBDA { return QTime::fromString(s, Qt::ISODateWithMs); });
  QMetaType::registerConverter<QPointF,Utf8String>(
        [](const QPointF &p) STATIC_LAMBDA { return Utf8String(p); });
  QMetaType::registerConverter<Utf8String,QPointF>(&Utf8String::toPointF);
  QMetaType::registerConverter<QSizeF,Utf8String>(
        [](const QSizeF &p) STATIC_LAMBDA { return Utf8String(p); });
  QMetaType::registerConverter<Utf8String,QSizeF>(&Utf8String::toSizeF);
  QMetaType::registerConverter<QRectF,Utf8String>(
        [](const QRectF &p) STATIC_LAMBDA { return Utf8String(p); });
  QMetaType::registerConverter<Utf8String,QRectF>(&Utf8String::toRectF);
  QMetaType::registerConverter<QLineF,Utf8String>(
        [](const QLineF &p) STATIC_LAMBDA { return Utf8String(p); });
  QMetaType::registerConverter<Utf8String,QLineF>(&Utf8String::toLineF);
  QMetaType::registerConverter<QList<QPointF>,Utf8String>(
        [](const QList<QPointF> &p) STATIC_LAMBDA { return Utf8String(p); });
  QMetaType::registerConverter<Utf8String,QList<QPointF>>(&Utf8String::toPointFList);
  QMetaType::registerConverter<QList<double>,Utf8String>(
        [](const QList<double> &p) STATIC_LAMBDA { return Utf8String(p); });
  QMetaType::registerConverter<Utf8String,QList<double>>(&Utf8String::toNumberList);
  QMetaType::registerConverter<QList<qint64>,Utf8String>(
        [](const QList<qint64> &p) STATIC_LAMBDA { return Utf8String(p); });
  QMetaType::registerConverter<Utf8String,QList<qint64>>(&Utf8String::toNumberList);
  QMetaType::registerConverter<QList<quint64>,Utf8String>(
        [](const QList<quint64> &p) STATIC_LAMBDA { return Utf8String(p); });
  QMetaType::registerConverter<Utf8String,QList<quint64>>(&Utf8String::toNumberList);

  // TODO there are more to map in QT_FOR_EACH_STATIC_CORE_CLASS
  // QT_FOR_EACH_STATIC_ALIAS_TYPE QT_FOR_EACH_STATIC_CORE_TEMPLATE
  // etc. (qmetatype.h)
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

Utf8String::Utf8String(QPointF point) {
  *this = Utf8String::number(point.x())+","_u8
      +Utf8String::number(point.y());
}

Utf8String::Utf8String(QSizeF size) {
  *this = Utf8String::number(size.width())+","_u8
      +Utf8String::number(size.height());
}

Utf8String::Utf8String(QRectF rect) {
  *this = Utf8String::number(rect.x())+","_u8
      +Utf8String::number(rect.y())+","_u8
      +Utf8String::number(rect.width())+","_u8
      +Utf8String::number(rect.height());
}

Utf8String::Utf8String(QLineF line) {
  *this = Utf8String::number(line.x1())+","_u8
      +Utf8String::number(line.y2())+","_u8
      +Utf8String::number(line.x2())+","_u8
      +Utf8String::number(line.y2());
}

Utf8String::Utf8String(QList<QPointF> list) {
  for (bool begin = true; const auto &point: list) {
    if (begin)
      begin = false;
    else
      append(' ');
    append(Utf8String::number(point.x()));
    append(',');
    append(Utf8String::number(point.y()));
  }
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

// TODO turn into template
static inline Utf8String foldCase(
    const char *s, const char *end, std::function<char32_t(char32_t)> fold) {
  Utf8String folded;
  for (; Utf8String::go_forward_to_utf8_char(&s, end); ++s) {
    folded += fold(Utf8String::decode_utf8(s, end));
  }
  return folded;
}

// TODO turn into template
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

// TODO turn into template
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

Utf8String Utf8String::toIdentifier(bool allow_non_ascii) const {
  auto s = constData();
  auto end = s + size();
  auto f = [&allow_non_ascii](char32_t c) -> char32_t {
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
  auto f = [](char32_t u) STATIC_LAMBDA -> char32_t {
    // rfc5322 states that a header may contain any ascii printable char but ':'
    if (u > 0x20 && u < 0x7f && u != ':')
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
    // with snake_case, camelCase, PascalCase, kebab-case, flatcase,
    // CONSTANT_CASE, Internet-Header-Case, TRAIN-CASE UPPERFLATCASE,
    // CAMEL_SNAKE_CASE, space case, Title Case, namespace::case, dot.case
    // plus prefix_if_invalid (like _ in C identifiers), merge_invalids
    // may also provide a split function: {"splited","parts"}
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

Utf8String Utf8String::toUpper() const {
  auto s = constData();
  auto end = s + size();
  return foldCase(
        s, end, static_cast<char32_t(*)(char32_t)>(&Utf8String::toUpper));
}

Utf8String Utf8String::toLower() const {
  auto s = constData();
  auto end = s + size();
  return foldCase(
        s, end, static_cast<char32_t(*)(char32_t)>(&Utf8String::toLower));
}

Utf8String Utf8String::toTitle() const {
  auto s = constData();
  auto end = s + size();
  return foldCase(
        s, end, static_cast<char32_t(*)(char32_t)>(&Utf8String::toTitle));
}

bool Utf8String::isUpper() const {
  auto s = constData();
  auto end = s + size();
  return testCase(
        s, end, static_cast<char32_t(*)(char32_t)>(&Utf8String::toUpper));
}

bool Utf8String::isLower() const {
  auto s = constData();
  auto end = s + size();
  return testCase(
        s, end, static_cast<char32_t(*)(char32_t)>(&Utf8String::toLower));
}

bool Utf8String::isTitle() const {
  auto s = constData();
  auto end = s + size();
  return testCase(
        s, end, static_cast<char32_t(*)(char32_t)>(&Utf8String::toTitle));
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

static inline qint8 hex_digit_value(char c) {
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

/** convert 4 into '4' and 13 into 'd', assuming i < 16 */
static inline char hex_digit(quint8 i) {
  Q_ASSERT(i < 16);
  if (i < 10)
    return '0'+i;
  return 'a'+i-10;
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

/** return 0 for non escapable chars, their escape char if any or -1 for hex
 *  sequences.
 *  e.g. '\n' -> 'n'
 *       '\x19' '\0' '\1' and '\x7f' -> -1
 *       'a' 'Z' ' ' and '\x80' -> 0
 *       '\'' -> '\''
 *       '"' -> '"'
 */
static inline qsizetype c_escape_char(char c) {
  if (c == '\n')
    return 'n';
  if (c == '\r')
    return 'r';
  if (c == '\t')
    return 't';
  if (c == '\\')
    return '\\';
  if (c == '\'')
    return '\'';
  if (c == '"')
    return '"';
  if ((quint8)c < 32 || c == 127)
    return -1; // to be escaped with hex sequence
  [[likely]] return 0; // not to be escaped
}

Utf8String Utf8String::cEscaped(char c) {
  auto escape = c_escape_char(c);
  if (escape < 0 || ((quint8)c) > 0x7f)
    return "\\x"_u8
        + hex_digit(((quint8)c) >> 4)
        + hex_digit(((quint8)c) & 0x0f);
  if (escape > 0)
    return "\\"_u8+c;
  [[likely]] return Utf8String(c);
}

Utf8String Utf8String::cEscaped() const {
  const char *begin = constData(), *s = begin;
  for (; *s; ++s)
    if (c_escape_char(*s))
      [[unlikely]] goto need_escaping;
  return *this; // short path: copy nothing because there was nothing to escape
need_escaping:;
  Utf8String output = sliced(0, s-begin);
  for (; *s; ++s) {
    qsizetype escape = c_escape_char(*s);
    if (escape < 0) {
      output += '\\';
      output += 'x';
      output += hex_digit(((quint8)*s) >> 4);
      output += hex_digit(((quint8)*s) & 0xf);
      continue;
    }
    if (escape > 0) {
      output += '\\';
      output += (char)escape;
      continue;
    }
    output += *s;
  }
  return output;
}

Utf8String Utf8String::asciiCEscaped(char32_t u) {
  if (u > 0xffff)
    return "\\U"_u8
        + hex_digit(u >> 28)
        + hex_digit((u >> 24) & 0xf)
        + hex_digit((u >> 20) & 0xf)
        + hex_digit((u >> 16) & 0xf)
        + hex_digit((u >> 12) & 0xf)
        + hex_digit((u >> 8) & 0xf)
        + hex_digit((u >> 4) & 0xf)
        + hex_digit(u & 0xf);
  if (u > 0x7f)
    return "\\u"_u8
        + hex_digit(u >> 12)
        + hex_digit((u >> 8) & 0xf)
        + hex_digit((u >> 4) & 0xf)
        + hex_digit(u & 0xf);
  [[likely]] return cEscaped((char)u);
}

Utf8String Utf8String::asciiCEscaped() const {
  const char *begin = constData(), *s = begin, *end = s+size();
  for (; go_forward_to_utf8_char(&s, end); ++s) {
    char32_t u = decode_utf8(s, end);
    if (u > 0x7f || c_escape_char(u))
      [[unlikely]] goto need_escaping;
  }
  return *this; // short path: copy nothing because there was nothing to escape
need_escaping:;
  Utf8String output = sliced(0, s-begin);
  // for (; go_forward_to_utf8_char(&s, end); ++s) {
  //   char32_t u = decode_utf8(s, end);
  while (s < end) {
    auto u = decode_utf8_and_step_forward(&s, end);
    if (u > 0xffff) {
      output += '\\';
      output += 'U';
      output += hex_digit(u >> 28);
      output += hex_digit((u >> 24) & 0xf);
      output += hex_digit((u >> 20) & 0xf);
      output += hex_digit((u >> 16) & 0xf);
      output += hex_digit((u >> 12) & 0xf);
      output += hex_digit((u >> 8) & 0xf);
      output += hex_digit((u >> 4) & 0xf);
      output += hex_digit(u & 0xf);
      continue;
    }
    if (u > 0x7f) {
      output += '\\';
      output += 'u';
      output += hex_digit(u >> 12);
      output += hex_digit((u >> 8) & 0xf);
      output += hex_digit((u >> 4) & 0xf);
      output += hex_digit(u & 0xf);
      continue;
    }
    qsizetype escape = c_escape_char(u);
    if (escape < 0) {
      output += '\\';
      output += 'x';
      output += hex_digit(u >> 4);
      output += hex_digit(u & 0xf);
      continue;
    }
    if (escape > 0) {
      output += '\\';
      output += (char)escape;
      continue;
    }
    output += (char)u;
  }
  return output;
}

Utf8String Utf8String::fromCEscaped(const char *s, qsizetype len) {
  if (!s)
    [[unlikely]] return {};
  if (len < 0)
    len = ::strlen(s);
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
            result += u < 0x80 ? u : (char32_t)ReplacementCharacter;
          s += taken-1;
          break;
        case 'x':
          // LATER handle x{} C++23
          if ((taken = decode_hex_char(++s, end, 0, &u)))
            result += u < 0x80 ? u : (char32_t)ReplacementCharacter;
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
          // "\\" is \, "\?" is ?, "\'" is ' and '\"' is " are standard
          // any other sequence, such as "\;", is implementation defined, we
          // decided to handle this way too: "\;" is ; and "\Z" is Z
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

inline std::pair<qreal,qreal> utf8_to_coords_2(const Utf8String &xy) {
  auto coords = xy.split(',');
  bool ok;
  auto x = coords.value(0).toNumber<qreal>(&ok);
  if (!ok)
    return {};
  auto y = coords.value(1).toNumber<qreal>(&ok);
  if (!ok)
    return {};
  return {x, y};
}

inline std::tuple<qreal,qreal,qreal,qreal> utf8_to_coords_4(
    const Utf8String &xyza) {
  auto coords = xyza.split(',');
  bool ok;
  auto x = coords.value(0).toNumber<qreal>(&ok);
  if (!ok)
    return {};
  auto y = coords.value(1).toNumber<qreal>(&ok);
  if (!ok)
    return {};
  auto z = coords.value(2).toNumber<qreal>(&ok);
  if (!ok)
    return {};
  auto a = coords.value(3).toNumber<qreal>(&ok);
  if (!ok)
    return {};
  return {x, y, z, a};
}

QPointF Utf8String::toPointF() const {
  auto [x,y] = utf8_to_coords_2(*this);
  return QPointF(x, y);
}

QSizeF Utf8String::toSizeF() const {
  auto [x,y] = utf8_to_coords_2(*this);
  return QSizeF(x, y);
}

QRectF Utf8String::toRectF() const {
  auto [x,y,w,h] = utf8_to_coords_4(*this);
  return QRectF(x, y, w, h);
}

QLineF Utf8String::toLineF() const {
  auto [x1,y1,x2,y2] = utf8_to_coords_4(*this);
  return QLineF(x1, y1, x2, y2);
}

QList<QPointF> Utf8String::toPointFList() const {
  QList<QPointF> points;
  for (const auto &coords: split(' '))
    points += coords.toPointF();
  return points;
}

Utf8String Utf8String::number_and_name(const QObject *p) {
  if (!p)
    return "QObject(0x0)";
  return "QObject(0x"+number(p, 16)+", \""+p->objectName()+"\")";
}
