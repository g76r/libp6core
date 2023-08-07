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

#ifndef UTF8STRING_H
#define UTF8STRING_H

#include <QVariant>
#include <QList>
#include <QSet>
#include "libp6core_global.h"

using namespace Qt::Literals::StringLiterals;

class AsciiString;
class Utf8StringList;

/** Enhanced QByteArray with string methods, always assuming 8 bits content is a
 * UTF-8 encoded string (QByteArray, char *, etc.). */
class LIBP6CORESHARED_EXPORT Utf8String : public QByteArray {
public:
  const static QList<char> Whitespace;
  inline Utf8String(const QByteArray &ba = {}) : QByteArray(ba) {}
  inline Utf8String(const QByteArray &&ba) : QByteArray(ba) {}
  inline Utf8String(const QByteArrayData ba) : QByteArray(ba) {}
  inline Utf8String(const QString &s) : QByteArray(s.toUtf8()) {}
  inline Utf8String(const Utf8String &other) : QByteArray(other) {}
  inline Utf8String(const Utf8String &&other) : QByteArray(other) {}
  inline Utf8String(const QLatin1StringView v)
    : QByteArray(v.toString().toUtf8()) {}
  inline Utf8String(const QByteArrayView v) : QByteArray(v.toByteArray()) {}
  inline Utf8String(const QAnyStringView v)
    : QByteArray(v.toString().toUtf8()) {}
  inline Utf8String(const QUtf8StringView v)
    : QByteArray(v.toString().toUtf8()) {}
  inline Utf8String(const QStringView v) : QByteArray(v.toUtf8()) {}
  inline Utf8String(const char *s, qsizetype size = -1)
    : QByteArray(s, size) { }
  inline ~Utf8String() {}
  //template <typename Char,if_compatible_char<Char>>
  //Utf8String(const Char *s, qsizetype size = -1) : QByteArray(s, size) { }
  /** convert arithmetic types using QByteArray::number() */
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  explicit inline Utf8String(T o) : QByteArray(QByteArray::number(o)) {}
  /** convert bool to "true" or "false" */
  explicit inline Utf8String(bool o) : QByteArray(o ? "true"_ba : "false"_ba) {}
  /** take QByteArray if v.canConvert<QByteArray>() (assuming UTF-8) otherwise
   * take QString and convert to UTF-8 */
  explicit inline Utf8String(QVariant v)
    : QByteArray(v.isValid()
                 ? (v.canConvert<QByteArray>() ? v.toByteArray()
                                               : v.toString().toUtf8())
                 : QByteArray{}) {}
  inline Utf8String &operator =(const Utf8String &other) {
    QByteArray::operator =(other); return *this;
  }
  inline Utf8String &operator =(const Utf8String &&other) {
    QByteArray::operator =(other); return *this;
  }
  [[nodiscard]] inline char value(qsizetype i, char defaultValue = '\0') const {
    return size() < i+1 ? defaultValue : at(0);
  }
  [[nodiscard]] inline char operator[](qsizetype i) const {
    return value(i, '\0');
  }
  [[nodiscard]] inline QString toString() const {
    return QString::fromUtf8(*this);
  }
  [[nodiscard]] inline AsciiString toAscii() const;
  operator char *() const = delete;
  operator void *() const = delete;
  QByteArray toLower() = delete;
  QByteArray toUpper() = delete;
  QByteArray isLower() = delete;
  QByteArray isUpper() = delete;
  Utf8String left(qsizetype len) const { return QByteArray::left(len); }
  Utf8String right(qsizetype len) const { return QByteArray::right(len); }
  Utf8String mid(qsizetype pos, qsizetype len = -1) const {
    return QByteArray::mid(pos, len); }
  Utf8String utf8Left(qsizetype len) const {
    // LATER optimize and support all unicode (not just 16 bits)
    return toString().left(len); }
  Utf8String utf8Right(qsizetype len) const {
    // LATER optimize and support all unicode (not just 16 bits)
    return toString().right(len); }
  Utf8String utf8Mid(qsizetype pos, qsizetype len = -1) const {
    // LATER optimize and support all unicode (not just 16 bits)
    return toString().mid(pos, len); }
  const Utf8StringList split(QList<char> seps, qsizetype offset = 0,
      Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  inline const Utf8StringList split(
      QList<char> seps, Qt::SplitBehavior behavior) const;
  inline const Utf8StringList split(
      const char sep, const qsizetype offset = 0,
      Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  inline const Utf8StringList split(
      const char sep, Qt::SplitBehavior behavior) const;
  /** Split the string using its first char as a delimiter, at byte level.
   *  e.g. "/foo/bar/g" -> { "foo", "bar", "g" }
   *  e.g. ",/,:,g" -> { "/", ":", "g" }
   *  e.g. "§foo§bar§g" unsupported (multibyte delimiter)
   */
  const Utf8StringList splitByLeadingChar(qsizetype offset = 0) const;
  /** Split the string using its first char as a delimiter, at character level.
   *  e.g. "/foo/bar/g" -> { "foo", "bar", "g" }
   *  e.g. ",/,:,g" -> { "/", ":", "g" }
   *  e.g. "§foo§bar§g" -> { "foo", "bar", "g" }
   */
  const Utf8StringList utf8SplitByLeadingChar(qsizetype offset = 0) const;
  /** Converts to floating point, supporting e notation and SI suffixes from 'f'
   *  to 'P', 'u' is used as 1e-6 suffix. */
  double toDouble(bool *ok = nullptr, double def = 0.0,
                  bool suffixes_enabled = true) const;
  /** Converts to floating point, supporting e notation and SI suffixes from 'f'
   *  to 'P', 'u' is used as 1e-6 suffix. */
  float toFloat(bool *ok = nullptr, float def = 0.0,
                bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  qlonglong toLongLong(bool *ok = nullptr, int base = 0, qlonglong def = 0,
                       bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  qulonglong toULongLong(bool *ok = nullptr, int base = 0, qulonglong def = 0,
                         bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  long toLong(bool *ok = nullptr, int base = 0, long def = 0,
              bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  ulong toULong(bool *ok = nullptr, int base = 0, ulong def = 0,
                bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  int toInt(bool *ok = nullptr, int base = 0, int def = 0,
            bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  uint toUInt(bool *ok = nullptr, int base = 0, uint def = 0,
              bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  short toShort(bool *ok = nullptr, int base = 0, short def = 0,
                bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  ushort toUShort(bool *ok = nullptr, int base = 0, ushort def = 0,
                  bool suffixes_enabled = true) const;
  /** Converts to bool, supporting case insensitive "true" and "false", and any
   *  number, 0 being false and everything else true. */
  bool toBool(bool *ok = nullptr, bool def = false) const;
};

Q_DECLARE_METATYPE(Utf8String)

inline Utf8String operator"" _u8(const char *str, size_t size) noexcept {
  return Utf8String(QByteArrayData(nullptr, const_cast<char *>(str), qsizetype(size)));
}

#if __cplusplus >= 202002L
inline Utf8String operator"" _u8(const char8_t *str, size_t size) noexcept {
  return Utf8String(QByteArrayData(nullptr, (char *)(str), qsizetype(size)));
}
#endif

class Utf8StringSet;

class LIBP6CORESHARED_EXPORT Utf8StringList : public QList<Utf8String> {
public:
  Utf8StringList() { }
  Utf8StringList(std::initializer_list<Utf8String> args)
    : QList<Utf8String>(args) { }
  // explicit Utf8StringList(const Utf8String &item) : QList<Utf8String>({item}) { }
  Utf8StringList(const QList<Utf8String> &list) : QList<Utf8String>(list) { }
  Utf8StringList(const QList<QByteArray> &list)
    : QList<Utf8String>(list.constBegin(), list.constEnd()) { }
  Utf8StringList(const QList<QString> &list)
    : QList<Utf8String>(list.constBegin(), list.constEnd()) { }
  Utf8StringList(const QSet<Utf8String> &set)
    : QList<Utf8String>(set.constBegin(), set.constEnd()) { }
  Utf8StringList(const QSet<QByteArray> &set)
    : QList<Utf8String>(set.constBegin(), set.constEnd()) { }
  Utf8StringList(const QSet<QString> &set)
    : QList<Utf8String>(set.constBegin(), set.constEnd()) { }
  Utf8String join(const Utf8String &separator) const;
  Utf8String join(const char separator) const;
  QStringList toStringList() const {
    return QStringList(constBegin(), constEnd()); }
  QByteArrayList toByteArrayList() const {
    return QByteArrayList(constBegin(), constEnd()); }
  inline Utf8StringSet toSet() const;
  inline Utf8StringList toSortedDeduplicated() const;
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
    : QSet<Utf8String>(set.constBegin(), set.constEnd()) { }
  Utf8StringSet(const QSet<QString> &set)
    : QSet<Utf8String>(set.constBegin(), set.constEnd()) { }
  Utf8StringSet(const QList<Utf8String> &set)
    : QSet<Utf8String>(set.constBegin(), set.constEnd()) { }
  Utf8StringSet(const QList<QByteArray> &set)
    : QSet<Utf8String>(set.constBegin(), set.constEnd()) { }
  Utf8StringSet(const QList<QString> &set)
    : QSet<Utf8String>(set.constBegin(), set.constEnd()) { }
  Utf8String join(const Utf8String &separator) const;
  Utf8String join(const char separator) const;
  Utf8String sortedJoin(const Utf8String &separator) {
    return toSortedList().join(separator); }
  Utf8String sortedJoin(const char separator) {
    return toSortedList().join(separator); }
  Utf8StringList toList() const { return Utf8StringList(*this); }
  Utf8StringList toSortedList() const {
    auto list = toList(); std::sort(list.begin(), list.end()); return list; }
};

Q_DECLARE_METATYPE(Utf8StringSet)

const Utf8StringList Utf8String::split(
    QList<char> seps, Qt::SplitBehavior behavior) const {
  return split(seps, 0, behavior);
}

const Utf8StringList Utf8String::split(
    const char sep, const qsizetype offset, Qt::SplitBehavior behavior) const {
  return split({sep}, offset, behavior);
}

const Utf8StringList Utf8String::split(
    const char sep, Qt::SplitBehavior behavior) const {
  return split(sep, 0, behavior);
}

Utf8StringSet Utf8StringList::toSet() const {
  return Utf8StringSet(*this);
}

Utf8StringList Utf8StringList::toSortedDeduplicated() const {
  return toSet().toSortedList();
}

class LIBP6CORESHARED_EXPORT AsciiString : public Utf8String {
public:
  //struct AsciiLiteral {
  //  const char *str;
  //  size_t size;
  //};
  friend AsciiString operator"" _a7(const char *str, size_t size) noexcept;
  //inline AsciiString(QByteArrayData ba) : Utf8String(ba) {}
public:
  AsciiString(Utf8String s) : Utf8String(stripNonAsciiChars(s)) { }
  //AsciiString(AsciiLiteral l)
  //  : Utf8String(QByteArrayData(nullptr, const_cast<char *>(l.str),
  //                              qsizetype(l.size))) { }
  [[nodiscard]] static QByteArray stripNonAsciiChars(const QByteArray &source) {
    auto size = source.size();
    char ascii[size];
    int j = 0;
    for (int i = 0; i < size; ++i)
      if (isascii(source[i]))
        ascii[j++] = source[i];
    return QByteArray(ascii, j);
  }
  using QByteArray::toLower;
  using QByteArray::toUpper;
  using QByteArray::isLower;
  using QByteArray::isUpper;
};

Q_DECLARE_METATYPE(AsciiString)

/*inline constexpr AsciiString::AsciiLiteral operator"" _a7(
    const char * const str, size_t size) noexcept {
  for (size_t i = 0; i < size; ++i)
    static_assert(!(str[i] & 0x80), "operator \"\"_a7 disallows 8 bit chars");
  return {str, size};
}*/

/*inline constexpr size_t ensure_ascii7(const char *str, size_t size) {
  for (size_t i = 0; i < size; ++i)
    if (!!(str[i] & 0x80))
      throw;
  return size;
}*/

inline AsciiString operator"" _a7(const char *str, size_t size) noexcept {
  // TODO ensure that no 8 bit data is including, in a compile-time way
  //for (size_t i = 0; i < size; ++i)
  //  if (!!(str[i] & 0x80))
  //    throw;
  //auto dummy = ensure_ascii7(str, size);
  return AsciiString(QByteArrayData(nullptr, const_cast<char *>(str), qsizetype(size)));
}

//static auto a = "foo"_a7;
//static auto b = "é"_a7;
//static auto c = "ésdflkj"_u8;

AsciiString Utf8String::toAscii() const {
  return AsciiString(*this);
}

#endif // UTF8STRING_H
