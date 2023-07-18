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
#include "libp6core_global.h"

using namespace Qt::Literals::StringLiterals;

class AsciiString;
class Utf8String;

/** Enhanced QByteArray with string methods, always assuming 8 bits content is a
 * UTF-8 encoded string (QByteArray, char *, etc.). */
class LIBP6CORESHARED_EXPORT Utf8String : public QByteArray {
public:
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
    : QByteArray(v.canConvert<QByteArray>() ? v.toByteArray()
                                            : v.toString().toUtf8()) {}
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
