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
  Utf8String(const QByteArray &ba = {}) : QByteArray(ba) {}
  Utf8String(const QByteArray &&ba) : QByteArray(ba) {}
  Utf8String(const QString &s) : QByteArray(s.toUtf8()) {}
  Utf8String(const Utf8String &other) : QByteArray(other) {}
  Utf8String(const Utf8String &&other) : QByteArray(other) {}
  Utf8String(const QLatin1StringView v) : QByteArray(v.toString().toUtf8()) {}
  Utf8String(const QByteArrayView v) : QByteArray(v.toByteArray()) {}
  Utf8String(const QAnyStringView v) : QByteArray(v.toString().toUtf8()) {}
  Utf8String(const QUtf8StringView v) : QByteArray(v.toString().toUtf8()) {}
  Utf8String(const QStringView v) : QByteArray(v.toUtf8()) {}
  Utf8String(const char *s, qsizetype size = -1) : QByteArray(s, size) { }
  //template <typename Char,if_compatible_char<Char>>
  //Utf8String(const Char *s, qsizetype size = -1) : QByteArray(s, size) { }
  /** convert arithmetic types using QByteArray::number() */
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  explicit Utf8String(T o) : QByteArray(QByteArray::number(o)) {}
  /** convert bool to "true" or "false" */
  explicit Utf8String(bool o) : QByteArray(o ? "true"_ba : "false"_ba) {}
  /** take QByteArray if v.canConvert<QByteArray>() (assuming UTF-8) otherwise
   * take QString and convert to UTF-8 */
  explicit Utf8String(QVariant v)
    : QByteArray(v.canConvert<QByteArray>() ? v.toByteArray()
                                            : v.toString().toUtf8()) {}
  Utf8String &operator =(const Utf8String &other) {
    QByteArray::operator =(other); return *this;
  }
  Utf8String &operator =(const Utf8String &&other) {
    QByteArray::operator =(other); return *this;
  }

  [[nodiscard]] char value(qsizetype i, char defaultValue = '\0') const {
    return size() < i+1 ? defaultValue : at(0);
  }
  [[nodiscard]] char operator[](qsizetype i) const { return value(i, '\0'); }

  operator char *() const = delete;
  operator void *() const = delete;
  //QString operator QString() const { return QString::fromUtf8(*this); }
  [[nodiscard]] QString toString() const { return QString::fromUtf8(*this); }
  [[nodiscard]] inline AsciiString toAscii() const;// { return *this; }
  QByteArray toLower() = delete;
  QByteArray toUpper() = delete;
  QByteArray isLower() = delete;
  QByteArray isUpper() = delete;
};

Q_DECLARE_METATYPE(Utf8String)

class LIBP6CORESHARED_EXPORT AsciiString : public Utf8String {
public:
  AsciiString(Utf8String s) : Utf8String(stripNonAsciiChars(s)) { }
  [[nodiscard]] QByteArray stripNonAsciiChars(const QByteArray &source) {
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

AsciiString Utf8String::toAscii() const {
  return AsciiString(*this);
}

#endif // UTF8STRING_H
