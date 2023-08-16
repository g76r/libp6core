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

#include "libp6core_global.h"
#include <QVariant>
#include <bit>

using namespace Qt::Literals::StringLiterals;

class Utf8StringList;
class Utf8StringSet;

/** Enhanced QByteArray with string methods, always assuming 8 bits content is a
 * UTF-8 encoded string (QByteArray, char *, etc.). */
class LIBP6CORESHARED_EXPORT Utf8String : public QByteArray {
public:
  const static QList<char> AsciiWhitespace;
  const static char32_t ReplacementCharacter = U'\ufffd';
  const static Utf8String ReplacementCharacterUtf8;
  const static Utf8String Empty;

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
  inline Utf8String(const unsigned char *s, qsizetype size = -1)
    : QByteArray((const char *)s, size) { }
  inline Utf8String(const signed char *s, qsizetype size = -1)
    : QByteArray((const char *)s, size) { }
  inline Utf8String(const char *s, qsizetype size = -1)
    : QByteArray(s, size) { }
  explicit inline Utf8String(const char c) : QByteArray(&c, 1) { }
  explicit inline Utf8String(char32_t c) : QByteArray(encodeUtf8(c)) { }
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
    : QByteArray(v.canConvert<QByteArray>()
                 ? v.toByteArray()
                 : v.canConvert<Utf8String>()
                   ? v.value<Utf8String>()
                   : v.canConvert<QString>()
                     ? v.toString().toUtf8()
                     : QByteArray{}) { }

  /** Return ith byte of the string, like operator[] or at() but safe if i
   *  is out of range. */
  [[nodiscard]] inline char value(qsizetype i, char def = 0) const {
    return size() < i+1 || i < 0 ? def : at(i); }
  /** Return ith utf8 character (bytes sequence) of the string. Safe if i
   *  is out of range. */
  [[nodiscard]] static Utf8String utf8Value(
      qsizetype i, const char *s, qsizetype len,
      const Utf8String &def = Empty);
  [[nodiscard]] inline static Utf8String utf8Value(
      qsizetype i, const char *s, const char *end,
      const Utf8String &def = Empty) {
    return utf8Value(i, s, end-s, def); }
  [[nodiscard]] inline Utf8String utf8Value(
      qsizetype i, const Utf8String &def = Empty) const {
    return utf8Value(i, constData(), size(), def); }
  /** Return ith unicode character (code point value) of the string. Safe if i
   *  is out of range. */
  [[nodiscard]] static char32_t utf32Value(
      qsizetype i, const char *s, qsizetype len, const char32_t def = 0);
  [[nodiscard]] inline char32_t utf32Value(
      qsizetype i, char32_t def = 0) const {
    return utf32Value(i, constData(), size(), def); }

  [[nodiscard]] inline QString toString() const {
    return QString::fromUtf8(*this); }

  [[nodiscard]] static inline Utf8String encodeUtf8(char32_t c);
  [[nodiscard]] static inline char32_t decodeUtf8(
      const char *s, qsizetype len);
  [[nodiscard]] static inline char32_t decodeUtf8(
      const char *s, const char *end) {
    return decodeUtf8(s, end-s); }
  [[nodiscard]] static inline char32_t decodeUtf8(const QByteArray &s) {
    return decodeUtf8(s.constData(), s.size()); }
  [[nodiscard]] static inline char32_t decodeUtf8(const char *s) {
    return s ? decodeUtf8(s, ::strlen(s)) : 0; }

  /** Convert a unicode charater into its uppercase character, e.g. é -> É.
   *  Return the input character itself if no change is needed e.g. E É #
   */
  static inline char32_t toUpper(char32_t c);
  /** Convert a unicode charater into its uppercase character, e.g. É -> é .
   *  Return the input character itself if no change is needed e.g. E é #
   */
  static inline char32_t toLower(char32_t c);
  /** Set the characters to title case.
   *  For most letters, title case is the same than upper case, but for some
   *  rare characters representing several letters at once, there is a title case
   *  for which the first letter is in upper case and others in lower cases.
   *  For instance ǆ minuscule (unicode: 0x1C6) is mapped to Ǆ majuscule
   *  (unicode: 0x1C4) and to ǅ title case letter (unicode: 0x1C5)
   *  Return the input character itself if no change is needed e.g. E É #
   */
  static inline char32_t toTitle(char32_t c);
  [[nodiscard]] Utf8String toUpper() const;
  [[nodiscard]] Utf8String toLower() const;
  [[nodiscard]] Utf8String toTitle();
  [[nodiscard]] bool isLower() const;
  [[nodiscard]] bool isUpper() const;
  [[nodiscard]] bool isTitle() const;
  // FIXME inline int compare(QByteArrayView a, Qt::CaseSensitivity cs) const noexcept;

  /** Return utf8 characters count. Count utf8 sequences without ensuring
    * their validity, so with invalid utf8 data this may overestimates. */
  [[nodiscard]] qsizetype utf8Size() const;
  [[nodiscard]] Utf8String trimmed() const { return QByteArray::trimmed(); }
  Utf8String &trim() { *this = trimmed(); return *this; }
  inline Utf8String &fill(char c, qsizetype size = -1) {
    QByteArray::fill(c, size); return *this; }
  /** Return valid utf8 without invalid sequences (or having them replaced by
   *  so called replacement character), without BOMs and without overlong
   *  encoding. */
  [[nodiscard]] inline Utf8String cleaned() const {
    return cleaned(constData(), size()); }
  [[nodiscard]] inline static Utf8String cleaned(const char *s, qsizetype len) {
    return cleaned(s, s+len); }
  [[nodiscard]] static Utf8String cleaned(const char *s, const char *end);
  Utf8String &clean() { return *this = cleaned(); }

  /** Return leftmost len bytes. Empty if len < 0. */
  [[nodiscard]] Utf8String left(qsizetype len) const {
    return QByteArray::left(len); }
  /** Return rightmost len bytes. Empty if len < 0. */
  [[nodiscard]] Utf8String right(qsizetype len) const {
    return QByteArray::right(len); }
  /** Return len bytes starting at pos.
   *  Everything after pos if len < 0 or pos+len > size(). */
  [[nodiscard]] Utf8String mid(qsizetype pos, qsizetype len = -1) const {
    return QByteArray::mid(pos, len); }
  /** Return leftmost len utf8 characters. */
  [[nodiscard]] Utf8String utf8Left(qsizetype len) const;
  /** Return rightmost len utf8 characters. */
  [[nodiscard]] Utf8String utf8Right(qsizetype len) const;
  /** Return len utf8 characters starting at pos.
   *  Everything after pos if len < 0 or pos+len > size(). */
  [[nodiscard]] Utf8String utf8Mid(qsizetype pos, qsizetype len = -1) const;

  /** Splitting utf8 string on ascii 7 separators, e.g. {',',';'}
    * @see Utf8String::AsciiWhitespace */
  [[nodiscard]] const Utf8StringList split(
      QList<char> seps, qsizetype offset = 0,
      Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Splitting utf8 string on ascii 7 separators, e.g. {',',';'}
    * @see Utf8String::AsciiWhitespace */
  [[nodiscard]] const Utf8StringList split(
      QList<char> seps, Qt::SplitBehavior behavior) const;
  /** Splitting utf8 string on ascii 7 separator, e.g. ' ' */
  [[nodiscard]] const Utf8StringList split(
      const char sep, const qsizetype offset = 0,
      Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Splitting utf8 string on ascii 7 separator, e.g. ' ' */
  [[nodiscard]] const Utf8StringList split(
      const char sep, Qt::SplitBehavior behavior) const;
  /** Splitting utf8 string on multi-byte (utf8) or multi-char separator,
   *  e.g. "-->", "💩"_u8, U'💩', "<≠>"_u8 */
  [[nodiscard]] const Utf8StringList split(Utf8String sep, qsizetype offset = 0,
      Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Split the string using its first utf8 char as a delimiter.
   *  e.g. "/foo/bar/g" -> { "foo", "bar", "g" }
   *  e.g. ",/,:,g" -> { "/", ":", "g" }
   *  e.g. "§foo§bar§g" -> { "foo", "bar", "g" }
   *  e.g. "越foo越bar越g" -> { "foo", "bar", "g" }
   *  e.g. "💩foo💩bar💩g" -> { "foo", "bar", "g" }
   */
  [[nodiscard]] const Utf8StringList splitByLeadingChar(
      qsizetype offset = 0) const;

  /** Converts to floating point, supporting e notation and SI suffixes from 'f'
   *  to 'P', 'u' is used as 1e-6 suffix. */
  [[nodiscard]] double toDouble(bool *ok = nullptr, double def = 0.0,
                                bool suffixes_enabled = true) const;
  /** Converts to floating point, supporting e notation and SI suffixes from 'f'
   *  to 'P', 'u' is used as 1e-6 suffix. */
  [[nodiscard]] float toFloat(bool *ok = nullptr, float def = 0.0,
                              bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  [[nodiscard]] qlonglong toLongLong(
      bool *ok = nullptr, int base = 0, qlonglong def = 0,
      bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  [[nodiscard]] qulonglong toULongLong(
      bool *ok = nullptr, int base = 0, qulonglong def = 0,
      bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  [[nodiscard]] long toLong(bool *ok = nullptr, int base = 0, long def = 0,
                            bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  [[nodiscard]] ulong toULong(bool *ok = nullptr, int base = 0, ulong def = 0,
                              bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  [[nodiscard]] int toInt(bool *ok = nullptr, int base = 0, int def = 0,
                          bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  [[nodiscard]] uint toUInt(bool *ok = nullptr, int base = 0, uint def = 0,
                            bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  [[nodiscard]] short toShort(bool *ok = nullptr, int base = 0, short def = 0,
                              bool suffixes_enabled = true) const;
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  common casula suffixes ('k', 'm', 'b'). Defaults to base 0 where
   *  C prefixes are supported "0x" "0" and "0b". */
  [[nodiscard]] ushort toUShort(
      bool *ok = nullptr, int base = 0, ushort def = 0,
      bool suffixes_enabled = true) const;
  /** Converts to bool, supporting case insensitive "true" and "false", and any
   *  number, 0 being false and everything else true. */
  [[nodiscard]] bool toBool(bool *ok = nullptr, bool def = false) const;
  [[nodiscard]] inline Utf8String toBase64(
      Base64Options options = Base64Encoding) const {
    return QByteArray::toBase64(options); }
  [[nodiscard]] inline Utf8String toHex(char separator = '\0') const {
    return QByteArray::toHex(separator); }
  [[nodiscard]] inline Utf8String toPercentEncoding(
      const QByteArray &exclude = {}, const QByteArray &include = {},
      char percent = '%') const {
    return QByteArray::toPercentEncoding(exclude, include, percent); }
  [[nodiscard]] inline Utf8String percentDecoded(char percent = '%') const {
    return QByteArray::percentDecoded(percent); }

  [[nodiscard]] static inline Utf8String number(int i, int base = 10) {
    return QByteArray::number(i, base); }
  [[nodiscard]] static inline Utf8String number(uint i, int base = 10) {
    return QByteArray::number(i, base); }
  [[nodiscard]] static inline Utf8String number(long i, int base = 10) {
    return QByteArray::number(i, base); }
  [[nodiscard]] static inline Utf8String number(ulong i, int base = 10) {
    return QByteArray::number(i, base); }
  [[nodiscard]] static inline Utf8String number(qlonglong i, int base = 10) {
    return QByteArray::number(i, base); }
  [[nodiscard]] static inline Utf8String number(qulonglong i, int base = 10) {
    return QByteArray::number(i, base); }
  [[nodiscard]] static inline Utf8String number(
      double d, char format = 'g', int precision = 6) {
    return QByteArray::number(d, format, precision); }

  [[nodiscard]] inline Utf8String first(qsizetype n) const {
    return QByteArray::first(n); }
  [[nodiscard]] inline Utf8String last(qsizetype n) const {
    return QByteArray::last(n); }
  [[nodiscard]] inline Utf8String sliced(qsizetype pos) const {
    return QByteArray::sliced(pos); }
  [[nodiscard]] inline Utf8String sliced(qsizetype pos, qsizetype n) const {
    return QByteArray::sliced(pos, n); }
  [[nodiscard]] inline Utf8String chopped(qsizetype len) const {
    return QByteArray::chopped(len); }

  inline Utf8String &replace(
      qsizetype index, qsizetype len, const char *s, qsizetype alen)
  { return replace(index, len, QByteArrayView(s, alen)); }
  inline Utf8String &replace(
      qsizetype index, qsizetype len, const Utf8String &s) {
    QByteArray::replace(index, len, s); return *this; }
  inline Utf8String &replace(char before, const Utf8String &after) {
    QByteArray::replace(before, after); return *this; }
  inline Utf8String &replace(
      const char *before, qsizetype bsize, const char *after, qsizetype asize) {
    QByteArray::replace(QByteArrayView(before, bsize),
                        QByteArrayView(after, asize)); return *this; }
  inline Utf8String &replace(
      const Utf8String &before, const Utf8String &after) {
    QByteArray::replace(before, after); return *this; }
  inline Utf8String &replace(char before, char after) {
    QByteArray::replace(before, after); return *this; }

  inline Utf8String &append(char c) {
    QByteArray::append(c); return *this; }
  inline Utf8String &append(qsizetype count, char c) {
    QByteArray::append(count, c); return *this; }
  inline Utf8String &append(const char *s) {
    QByteArray::append(s); return *this; }
  inline Utf8String &append(const char *s, qsizetype len) {
    QByteArray::append(s, len); return *this; }
  inline Utf8String &append(const QByteArray &a) {
    QByteArray::append(a); return *this; }
  inline Utf8String &append(QByteArrayView a) {
    QByteArray::append(a); return *this; }
  inline Utf8String &append(char32_t c) {
    QByteArray::append(encodeUtf8(c)); return *this; }

  inline Utf8String &prepend(char c) {
    QByteArray::prepend(c); return *this; }
  inline Utf8String &prepend(qsizetype count, char c) {
    QByteArray::prepend(count, c); return *this; }
  inline Utf8String &prepend(const char *s) {
    QByteArray::prepend(s); return *this; }
  inline Utf8String &prepend(const char *s, qsizetype len) {
    QByteArray::prepend(s, len); return *this; }
  inline Utf8String &prepend(const QByteArray &a) {
    QByteArray::prepend(a); return *this; }
  inline Utf8String &prepend(QByteArrayView a) {
    QByteArray::prepend(a); return *this; }
  inline Utf8String &prepend(char32_t c) {
    QByteArray::prepend(encodeUtf8(c)); return *this; }

  inline Utf8String &insert(qsizetype i, QByteArrayView data) {
    QByteArray::insert(i, data); return *this; }
  inline Utf8String &insert(qsizetype i, const char *s) {
    QByteArray::insert(i, s); return *this; }
  inline Utf8String &insert(qsizetype i, const QByteArray &data) {
    QByteArray::insert(i, data); return *this; }
  inline Utf8String &insert(qsizetype i, qsizetype count, char c) {
    QByteArray::insert(i, count, c); return *this; }
  inline Utf8String &insert(qsizetype i, char c) {
    QByteArray::insert(i, c); return *this; }
  inline Utf8String &insert(qsizetype i, const char *s, qsizetype len) {
    QByteArray::insert(i, s, len); return *this; }

  inline Utf8String &remove(qsizetype index, qsizetype len) {
    QByteArray::remove(index, len); return *this;}
  inline Utf8String &removeAt(qsizetype pos) {
    QByteArray::removeAt(pos); return *this;}
  inline Utf8String &removeFirst() { QByteArray::removeFirst(); return *this;}
  inline Utf8String &removeLast() { QByteArray::removeLast(); return *this;}

  inline Utf8String &operator+=(const Utf8String &s) {
    QByteArray::operator+=(s); return *this; }
  inline Utf8String &operator+=(const QByteArray &ba) {
    QByteArray::operator+=(ba); return *this; }
  inline Utf8String &operator+=(char ch) {
    QByteArray::operator+=(ch); return *this; }
  inline Utf8String &operator+=(const char *s) {
    QByteArray::operator+=(s); return *this; }
  inline Utf8String &operator+=(char32_t c) {
    QByteArray::operator+=(encodeUtf8(c)); return *this; }

  inline Utf8String &operator=(const Utf8String &other) {
    QByteArray::operator =(other); return *this; }
  inline Utf8String &operator=(const Utf8String &&other) {
    QByteArray::operator =(other); return *this; }
  inline Utf8String &operator=(const QByteArray &ba) {
    QByteArray::operator=(ba); return *this; }
  inline Utf8String &operator=(const QString &s) {
    return operator=(s.toUtf8()); }
//  inline Utf8String &operator=(char ch) {
//    QByteArray::operator=(ch); return *this; }
  inline Utf8String &operator=(const char *s) {
    QByteArray::operator=(s); return *this; }

#if __cpp_impl_three_way_comparison >= 201711
  [[nodiscard]] static inline std::strong_ordering cmp(
      const char *x, qsizetype lenx, const char *y, qsizetype leny) {
    auto n = std::min(lenx, leny);
    for (int i = 0; i < n; ++i)
      if (auto cmp = x[i] <=> y[i]; cmp != 0)
        return cmp;
    return lenx <=> leny;
  }

  [[nodiscard]] inline int compare(const Utf8String &that,
              Qt::CaseSensitivity cs = Qt::CaseSensitive) const {
    auto c = cs == Qt::CaseSensitive
             ? cmp(this->constData(), size(), that.constData(), that.size())
             : toUpper() <=> that.toUpper();
    if (c == std::strong_ordering::less)
      return -1;
    if (c == std::strong_ordering::greater)
      return 1;
    return 0;
  }

  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const Utf8String &x, const Utf8String &y) {
    return Utf8String::cmp(x.constData(), x.size(), y.constData(), y.size()); }
  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const Utf8String &x, const QByteArray &y) {
    return Utf8String::cmp(x.constData(), x.size(), y.constData(), y.size()); }
  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const QByteArray &x, const Utf8String &y) {
    return Utf8String::cmp(x.constData(), x.size(), y.constData(), y.size()); }
  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const Utf8String &x, const char *y) {
    return Utf8String::cmp(x.constData(), x.size(), y, ::strlen(y)); }
  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const char *x, const Utf8String &y) {
    return Utf8String::cmp(x, ::strlen(x), y.constData(), y.size()); }

  [[nodiscard]] friend inline bool operator==(
      const Utf8String &x, const Utf8String &y) {
    return Utf8String::cmp(x.constData(), x.size(), y.constData(), y.size()) == 0; }
  [[nodiscard]] friend inline bool operator==(
      const Utf8String &x, const QByteArray &y) {
    return Utf8String::cmp(x.constData(), x.size(), y.constData(), y.size()) == 0; }
  [[nodiscard]] friend inline bool operator==(
      const QByteArray &x, const Utf8String &y) {
    return Utf8String::cmp(x.constData(), x.size(), y.constData(), y.size()) == 0; }
  [[nodiscard]] friend inline bool operator==(
      const Utf8String &x, const char *y) {
    return Utf8String::cmp(x.constData(), x.size(), y, ::strlen(y)) == 0; }
  [[nodiscard]] friend inline bool operator==(
      const char *x, const Utf8String &y) {
    return Utf8String::cmp(x, ::strlen(x), y.constData(), y.size()) == 0; }
#else
  [[nodiscard]] static inline int cmp(
      const char *x, qsizetype lenx, const char *y, qsizetype leny) {
    auto n = std::min(lenx, leny);
    for (int i = 0; i < n; ++i)
      if (auto cmp = x[i]-y[i]; cmp != 0)
        return cmp < 0 ? -1 : 0;
    return lenx-leny < 0 ? -1 : 0;
  }

  [[nodiscard]] inline int compare(const Utf8String &that,
              Qt::CaseSensitivity cs = Qt::CaseSensitive) const {
    return cs == Qt::CaseSensitive
        ? cmp(this->constData(), size(), that.constData(), that.size())
        : toUpper().compare(that.toUpper());
  }

  using QByteArray::operator<;
  friend inline bool operator<(const Utf8String &x, const Utf8String &y)
  noexcept { return static_cast<const QByteArray&>(x) <
        static_cast<const QByteArray&>(y); }
  friend inline bool operator<(const Utf8String &x, const QByteArray &y)
  noexcept { return static_cast<const QByteArray&>(x) < y; }
  friend inline bool operator<(const QByteArray &x, const Utf8String &y)
  noexcept { return x < static_cast<const QByteArray&>(y); }
  friend inline bool operator<(const Utf8String &x, const char *y)
  noexcept { return static_cast<const QByteArray&>(x) < y; }
  friend inline bool operator<(const char *x, const Utf8String &y)
  noexcept { return x < static_cast<const QByteArray&>(y); }

  using QByteArray::operator<=;
  friend inline bool operator<=(const Utf8String &x, const Utf8String &y)
  noexcept { return static_cast<const QByteArray&>(x) <=
        static_cast<const QByteArray&>(y); }
  friend inline bool operator<=(const Utf8String &x, const QByteArray &y)
  noexcept { return static_cast<const QByteArray&>(x) <= y; }
  friend inline bool operator<=(const QByteArray &x, const Utf8String &y)
  noexcept { return x <= static_cast<const QByteArray&>(y); }
  friend inline bool operator<=(const Utf8String &x, const char *y)
  noexcept { return static_cast<const QByteArray&>(x) <= y; }
  friend inline bool operator<=(const char *x, const Utf8String &y)
  noexcept { return x <= static_cast<const QByteArray&>(y); }

  using QByteArray::operator==;
  friend inline bool operator==(const Utf8String &x, const Utf8String &y)
  noexcept { return static_cast<const QByteArray&>(x) ==
        static_cast<const QByteArray&>(y); }
  friend inline bool operator==(const Utf8String &x, const QByteArray &y)
  noexcept { return static_cast<const QByteArray&>(x) == y; }
  friend inline bool operator==(const QByteArray &x, const Utf8String &y)
  noexcept { return x == static_cast<const QByteArray&>(y); }
  friend inline bool operator==(const Utf8String &x, const char *y)
  noexcept { return static_cast<const QByteArray&>(x) == y; }
  friend inline bool operator==(const char *x, const Utf8String &y)
  noexcept { return x == static_cast<const QByteArray&>(y); }

  using QByteArray::operator>=;
  friend inline bool operator>=(const Utf8String &x, const Utf8String &y)
  noexcept { return static_cast<const QByteArray&>(x) >=
        static_cast<const QByteArray&>(y); }
  friend inline bool operator>=(const Utf8String &x, const QByteArray &y)
  noexcept { return static_cast<const QByteArray&>(x) >= y; }
  friend inline bool operator>=(const QByteArray &x, const Utf8String &y)
  noexcept { return x >= static_cast<const QByteArray&>(y); }
  friend inline bool operator>=(const Utf8String &x, const char *y)
  noexcept { return static_cast<const QByteArray&>(x) >= y; }
  friend inline bool operator>=(const char *x, const Utf8String &y)
  noexcept { return x >= static_cast<const QByteArray&>(y); }

  using QByteArray::operator>;
  friend inline bool operator>(const Utf8String &x, const Utf8String &y)
  noexcept { return static_cast<const QByteArray&>(x) >
        static_cast<const QByteArray&>(y); }
  friend inline bool operator>(const Utf8String &x, const QByteArray &y)
  noexcept { return static_cast<const QByteArray&>(x) > y; }
  friend inline bool operator>(const QByteArray &x, const Utf8String &y)
  noexcept { return x > static_cast<const QByteArray&>(y); }
  friend inline bool operator>(const Utf8String &x, const char *y)
  noexcept { return static_cast<const QByteArray&>(x) > y; }
  friend inline bool operator>(const char *x, const Utf8String &y)
  noexcept { return x > static_cast<const QByteArray&>(y); }
#endif // C++ 20: spaceship op

  [[nodiscard]] inline int compare(QByteArrayView bv,
              Qt::CaseSensitivity cs = Qt::CaseSensitive) const {
    return compare(Utf8String(bv), cs); }

private:
  struct UnicodeCaseMapping {
    char32_t utf32;
    char32_t upper_utf32, lower_utf32, title_utf32;
    // LATER add already encoded utf8 here as optimization
    operator char32_t() const { return utf32; }
  };
  static std::vector<UnicodeCaseMapping> _case_mapping;
};

Q_DECLARE_METATYPE(Utf8String)

inline Utf8String operator"" _u8(const char *str, size_t size) noexcept {
  return Utf8String(QByteArrayData(nullptr, const_cast<char *>(str),
                                   qsizetype(size)));
}

#if __cpp_char8_t >= 201811
inline Utf8String operator"" _u8(const char8_t *str, size_t size) noexcept {
  return Utf8String(QByteArrayData(nullptr, (char *)(str), qsizetype(size)));
}
#endif

inline Utf8String operator+(const Utf8String &a1, const Utf8String &a2) {
  return Utf8String(a1) += a2; }
inline Utf8String operator+(Utf8String &&lhs, const Utf8String &rhs) {
  return std::move(lhs += rhs); }

inline Utf8String operator+(const Utf8String &a1, const QByteArray &a2) {
  return Utf8String(a1) += a2; }
inline Utf8String operator+(Utf8String &&lhs, const QByteArray &rhs) {
  return std::move(lhs += rhs); }
inline Utf8String operator+(const QByteArray &a1, const Utf8String &a2) {
  return Utf8String(a1) += a2; }

inline Utf8String operator+(const Utf8String &a1, const QString &a2) {
  return Utf8String(a1) += Utf8String(a2); }
inline Utf8String operator+(Utf8String &&lhs, const QString &rhs) {
  return std::move(lhs += Utf8String(rhs)); }
inline Utf8String operator+(const QString &a1, const Utf8String &a2) {
  return Utf8String(a1) += a2; }

inline Utf8String operator+(const Utf8String &a1, const char *a2) {
  return Utf8String(a1) += a2; }
inline Utf8String operator+(Utf8String &&lhs, const char *rhs) {
  return std::move(lhs += rhs); }
inline Utf8String operator+(const char *a1, const Utf8String &a2) {
  return Utf8String(a1) += a2; }

//inline Utf8String operator+(const Utf8String &a1, char a2) {
//  return Utf8String(a1) += a2; }
//inline Utf8String operator+(Utf8String &&lhs, char rhs) {
//  return std::move(lhs += rhs); }
//inline Utf8String operator+(char a1, const Utf8String &a2) {
//  return Utf8String(&a1, 1) += a2; }

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const Utf8String &s);

char32_t Utf8String::decodeUtf8(const char *s, qsizetype len) {
  auto c = reinterpret_cast<const unsigned char *>(s);
  Q_ASSERT(s);
  Q_ASSERT(len > 0);
#if __cpp_lib_bitops >= 201907L // C++ < 20: bitops
  switch(std::countl_one(c[0])) {
    [[likely]] case 0:
      return *s;
    case 1:
      return ReplacementCharacter; // unexpected continuation character
    case 2:
      if (len < 2) [[unlikely]]
        return ReplacementCharacter;
      return (c[0] & 0b00111111) << 6 | (c[1] & 0b00111111);
    case 3: {
      if (len < 3) [[unlikely]]
        return ReplacementCharacter;
      char32_t u = ((c[0] & 0b00011111) << 12) | ((c[1] & 0b00111111) << 6)
          | (c[2] & 0b00111111);
#if !UTF8_FULL_UCS4_INSTEAD_OF_RFC_3629
      if (u >= 0xd800 && u <= 0xdfff) [[unlikely]]
        return ReplacementCharacter; // RFC 3629: surrogate halves are invalid
#endif
      return u;
    }
    case 4: {
      if (len < 4) [[unlikely]]
        return ReplacementCharacter;
      char32_t u = ((c[0] & 0b00001111) << 18) | ((c[1] & 0b00111111) << 12)
          | ((c[2] & 0b00111111) << 6) | (c[3] & 0b00111111);
  #if !UTF8_FULL_UCS4_INSTEAD_OF_RFC_3629
      if (u >= 0x10ffff) [[unlikely]]
        return ReplacementCharacter;  // RFC 3629: > 0x10ffff are invalid
  #endif
      return u;
    }
#if UTF8_FULL_UCS4_INSTEAD_OF_RFC_3629 // RFC 3629: > 0x10ffff are invalid
    case 5:
      if (len < 5) [[unlikely]]
        return ReplacementCharacter;
      return ((c[0] & 0b00000111) << 24) | ((c[1] & 0b00111111) << 18)
          | ((c[2] & 0b00111111) << 12) | ((c[3] & 0b00111111) << 6)
          | (c[4] & 0b00111111);
    case 6:
      if (len < 6) [[unlikely]]
        return ReplacementCharacter;
      return ((c[0] & 0b00000011) << 30) | ((c[1] & 0b00111111) << 24)
          | ((c[2] & 0b00111111) << 18) | ((c[3] & 0b00111111) << 12)
          | ((c[4] & 0b00111111) << 6) | (c[5] & 0b00111111);
#endif
  }
#else // C++ < 20: bitops
  if (c[0] <= 0x7f)
    return *s;
  if ((c[0] & 0b01000000) == 0)
    return ReplacementCharacter; // unexpected continuation character
  if ((c[0] & 0b00100000) == 0 && len > 1)
    return (c[0] & 0b00011111) << 6 | (c[1] & 0b00111111);
  if ((c[0] & 0b00010000) == 0 && len > 2) {
    char32_t u = ((c[0] & 0b00001111) << 12) | ((c[1] & 0b00111111) << 6)
        | (c[2] & 0b00111111);
#if !UTF8_FULL_UCS4_INSTEAD_OF_RFC_3629
    if (u >= 0xd800 && u <= 0xdfff)
      return ReplacementCharacter;  // RFC 3629: surrogate halves are invalid
#endif
    return u;
  }
  if ((c[0] & 0b00001000) == 0 && len > 3) {
    char32_t u = ((c[0] & 0b00000111) << 18) | ((c[1] & 0b00111111) << 12)
        | ((c[2] & 0b00111111) << 6) | (c[3] & 0b00111111);
#if !UTF8_FULL_UCS4_INSTEAD_OF_RFC_3629
    if (u >= 0x10ffff) // includes 0xfe 0xff bytes exclusion
        return ReplacementCharacter; // RFC 3629: > 0x10ffff are invalid
#endif
    return u;
  }
#if UTF8_FULL_UCS4_INSTEAD_OF_RFC_3629 // RFC 3629: > 0x10ffff are invalid
  if ((c[0] & 0b00000100) == 0 && len > 4)
    return ((c[0] & 0b00000011) << 24) | ((c[1] & 0b00111111) << 18)
        | ((c[2] & 0b00111111) << 12) | ((c[3] & 0b00111111) << 6)
        | (c[4] & 0b00111111);
  if ((c[0] & 0b00000010) == 0 && len > 5)
    return ((c[0] & 0b00000001) << 30) | ((c[1] & 0b00111111) << 24)
        | ((c[2] & 0b00111111) << 18) | ((c[3] & 0b00111111) << 12)
        | ((c[4] & 0b00111111) << 6) | (c[5] & 0b00111111);
#endif
#endif // C++ < 20: bitops
  return ReplacementCharacter;
}

Utf8String Utf8String::encodeUtf8(char32_t c) {
  if (c <= 0x7f) {
    unsigned char a = c & 0x7f;
    return Utf8String(&a, 1);
  }
  if (c <= 0x7ff) {
    unsigned char s[2];
    s[0] = 0b11000000 + (c >> 6);
    s[1] = 0b10000000 + (c & 0b111111);
    return Utf8String(s, sizeof s);
  }
#if !UTF8_FULL_UCS4_INSTEAD_OF_RFC_3629
  if (c >= 0xd800 && c <= 0xdfff) // RFC 3629: surrogate halves are invalid
    return ReplacementCharacterUtf8;
#endif
  if (c <= 0xffff) {
    unsigned char s[3];
    s[0] = 0b11100000 + (c >> 12);
    s[1] = 0b10000000 + ((c >> 6) & 0b111111);
    s[2] = 0b10000000 + (c & 0b111111);
    return Utf8String(s, sizeof s);
  }
#if !UTF8_FULL_UCS4_INSTEAD_OF_RFC_3629
  if (c >= 0x10ffff) // RFC 3629: > 0x10ffff are invalid
    return ReplacementCharacterUtf8;
#endif
  if (c <= 0x1fffff) {
    unsigned char s[4];
    s[0] = 0b11110000 + (c >> 18);
    s[1] = 0b10000000 + ((c >> 12) & 0b111111);
    s[2] = 0b10000000 + ((c >> 6) & 0b111111);
    s[3] = 0b10000000 + (c & 0b111111);
    return Utf8String(s, sizeof s);
  }
#if UTF8_FULL_UCS4_INSTEAD_OF_RFC_3629 // RFC 3629: > 0x10ffff are invalid
  if (c <= 0x3ffffff) {
    unsigned char s[5];
    s[0] = 0b11111000 + (c >> 24);
    s[1] = 0b10000000 + ((c >> 18) & 0b111111);
    s[2] = 0b10000000 + ((c >> 12) & 0b111111);
    s[3] = 0b10000000 + ((c >> 6) & 0b111111);
    s[4] = 0b10000000 + (c & 0b111111);
    return Utf8String(s, sizeof s);
  }
  if (c <= 0x7fffffff) {
    unsigned char s[6];
    s[0] = 0b11111100 + (c >> 30);
    s[1] = 0b10000000 + ((c >> 24) & 0b111111);
    s[2] = 0b10000000 + ((c >> 18) & 0b111111);
    s[3] = 0b10000000 + ((c >> 12) & 0b111111);
    s[4] = 0b10000000 + ((c >> 6) & 0b111111);
    s[5] = 0b10000000 + (c & 0b111111);
    return Utf8String(s, sizeof s);
  }
#endif
  return ReplacementCharacterUtf8;
}

char32_t Utf8String::toUpper(char32_t c) {
  auto cm = std::lower_bound(_case_mapping.cbegin(), _case_mapping.cend(), c);
  return cm == _case_mapping.end() || cm->utf32 != c ? c : cm->upper_utf32;
}

char32_t Utf8String::toLower(char32_t c) {
  auto cm = std::lower_bound(_case_mapping.cbegin(), _case_mapping.cend(), c);
  return cm == _case_mapping.end() || cm->utf32 != c ? c : cm->lower_utf32;
}

char32_t Utf8String::toTitle(char32_t c) {
  auto cm = std::lower_bound(_case_mapping.cbegin(), _case_mapping.cend(), c);
  return cm == _case_mapping.end() || cm->utf32 != c ? c : cm->title_utf32;
}

#endif // UTF8STRING_H
