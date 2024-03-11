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
  const static QList<char32_t> UnicodeWhitespace;
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
                 : v.canConvert<QString>()
                   ? v.toString().toUtf8()
                   : QByteArray{}) { }

  /** Return ith byte of the string, like operator[] or at() but safe if i
   *  is out of range. */
  [[nodiscard]] inline char value(qsizetype i, char def = 0) const {
    return size() < i+1 || i < 0 ? def : at(i); }
  /** Return ith utf8 character (bytes sequence) of the string. Safe if i
   *  is out of range. */
  [[nodiscard]] inline static Utf8String utf8value(
      qsizetype i, const char *s, qsizetype len,
      const Utf8String &def = Empty) {
    return utf8value(i, s, s+len, def); }
  [[nodiscard]] static Utf8String utf8value(
      qsizetype i, const char *s, const char *end,
      const Utf8String &def = Empty);
  [[nodiscard]] inline Utf8String utf8value(
      qsizetype i, const Utf8String &def = Empty) const {
    return utf8value(i, constData(), size(), def); }
  /** Return ith unicode character (code point value) of the string. Safe if i
   *  is out of range. */
  [[nodiscard]] inline static char32_t utf32value(
      qsizetype i, const char *s, qsizetype len, const char32_t def = 0) {
    Utf8String utf8 = utf8value(i, s, len, {});
    return utf8.isNull() ? def : decodeUtf8(utf8); }
  [[nodiscard]] inline static char32_t utf32value(
      qsizetype i, const char *s, const char *end, const char32_t def = 0) {
    Utf8String utf8 = utf8value(i, s, end, {});
    return utf8.isNull() ? def : decodeUtf8(utf8); }
  [[nodiscard]] inline char32_t utf32value(
      qsizetype i, char32_t def = 0) const {
    return utf32value(i, constData(), size(), def); }

  [[deprecated("use toUtf16() instead")]]
  [[nodiscard]] inline QString toString() const {
    return QString::fromUtf8(*this); }
  [[nodiscard]] inline QString toUtf16() const {
    return QString::fromUtf8(*this); }
  [[nodiscard]] inline operator QVariant() const {
    return QVariant::fromValue(*this); }

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
  [[nodiscard]] static inline char32_t toUpper(char32_t c);
  /** Convert a unicode charater into its uppercase character, e.g. É -> é .
   *  Return the input character itself if no change is needed e.g. E é #
   */
  [[nodiscard]] static inline char32_t toLower(char32_t c);
  /** Set the characters to title case.
   *  For most letters, title case is the same than upper case, but for some
   *  rare characters representing several letters at once, there is a title case
   *  for which the first letter is in upper case and others in lower cases.
   *  For instance ǆ lower case (unicode: 0x1C6) is mapped to Ǆ upper case
   *  (unicode: 0x1C4) and to ǅ title case letter (unicode: 0x1C5)
   *  Return the input character itself if no change is needed e.g. E É #
   */
  [[nodiscard]] static inline char32_t toTitle(char32_t c);
  [[nodiscard]] Utf8String toUpper() const;
  [[nodiscard]] Utf8String toLower() const;
  [[nodiscard]] Utf8String toTitle() const;
  [[nodiscard]] bool isLower() const;
  [[nodiscard]] bool isUpper() const;
  [[nodiscard]] bool isTitle() const;
  /** Convert to C-identifier: allow only letters, digits and underscores, by
   *  replacing unallowed chars with '_' and prefixing with '_' if it begins
   *  with a digit.
   *  e.g. "::foo" -> "__foo", "42" -> "_42", "foo*bar" -> "foo_bar"
   *  @param allow_non_ascii also allow >= 0x80 chars (but not as first char)
   */
  [[nodiscard]] Utf8String toIdentifier(bool allow_non_ascii = false) const;
  /** Convert to valid Internet header (RFC 5322): allow only ascii printable
   *  characters different from ':', by replacing unallowed chars with '_'.
   *  @param ignore_trailing_colon remove trailing ':' rather than adding '_'
   *  @see toInternetHeaderCase
   */
  [[nodiscard]] Utf8String toInternetHeaderName(
      bool ignore_trailing_colon = true) const;
  /** Convert to Kebab-Upper-Camel-Case, which is a common convention for
   *  Internet header (event though RFC 5322 allows any case and some large
   *  scale vendors such as AWS just ignore it).
   *  e.g. "host" -> "Host", "x-forwarded-for" -> "X-Forwarded-For",
   *  "_FOO" -> "-Foo", "foo_bar" -> "Foo-Bar", "2É" -> "2é"
   *  @see toInternetHeaderName
   */
  [[nodiscard]] Utf8String toInternetHeaderCase() const;

  // FIXME inline int compare(QByteArrayView a, Qt::CaseSensitivity cs) const noexcept;

  /** Return utf8 characters count. Count utf8 sequences without ensuring
    * their validity, so with invalid utf8 data this may overestimates. */
  [[nodiscard]] qsizetype utf8Size() const;
  /** Syntaxic sugar: !s === s.isNull() and thus !!s === !s.isNull() */
  [[nodiscard]] inline bool operator!() const { return isNull(); }
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

  // slicing: left right mid trimmed sliced chopped...
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
  [[nodiscard]] Utf8String utf8left(qsizetype len) const;
  /** Return rightmost len utf8 characters. */
  [[nodiscard]] Utf8String utf8right(qsizetype len) const;
  /** Return len utf8 characters starting at pos.
   *  Everything after pos if len < 0 or pos+len > size(). */
  [[nodiscard]] Utf8String utf8mid(qsizetype pos, qsizetype len = -1) const;
  [[nodiscard]] Utf8String trimmed() const { return QByteArray::trimmed(); }
  Utf8String &trim() { *this = trimmed(); return *this; }
  [[nodiscard]] inline Utf8String chopped(qsizetype len) const {
    return QByteArray::chopped(len); }
  /** like left() but crashes if out of bound. */
  [[nodiscard]] inline Utf8String first(qsizetype n) const {
    return QByteArray::first(n); }
  /** like right() but crashes if out of bound. */
  [[nodiscard]] inline Utf8String last(qsizetype n) const {
    return QByteArray::last(n); }
  /** like mid() but crashes if out of bound. */
  [[nodiscard]] inline Utf8String sliced(qsizetype pos) const {
    return QByteArray::sliced(pos); }
  /** like mid() but crashes if out of bound. */
  [[nodiscard]] inline Utf8String sliced(qsizetype pos, qsizetype n) const {
    return QByteArray::sliced(pos, n); }

  // splitting
  /** Splitting utf8 string on ascii 7 separators, e.g. {',',';'}
    * @see Utf8String::AsciiWhitespace */
  [[nodiscard]] const Utf8StringList split_after(
      QList<char> seps, qsizetype offset = 0,
      Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Splitting utf8 string on ascii 7 separator, e.g. ' ' */
  [[nodiscard]] const Utf8StringList split_after(
      const char sep, const qsizetype offset = 0,
      Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Splitting utf8 string on multi-byte (utf8) or multi-char separator,
   *  e.g. "-->", "🥨"_u8, U'🥨', "<≠>"_u8 */
  [[nodiscard]] const Utf8StringList split_after(
      Utf8String sep, qsizetype offset = 0,
      Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Splitting utf8 string on ascii 7 separators, e.g. {',',';'}
    * @see Utf8String::AsciiWhitespace */
  [[nodiscard]] const Utf8StringList split(
      QList<char> seps, Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Splitting utf8 string on ascii 7 separator, e.g. ' ' */
  [[nodiscard]] const Utf8StringList split(
      const char sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Splitting utf8 string on multi-byte (utf8) or multi-char separator,
   *  e.g. "-->", "🥨"_u8, U'🥨', "<≠>"_u8 */
  [[nodiscard]] const Utf8StringList split(
      Utf8String sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Split the string using its first utf8 char as a delimiter.
   *  e.g. "/foo/bar/g" -> { "foo", "bar", "g" }
   *  e.g. ",/,:,g" -> { "/", ":", "g" }
   *  e.g. "§foo§bar§g" -> { "foo", "bar", "g" }
   *  e.g. "越foo越bar越g" -> { "foo", "bar", "g" }
   *  e.g. "🥨foo🥨bar🥨g" -> { "foo", "bar", "g" }
   */
  [[nodiscard]] const Utf8StringList splitByLeadingChar(
      qsizetype offset = 0) const;

  // conversions to numbers
  /** Converts to floating point, supporting e notation and SI suffixes from 'f'
   *  to 'P', 'u' is used as 1e-6 suffix e.g. ".1k" -> 100.0. */
  [[nodiscard]] double toDouble(
      bool *ok = nullptr, double def = 0.0, bool suffixes_enabled = true) const;
  /** Syntaxic sugar */
  [[nodiscard]] inline double toDouble(double def) const{
    return toDouble(nullptr, def, true); }
  /** Converts to floating point, supporting e notation and SI suffixes from 'f'
   *  to 'P', 'u' is used as 1e-6 suffix e.g. ".1k" -> 100.0. */
  [[nodiscard]] float toFloat(bool *ok = nullptr, float def = 0.0,
                              bool suffixes_enabled = true) const;
  /** Syntaxic sugar */
  [[nodiscard]] inline float toFloat(float def) const{
    return toFloat(nullptr, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  [[nodiscard]] qlonglong toLongLong(
      bool *ok = nullptr, int base = 0, qlonglong def = 0,
      bool suffixes_enabled = true, bool floating_point_enabled = true) const;
  /** Syntaxic sugar */
  [[nodiscard]] inline qlonglong toLongLong(qlonglong def) const {
    return toLongLong(nullptr, 0, def, true); }
  /** Syntaxic sugar */
  [[nodiscard]] inline qlonglong toLongLong(long def) const {
    return toLongLong(nullptr, 0, def, true); }
  /** Syntaxic sugar */
  [[nodiscard]] inline qlonglong toLongLong(int def) const {
    return toLongLong(nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  [[nodiscard]] qulonglong toULongLong(
      bool *ok = nullptr, int base = 0, qulonglong def = 0,
      bool suffixes_enabled = true, bool floating_point_enabled = true) const;
  /** Syntaxic sugar */
  [[nodiscard]] inline qulonglong toULongLong(qulonglong def) const {
    return toULongLong(nullptr, 0, def, true); }
  /** Syntaxic sugar */
  [[nodiscard]] inline qulonglong toULongLong(ulong def) const {
    return toULongLong(nullptr, 0, def, true); }
  /** Syntaxic sugar */
  [[nodiscard]] inline qulonglong toULongLong(uint def) const {
    return toULongLong(nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  [[nodiscard]] long toLong(
      bool *ok = nullptr, int base = 0, long def = 0,
      bool suffixes_enabled = true, bool floating_point_enabled = true) const;
  /** Syntaxic sugar */
  [[nodiscard]] inline long toLong(long def) const {
    return toLong(nullptr, 0, def, true); }
  /** Syntaxic sugar */
  [[nodiscard]] inline long toLong(int def) const {
    return toLong(nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  [[nodiscard]] ulong toULong(
      bool *ok = nullptr, int base = 0, ulong def = 0,
      bool suffixes_enabled = true, bool floating_point_enabled = true) const;
  /** Syntaxic sugar */
  [[nodiscard]] inline ulong toULong(ulong def) const {
    return toULong(nullptr, 0, def, true); }
  /** Syntaxic sugar */
  [[nodiscard]] inline ulong toULong(uint def) const {
    return toULong(nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  [[nodiscard]] int toInt(
      bool *ok = nullptr, int base = 0, int def = 0,
      bool suffixes_enabled = true, bool floating_point_enabled = true) const;
  /** Syntaxic sugar */
  [[nodiscard]] inline int toInt(int def) const {
    return toInt(nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  [[nodiscard]] uint toUInt(
      bool *ok = nullptr, int base = 0, uint def = 0,
      bool suffixes_enabled = true, bool floating_point_enabled = true) const;
  /** Syntaxic sugar */
  [[nodiscard]] inline uint toUInt(uint def) const {
    return toUInt(nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  [[nodiscard]] short toShort(
      bool *ok = nullptr, int base = 0, short def = 0,
      bool suffixes_enabled = true, bool floating_point_enabled = true) const;
  /** Syntaxic sugar */
  [[nodiscard]] inline short toShort(short def) const {
    return toShort(nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  [[nodiscard]] ushort toUShort(
      bool *ok = nullptr, int base = 0, ushort def = 0,
      bool suffixes_enabled = true, bool floating_point_enabled = true) const;
  /** Syntaxic sugar */
  [[nodiscard]] inline ushort toUShort(ushort def) const {
    return toUShort(nullptr, 0, def, true); }
  /** Converts to bool, supporting case insensitive "true" and "false", and any
   *  integer number, 0 being false and everything else true. */
  [[nodiscard]] bool toBool(bool *ok = nullptr, bool def = false) const;
  /** Syntaxic sugar */
  [[nodiscard]] inline bool toBool(bool def) const {
    return toBool(nullptr, def); }
  /** Converts to any number format, calling toXXX() methods above (and
   *  concerning integers, with base = 0 auto detection of base). */
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  [[nodiscard]] inline T toNumber(
      bool *ok = nullptr, const T &def = {},
      bool suffixes_enabled = true, bool floating_point_enabled = true) const;
  /** Convenience methods witout bool *ok. */
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  [[nodiscard]] inline T toNumber(
      const T &def, bool suffixes_enabled = true,
      bool floating_point_enabled = true) const {
    return toNumber<T>(nullptr, def, suffixes_enabled, floating_point_enabled);}

  // conversions from numbers
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
  [[nodiscard]] static inline Utf8String number(bool b);
  /** Write a number using a given arbitrary bijective base.
   *  0 is always "", 1 is the first char of the base etc.
   *  e.g. "ABCDEFGHIJKLMNOPQRSTUVWXYZ" is used for spreadsheets column notation
   *  and european driving plate left and right parts. 1 -> A, 26 -> Z,
   *  27 -> AA, 16384 -> XFD
   *  @see https://en.wikipedia.org/wiki/Bijective_numeration */
  [[nodiscard]] static inline Utf8String bijectiveBaseNumber(
      qulonglong i, const QByteArray &base) {
    Utf8String s;
    auto n = base.size();
    for (; i; i = (i-1)/n)
      s.prepend(base[(i-1)%n]);
    return s;
  }

  // byte arrays conversion
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

  // misc conversions
  /** Return list of contained bytes, sorted and deduplicated. */
  QList<char> toBytesSortedList() const;

  // replace
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
  Utf8String &replace(const QRegularExpression &re, const Utf8String &after);

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
  // supporting int index avoid ambiguity when using 0 as a parameter since
  // (int)0 can be converted to both (std::size_t)0 or (const char *)nullptr
  inline Utf8String &remove(int index, qsizetype len) {
    QByteArray::remove(index, len); return *this;}
  Utf8String &remove(const char *needle, qsizetype len);
  inline Utf8String &remove(char c) {
    return remove(&c, 1); }
  inline Utf8String &remove(const QByteArray &ba) {
    return remove(ba.constData(), ba.size()); }
  inline Utf8String &remove(char32_t c) {
    return remove(encodeUtf8(c)); }
  inline Utf8String &removeAt(qsizetype pos) {
    QByteArray::removeAt(pos); return *this;}
  Utf8String &remove_ascii_chars(QList<char> chars);
  inline Utf8String &removeFirst() { QByteArray::removeFirst(); return *this;}
  inline Utf8String &removeLast() { QByteArray::removeLast(); return *this;}

  /** Empty coalesce: replace with that if this is empty. */
  inline Utf8String &coalesce(const Utf8String &that) {
    if (isEmpty()) *this = that;
    return *this; }
  /** Empty coalesce: replace with that if this is empty. */
  inline Utf8String &coalesce(const QString &that) {
    if (isEmpty()) *this = that;
    return *this; }
  /** Empty coalesce: replace with that if this is empty.
   *  Assume UTF-8. */
  inline Utf8String &coalesce(const QByteArray &that) {
    if (isEmpty()) *this = that;
    return *this; }
  /** Empty coalesce: replace with that if this is empty.
   *  Assume UTF-8, len = -1 means zero-terminated string. */
  inline Utf8String &coalesce(const char *s, qsizetype len = -1) {
    if (isEmpty()) *this = QByteArray(s, len);
    return *this; }
  /** Empty coalesce: replace with that if this is empty. */
  inline Utf8String &coalesce(const QVariant &that) {
    if (isEmpty()) *this = that;
    return *this; }
  /** Empty coalesce operator */
  inline Utf8String &operator|=(const Utf8String &that) {
    return coalesce(that); }
  /** Empty coalesce operator */
  inline Utf8String &operator|=(const QString &that) {
    return coalesce(that); }
  /** Empty coalesce operator
   *  Assume UTF-8. */
  inline Utf8String &operator|=(const QByteArray &that) {
    return coalesce(that); }
  /** Empty coalesce operator */
  inline Utf8String &operator|=(const QVariant &that) {
    return coalesce(that); }

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
  inline Utf8String &operator=(const QVariant &v) {
    return operator=(Utf8String(v)); }

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
  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const Utf8String &x, const QString &y) {
    QByteArray ba = y.toUtf8();
    return Utf8String::cmp(x.constData(), x.size(), ba.constData(), ba.size()); }
  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const QString &x, const Utf8String &y) {
    QByteArray ba = x.toUtf8();
    return Utf8String::cmp(ba.constData(), ba.size(), y.constData(), y.size()); }

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
  [[nodiscard]] friend inline bool operator==(
      const Utf8String &x, const QString &y) {
    QByteArray ba = y.toUtf8();
    return Utf8String::cmp(x.constData(), x.size(), ba.constData(), ba.size()) == 0; }
  [[nodiscard]] friend inline bool operator==(
      const QString &x, const Utf8String &y) {
    QByteArray ba = x.toUtf8();
    return Utf8String::cmp(ba.constData(), ba.size(), y.constData(), y.size()) == 0; }
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

#ifdef UTF8STRING_IMPL_CPP
  bool convToBool(bool *ok) const { return toBool(ok, {}); }
  double convToDouble(bool *ok) const { return toDouble(ok, {}); }
  float convToFloat(bool *ok) const { return toFloat(ok, 0.0); }
  qlonglong convToLongLong(bool *ok) const { return toLongLong(ok, 0.0); }
  qulonglong convToULongLong(bool *ok) const { return toULongLong(ok, 0.0); }
  long convToLong(bool *ok) const { return toLong(ok, 0.0); }
  ulong convToULong(bool *ok) const { return toULong(ok, 0.0); }
  int convToInt(bool *ok) const { return toInt(ok, 0.0); }
  uint convToUInt(bool *ok) const { return toUInt(ok, 0.0); }
  short convToShort(bool *ok) const { return toShort(ok, 0.0); }
  ushort convToUShort(bool *ok) const { return toUShort(ok, 0.0); }
#endif

  [[nodiscard]] static Utf8String fromCEscaped(
      const char *escaped, qsizetype len);
  [[nodiscard]] inline static Utf8String fromCEscaped(
      const Utf8String &escaped) {
    return fromCEscaped(escaped.constData(), escaped.size()); }
  [[nodiscard]] inline static Utf8String fromCEscaped(
      const QByteArrayView &escaped) {
    return fromCEscaped(escaped.constData(), escaped.size()); }

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

#if UTF8STRING_COALESCE_DOUBLE_PIPE_OPERATORS
/** Empty coalesce operator */
inline const Utf8String &operator||(const Utf8String &a1, const Utf8String &a2) {
  return a1.isEmpty() ? a2 : a1; }
/** Empty coalesce operator */
inline const Utf8String operator||(const Utf8String &a1, const QString &a2) {
  return a1.isEmpty() ? Utf8String(a2) : a1; }
/** Empty coalesce operator */
inline const Utf8String operator||(const QString &a1, const Utf8String &a2) {
  return a1.isEmpty() ? a2 : Utf8String(a1); }
/** Empty coalesce operator */
inline const Utf8String operator||(const QString &a1, const QString &a2) {
  return a1.isEmpty() ? a2 : a1; }
/** Empty coalesce operator */
inline const Utf8String operator||(const Utf8String &a1, const QByteArray &a2) {
  return a1.isEmpty() ? Utf8String(a2) : a1; }
/** Empty coalesce operator */
inline const Utf8String operator||(const QByteArray &a1, const Utf8String &a2) {
  return a1.isEmpty() ? a2 : Utf8String(a1); }
/** Empty coalesce operator */
inline const Utf8String operator||(const Utf8String &a1, const char *a2) {
  return a1.isEmpty() ? Utf8String(a2) : a1; }
/** Empty coalesce operator */
inline const Utf8String operator||(const char *a1, const Utf8String &a2) {
  return !a1 || !*a1 ? a2 : Utf8String(a1); }
#endif

/** Empty coalesce operator */
inline const Utf8String &operator|(const Utf8String &a1, const Utf8String &a2) {
  return a1.isEmpty() ? a2 : a1; }
/** Empty coalesce operator */
inline const Utf8String operator|(const Utf8String &a1, const QString &a2) {
  return a1.isEmpty() ? Utf8String(a2) : a1; }
/** Empty coalesce operator */
inline const Utf8String operator|(const QString &a1, const Utf8String &a2) {
  return a1.isEmpty() ? a2 : Utf8String(a1); }
/** Empty coalesce operator */
inline const Utf8String operator|(const QString &a1, const QString &a2) {
  return a1.isEmpty() ? a2 : a1; }
/** Empty coalesce operator */
inline const Utf8String operator|(const Utf8String &a1, const QByteArray &a2) {
  return a1.isEmpty() ? Utf8String(a2) : a1; }
/** Empty coalesce operator */
inline const Utf8String operator|(const QByteArray &a1, const Utf8String &a2) {
  return a1.isEmpty() ? a2 : Utf8String(a1); }
/** Empty coalesce operator */
inline const Utf8String operator|(const Utf8String &a1, const char *a2) {
  return a1.isEmpty() ? Utf8String(a2) : a1; }
/** Empty coalesce operator */
inline const Utf8String operator|(const char *a1, const Utf8String &a2) {
  return !a1 || !*a1 ? a2 : Utf8String(a1); }
/** Empty coalesce operator */
inline const Utf8String operator|(const Utf8String &a1, const QVariant &a2) {
  return a1.isEmpty() ? Utf8String(a2) : a1; }
/** Empty coalesce operator */
inline const Utf8String operator|(const QVariant &a1, const Utf8String &a2) {
  Utf8String s{a1}; return s.isEmpty() ? a2 : s; }

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

[[nodiscard]] inline Utf8String Utf8String::number(bool b) {
  return b ? "true"_u8: "false"_u8;
}

template<>
[[nodiscard]] inline double Utf8String::toNumber<>(
    bool *ok, const double &def, bool suffixes_enabled, bool) const {
  return toDouble(ok, def, suffixes_enabled);
}

template<>
[[nodiscard]] inline float Utf8String::toNumber<>(
    bool *ok, const float &def, bool suffixes_enabled, bool) const {
  return toFloat(ok, def, suffixes_enabled);
}

template<>
[[nodiscard]] inline qlonglong Utf8String::toNumber<>(
    bool *ok, const qlonglong &def, bool suffixes_enabled,
    bool floating_point_enabled) const {
  return toLongLong(ok, 0, def, suffixes_enabled, floating_point_enabled);
}

template<>
[[nodiscard]] inline qulonglong Utf8String::toNumber<>(
    bool *ok, const qulonglong &def, bool suffixes_enabled,
    bool floating_point_enabled) const {
  return toULongLong(ok, 0, def, suffixes_enabled, floating_point_enabled);
}

template<>
[[nodiscard]] inline long Utf8String::toNumber<>(
    bool *ok, const long &def, bool suffixes_enabled,
    bool floating_point_enabled) const {
  return toLong(ok, 0, def, suffixes_enabled, floating_point_enabled);
}

template<>
[[nodiscard]] inline ulong Utf8String::toNumber<>(
    bool *ok, const ulong &def, bool suffixes_enabled,
    bool floating_point_enabled) const {
  return toULong(ok, 0, def, suffixes_enabled, floating_point_enabled);
}

template<>
[[nodiscard]] inline int Utf8String::toNumber<>(
    bool *ok, const int &def, bool suffixes_enabled,
    bool floating_point_enabled) const {
  return toInt(ok, 0, def, suffixes_enabled, floating_point_enabled);
}

template<>
[[nodiscard]] inline uint Utf8String::toNumber<>(
    bool *ok, const uint &def, bool suffixes_enabled,
    bool floating_point_enabled) const {
  return toUInt(ok, 0, def, suffixes_enabled, floating_point_enabled);
}

template<>
[[nodiscard]] inline short Utf8String::toNumber<>(
    bool *ok, const short &def, bool suffixes_enabled,
    bool floating_point_enabled) const {
  return toShort(ok, 0, def, suffixes_enabled, floating_point_enabled);
}

template<>
[[nodiscard]] inline ushort Utf8String::toNumber<>(
    bool *ok, const ushort &def, bool suffixes_enabled,
    bool floating_point_enabled) const {
  return toUShort(ok, 0, def, suffixes_enabled, floating_point_enabled);
}

template<>
[[nodiscard]] inline bool Utf8String::toNumber<>(
    bool *ok, const bool &def, bool, bool) const {
  return toBool(ok, def);
}

#endif // UTF8STRING_H
