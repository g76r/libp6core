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

#ifndef UTF8STRING_H
#define UTF8STRING_H

#include "util/numberutils.h"
#include <QVariant>

using namespace Qt::Literals::StringLiterals;

class Utf8StringList;
class Utf8StringSet;
class QDebug;
class QLine;
class QLineF;
class QPoint;
class QPointF;
class QRect;
class QRectF;
class QSize;
class QSizeF;

/** Enhanced QByteArray with string methods, always assuming 8 bits content is a
 * UTF-8 encoded string (QByteArray, char *, etc.). */
class LIBP6CORESHARED_EXPORT Utf8String : public QByteArray {
public:
  const static int MetaTypeId;
  const static QList<char> AsciiWhitespace; // ' ', '\t', '\n'...
  const static QList<char32_t> UnicodeWhitespace; // same plus \u0084, \u2000...
  enum WellKnownUnicodeCharacters : char32_t {
    ReplacementCharacter = U'\ufffd',
    ByteOrderMark = U'\ufeff',
    NextLine = U'\u0085',
    NoBreakSpace = U'\u00a0',
    EnQuad = U'\u2000',
    EmQuad = U'\u2001',
    EnSpace = U'\u2002',
    EmSpace = U'\u2003',
    FigureSpace = U'\u2007',
    PunctuationSpace = U'\u2008',
    ThinSpace = U'\u2009',
    HairSpace = U'\u200a',
    LineSeparator = U'\u2028',
    ParagraphSeparator = U'\u2029',
    NarrowNonBreakSpace = U'\u202f',
  };
  const static Utf8String ReplacementCharacterUtf8; // "\xef\xbf\xbd"_u8
  const static Utf8String Empty; // ""_u8
  const static Utf8String DefaultEllipsis; // "..."_u8
  const static Utf8String DefaultPadding; // " "_u8

  inline constexpr Utf8String() noexcept {}
  inline constexpr Utf8String(std::nullptr_t) noexcept {}
  inline Utf8String(const QByteArray &ba) noexcept : QByteArray(ba) {}
  inline Utf8String(QByteArray &&ba) noexcept : QByteArray(std::move(ba)) {}
  inline Utf8String(const QByteArrayData ba) noexcept : QByteArray(ba) {}
  inline Utf8String(const QString &s) noexcept : QByteArray(s.toUtf8()) {}
  inline Utf8String(const Utf8String &other) : QByteArray(other) {}
  inline Utf8String(Utf8String &&other) noexcept
    : QByteArray(std::move(other)) {}
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
  inline Utf8String(const char8_t *s, qsizetype size = -1)
    : QByteArray((const char *)s, size) { }
  inline Utf8String(const char *s, qsizetype size = -1)
    : QByteArray(s, size) { }
  explicit inline Utf8String(char c) : QByteArray(&c, 1) { }
  explicit inline Utf8String(signed char c) : QByteArray((char*)&c, 1) { }
  explicit inline Utf8String(unsigned char c) : QByteArray((char*)&c, 1) { }
  explicit inline Utf8String(char8_t c) : QByteArray((char*)&c, 1) { }
  explicit inline Utf8String(char32_t u) : QByteArray(encode_utf8(u)) { }
  inline ~Utf8String() noexcept {}
  // std::integral includes bool, which will be converted to "true" or "false"
  explicit inline Utf8String(std::integral auto i)
    : QByteArray(Utf8String::number(i)) {}
  explicit inline Utf8String(std::floating_point auto f)
    : QByteArray(Utf8String::number(f)) {}
  explicit Utf8String(const QPointF &point);
  explicit Utf8String(const QSizeF &size);
  explicit Utf8String(const QRectF &rect);
  explicit Utf8String(const QLineF &line);
  explicit Utf8String(const QList<QPointF> &list);
  template <p6::arithmetic T>
  inline explicit Utf8String(const QList<T> &list) {
    for (bool begin = true; auto number: list) {
      if (begin)
        begin = false;
      else
        append(',');
      append(Utf8String::number(number));
    }
  }
  /** Build Utf8String from QVariant.
   *  Take:
   *  - QByteArray if v.canConvert<QByteArray>() (assuming UTF-8), which
   *    includes Utf8String (since Utf8String is a QByteArray)
   *  - otherwise take QString and convert to UTF-8
   *  - otherwise {}
   *  This means that Utf8String(QVariant{}) == Utf8String{} or also that
   *  Utf8String(v).isNull() when !v.isValid(), so if you use QVariant{}
   *  semanticaly as a null value it will be consistent with Utf8String{}. */
  explicit inline Utf8String(QVariant v)
    : QByteArray(v.canConvert<Utf8String>()
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
  [[nodiscard]] inline static Utf8String utf8value(
      qsizetype i, const char *s, qsizetype len,
      const Utf8String &def = Empty) {
    return utf8value(i, s, s+len, def); }
  [[nodiscard]] inline static Utf8String utf8value(
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
    return utf8.isNull() ? def : decode_utf8(utf8); }
  [[nodiscard]] inline static char32_t utf32value(
      qsizetype i, const char *s, const char *end, const char32_t def = 0) {
    Utf8String utf8 = utf8value(i, s, end, {});
    return utf8.isNull() ? def : decode_utf8(utf8); }
  [[nodiscard]] inline char32_t utf32value(
      qsizetype i, char32_t def = 0) const {
    return utf32value(i, constData(), size(), def); }

  [[deprecated("use toUtf16() instead")]]
  [[nodiscard]] inline QString toString() const {
    return QString::fromUtf8(*this); }
  [[nodiscard]] inline QString toUtf16() const {
    return QString::fromUtf8(*this); }
  explicit inline operator QVariant() const {
    return QVariant::fromValue(*this); }
  explicit inline operator QString() const { return toUtf16(); }

  [[nodiscard, gnu::const]] static inline Utf8String encode_utf8(char32_t u);
  /** Decode first unicode character in utf8 bytes.
   *  Return replacement character (\ufffd) if an invalid sequence is found.
   *  Return a BOM character (\ufeff) if first bytes sequence is a BOM (\xef
   *  \xbb \xbf). */
  template <bool strict = true>
  [[nodiscard]] static inline char32_t decode_utf8(
      const char *s, const char *end) {
    return decode_utf8_and_step_forward<strict>(&s, end); }
  template <bool strict = true>
  [[nodiscard]] static inline char32_t decode_utf8(
      const char *s, qsizetype len) {
    return decode_utf8_and_step_forward<strict>(&s, s+len); }
  template <bool strict = true>
  [[nodiscard]] static inline char32_t decode_utf8(
      const QByteArray &s) {
    auto s2 = s.constData();
    return decode_utf8_and_step_forward<strict>(&s2, s2+s.size()); }
  /** test if c is a continuation byte i.e. 0b10xx'xxxx */
  [[nodiscard]] static inline bool is_utf8_continuation_byte(char c) {
    return ((unsigned char)c & 0b1100'0000) == 0b1000'0000;
  }
  template <bool strict = true>
  [[nodiscard]] static inline char32_t decode_utf8(const char *s) {
    return decode_utf8_and_step_forward<strict>(&s, s+::strlen(s)); }
  /** Low-level one utf8 character decoder.
   *  Decode utf8 character at *s and set *s after the character's byte
   *  sequence, never reading *end and beyond.
   *  Return replacement char (\ufffd) if sequence at *s is invalid (in this
   *  case set *s to the next possible valid sequence position, or at end if the
   *  sequence is invalid because it's incomplete).
   *  When strict is false: accept over long sequences (such as 0xc0 0x80 for
   *  \0), ignore 2 most significant bits of continuation bytes (0xc3 0x09 will
   *  form a É as 0xc3 0x89) because with valid utf8 it makes the code faster.
   */
  template <bool strict = true>
  [[nodiscard]] static inline char32_t decode_utf8_and_step_forward(
      const char **s, const char *end);
  /** Low-level one utf8 character encoder.
   *  Encode utf8 character at *d and set *d after the character's byte
   *  sequence, there must be enough room in *d for that.
   *  Don't write and advance if c is invalid or a replacement character.
   */
  static inline void encode_utf8_and_step_forward(char **d, char32_t u);
  /** Low-level non-decoding begin of next utf8 char finder.
   *  Intrement s until at the begin of a valid utf8 bytes sequence.
   *  If s is already at a char begin, do nothing.
   *  Return nullptr if end is reached.
   *  Skip out of sequence (erroneous) bytes and optionnaly BOMs.
   */
  template <bool skip_bom = true>
  static inline const char *go_forward_to_utf8_char(
      const char **s, const char *end);
  /** Low-level non-decoding begin of previous utf8 char finder.
   *  Decrement s until at the begin of a valid utf8 bytes sequence.
   *  If s is already at a char begin, do nothing.
   *  Return nullptr if begin is exceeded.
   *  Skip out of sequence (erroneous) bytes and optionnaly BOMs.
   */
  template <bool skip_bom = true>
  static inline const char *go_backward_to_utf8_char(
      const char **s, const char *begin);

  /** Convert a unicode charater into its upper case character, e.g. é -> É.
   *  Return the input character itself if no change is needed e.g. E É #
   */
  [[nodiscard, gnu::const]] static inline char32_t toUpper(char32_t u);
  /** Convert a unicode charater into its lower case character, e.g. É -> é .
   *  Return the input character itself if no change is needed e.g. e é #
   */
  [[nodiscard, gnu::const]] static inline char32_t toLower(char32_t u);
  /** Set the characters to title case.
   *  For most letters, title case is the same than upper case, but for some
   *  rare characters representing several letters at once, there is a title case
   *  for which the first letter is in upper case and others in lower cases.
   *  For instance ǆ lower case (unicode: 0x1C6) is mapped to Ǆ upper case
   *  (unicode: 0x1C4) and to ǅ title case letter (unicode: 0x1C5)
   *  Return the input character itself if no change is needed e.g. E É #
   */
  [[nodiscard, gnu::const]] static inline char32_t toTitle(char32_t u);
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
  [[nodiscard]] Utf8String toIdentifier(
      bool allow_non_ascii = false) const;
  /** Convert to valid Internet header (RFC 5322): allow only ascii printable
   *  characters different from ':', by replacing unallowed chars with '_'.
   *  @param ignore_trailing_colon remove trailing ':' rather than adding '_'
   *  @see toInternetHeaderCase
   */
  [[nodiscard]] Utf8String toInternetHeaderName(
      bool ignore_trailing_colon = true) const;
  /** Convert to Kebab-Upper-Camel-Case, which is a common convention for
   *  Internet header (even though RFC 5322 allows any case and some large
   *  scale vendors such as AWS just ignore it).
   *  e.g. "host" -> "Host", "x-forwarded-for" -> "X-Forwarded-For",
   *  "_FOO" -> "-Foo", "foo_bar" -> "Foo-Bar", "2É" -> "2é"
   *  @see toInternetHeaderName
   */
  [[nodiscard]] Utf8String toInternetHeaderCase() const;

  // FIXME inline int compare(QByteArrayView a, Qt::CaseSensitivity cs) const noexcept;

  /** Return unicode characters count. Count utf8 sequences without ensuring
    * their validity, so with invalid utf8 data this may overestimates. */
  [[nodiscard]] inline qsizetype utf8size() const;
  [[deprecated("use utf8size (without capital letter) instead")]]
  [[nodiscard]] inline qsizetype utf8Size() const { return utf8size(); }
  /** Syntaxic sugar: !s === s.isNull() and thus !!s === !s.isNull() */
  [[nodiscard]] inline bool operator!() const { return isNull(); }
  inline Utf8String &fill(char c, qsizetype size = -1) {
    QByteArray::fill(c, size); return *this; }
  /** Return valid utf8 without invalid sequences.
   *  @param strict don't be tolerant to encoding errors (overlong sequences,
   *    etc.)
   *  @param keep_replacement_chars write a replacement char where there was one
   *    or where there was an invalid sequence
   *  @param keep_bom keep byte order marks if any
   */
  template <bool strict = true, bool keep_replacement_chars = false,
            bool keep_bom = false>
  [[nodiscard]] inline Utf8String cleaned() const {
    return cleaned<strict, keep_replacement_chars, keep_bom>(
          constData(), size()); }
  /** Return valid utf8 without invalid sequences. */
  template <bool strict = true, bool keep_replacement_chars = false,
            bool keep_bom = false>
  [[nodiscard]] inline static Utf8String cleaned(
      const char *s, qsizetype len) {
    return cleaned<strict, keep_replacement_chars, keep_bom>(s, s+len); }
  /** Return valid utf8 without invalid sequences. */
  template <bool strict = true, bool keep_replacement_chars = false,
            bool keep_bom = false>
  [[nodiscard]] inline static Utf8String cleaned(
      const char *s, const char *end);
  /** Reencode valid utf8 in-place without invalid sequences. */
  template <bool strict = true, bool keep_replacement_chars = false,
            bool keep_bom = false>
  inline Utf8String &clean();
  /** Reencode valid utf8 in-place without invalid sequences. */
  template <bool strict = true, bool keep_replacement_chars = false,
            bool keep_bom = false>
  inline static void clean(char *s, const char *end);

  // slicing: left right mid trimmed sliced chopped...
  /** Return leftmost len bytes. Empty if len < 0. */
  [[nodiscard]] inline Utf8String left(qsizetype len) const {
    return QByteArray::left(len); }
  /** Return rightmost len bytes. Empty if len < 0. */
  [[nodiscard]] inline Utf8String right(qsizetype len) const {
    return QByteArray::right(len); }
  /** Return len bytes starting at pos.
   *  Everything after pos if len < 0 or pos+len > size(). */
  [[nodiscard]] inline Utf8String mid(qsizetype pos, qsizetype len = -1) const {
    return QByteArray::mid(pos, len); }
  /** Return leftmost len unicode characters. */
  [[nodiscard]] inline Utf8String utf8left(qsizetype len) const;
  /** Return rightmost len unicode characters. */
  [[nodiscard]] inline Utf8String utf8right(qsizetype len) const;
  /** Return len unicode characters starting at pos.
   *  Everything after pos if len < 0 or pos+len > size(). */
  [[nodiscard]] inline Utf8String utf8mid(
      qsizetype pos, qsizetype len = -1) const;
  [[nodiscard]] inline Utf8String trimmed() const {
    return QByteArray::trimmed(); }
  inline Utf8String &trim() { *this = trimmed(); return *this; }
  [[nodiscard]] inline Utf8String chopped(qsizetype len) const {
    return QByteArray::chopped(len); }
  /** remove last len unicode characters */
  inline void utf8chop(qsizetype len);
  /** return a string without last len unicode characters */
  [[nodiscard]] inline Utf8String utf8chopped(qsizetype len) const;
  /** like left() but crashes if out of bound. */
  [[nodiscard]] inline Utf8String first(qsizetype n) const {
    return QByteArray::first(n); }
  /** like right() but crashes if out of bound. */
  [[nodiscard]] inline Utf8String last(qsizetype n) const {
    return QByteArray::last(n); }
#if QT_VERSION >= 0x060800
  /** like mid() but crashes if out of bound. */
  inline Utf8String &slice(qsizetype pos) {
    QByteArray::slice(pos); return *this; }
  /** like mid() but crashes if out of bound. */
  inline Utf8String &slice(qsizetype pos, qsizetype n) {
    QByteArray::slice(pos, n); return *this; }
#endif
  /** like mid() but crashes if out of bound. */
  [[nodiscard]] inline Utf8String sliced(qsizetype pos) const {
    return QByteArray::sliced(pos); }
  /** like mid() but crashes if out of bound. */
  [[nodiscard]] inline Utf8String sliced(qsizetype pos, qsizetype n) const {
    return QByteArray::sliced(pos, n); }

  // eliding
  /** Truncate string to match at most maxsize characters/bytes, adding an
   *  ellipsis string (e.g. "...") where truncated.
   *  e.g. elide(0,false,"foobar",5,"§") -> "fo§ar"
   *  e.g. elide(-1,true,"foo§ar",4,"") -> "§ar" ("§" is 2 bytes long)
   *  You probably rather want to call specialized forms like elide_right().
   * @param direction: -1 left 0 middle +1 right
   * @param binary true to count bytes, false to count unicode chars
   */
  [[nodiscard]] static inline Utf8String elide(
      const int direction, const bool binary, const Utf8String &s,
      const qsizetype maxsize, const Utf8String &ellipsis) {
    auto ss = binary ? s.size() : s.utf8size();
    if (maxsize < 0 || ss < maxsize) // input string already fits
      [[likely]] return s;
    auto es = binary ? ellipsis.size() : ellipsis.utf8size();
    if (es >= maxsize) { // ellipsis is larger than maxsize
      [[unlikely]];
      if (direction >= 0) // elide by right or center will elide by right
        return (binary ? ellipsis.left(maxsize) : ellipsis.utf8left(maxsize));
      return (binary ? ellipsis.right(maxsize) : ellipsis.utf8right(maxsize));
    }
    if (direction > 0) // elide by right, keep left
      return binary ? s.left(maxsize-es)+ellipsis
                    : s.utf8left(maxsize-es)+ellipsis;
    if (direction < 0) // elide by left, keep right
      return binary ? ellipsis+s.right(maxsize-es)
                    : ellipsis+s.utf8right(maxsize-es);
    // elide by center
    auto half = (maxsize-es)/2;
    return binary ? s.left(half)+ellipsis+s.right(maxsize-es-half)
                  : s.utf8left(half)+ellipsis+s.utf8right(maxsize-es-half);
  }
  [[nodiscard]] static inline Utf8String elide_right(
      const Utf8String &s, const qsizetype maxsize,
      const Utf8String &ellipsis = DefaultEllipsis, const bool binary = false) {
    return elide(1, binary, s, maxsize, ellipsis); }
  [[nodiscard]] inline Utf8String &elide_right(
      const qsizetype maxsize,
      const Utf8String &ellipsis = DefaultEllipsis, const bool binary = false) {
    return *this = elide(1, binary, *this, maxsize, ellipsis); }
  [[nodiscard]] static inline Utf8String elide_left(
      const Utf8String &s, const qsizetype maxsize,
      const Utf8String &ellipsis = DefaultEllipsis, const bool binary = false) {
    return elide(-1, binary, s, maxsize, ellipsis); }
  [[nodiscard]] inline Utf8String &elide_left(
      const qsizetype maxsize,
      const Utf8String &ellipsis = DefaultEllipsis, const bool binary = false) {
    return *this = elide(-1, binary, *this, maxsize, ellipsis); }
  [[nodiscard]] static inline Utf8String elide_middle(
      const Utf8String &s, const qsizetype maxsize,
      const Utf8String &ellipsis = DefaultEllipsis, const bool binary = false) {
    return elide(0, binary, s, maxsize, ellipsis); }
  [[nodiscard]] inline Utf8String &elide_middle(
      const qsizetype maxsize,
      const Utf8String &ellipsis = DefaultEllipsis, const bool binary = false) {
    return *this = elide(0, binary, *this, maxsize, ellipsis); }

  // padding
  /** Pad string to match at less size characters/bytes, adding a padding
   *  pattern (e.g. " " or "0") on one or both sides.
   *  e.g. pad(-1,false,"fo§",6,"+") -> "+++fo§"
   *  e.g. pad(-1,true,"fo§",6,"+") -> "++fo§" ("§" is 2 bytes long)
   *  e.g. pad(0,false,"hi!",7," ") -> "  hi!  "
   *  e.g. pad(0,false,"fo§",6,"12345") -> "1fo§23"
   *  You probably rather want to call specialized forms like pad_left().
   * @param direction: -1 left 0 center +1 right
   * @param binary true to count bytes, false to count unicode chars
   */
  [[nodiscard]] static inline Utf8String pad(
      const int direction, const bool binary, const Utf8String &s,
      const qsizetype size, const Utf8String &padding) {
    auto ss = binary ? s.size() : s.utf8size();
    if (ss >= size) // nothing to do
      return s;
    auto ps = binary ? padding.size() : padding.utf8size();
    if (ps == 0) // can't pad with empty padding pattern
      [[unlikely]] return s;
    Utf8String real_padding = padding;
    if (ps < size-ss) { // not enough padding, must repeat it
      real_padding = padding.repeated((size-ss)/ps + ((size-ss)%ps ? 1 : 0));
      ps = binary ? real_padding.size() : real_padding.utf8size();
    }
    if (ps > size-ss) // padding too long, keep left part
      real_padding = binary ? real_padding.left(size-ss)
                            : real_padding.utf8left(size-ss);
    if (direction == 1) // pad on right
      return s+real_padding;
    if (direction == -1) // pad on left
      return real_padding+s;
    // pad on center
    auto half = (size-ss)/2;
    if (binary)
      return real_padding.left(half)+s+real_padding.right(size-ss-half);
    return real_padding.utf8left(half)+s+real_padding.utf8right(size-ss-half);
  }
  [[nodiscard]] static inline Utf8String pad_left(
      const Utf8String &s, const qsizetype size,
      const Utf8String &padding = DefaultPadding, const bool binary = false) {
    return pad(-1, binary, s, size, padding); }
  [[nodiscard]] inline Utf8String &pad_left(
      const qsizetype size, const Utf8String &padding = DefaultPadding,
      const bool binary = false) {
    return *this = pad(-1, binary, *this, size, padding); }
  [[nodiscard]] static inline Utf8String pad_right(
      const Utf8String &s, const qsizetype size,
      const Utf8String &padding = DefaultPadding, const bool binary = false) {
    return pad(1, binary, s, size, padding); }
  [[nodiscard]] inline Utf8String &pad_right(
      const qsizetype size, const Utf8String &padding = DefaultPadding,
      const bool binary = false) {
    return *this = pad(1, binary, *this, size, padding); }
  [[nodiscard]] static inline Utf8String pad_center(
      const Utf8String &s, const qsizetype size,
      const Utf8String &padding = DefaultPadding, const bool binary = false) {
    return pad(0, binary, s, size, padding); }
  [[nodiscard]] inline Utf8String &pad_center(
      const qsizetype size, const Utf8String &padding = DefaultPadding,
      const bool binary = false) {
    return *this = pad(0, binary, *this, size, padding); }

  // splitting
  /** Splitting utf8 string on ascii 7 separators, e.g. {',',';'}
    * @see Utf8String::AsciiWhitespace */
  [[nodiscard]] Utf8StringList split_after(
      QList<char> seps, qsizetype offset = 0,
      Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Splitting utf8 string on ascii 7 separator, e.g. ' ' */
  [[nodiscard]] Utf8StringList split_after(
      const char sep, const qsizetype offset = 0,
      Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Splitting utf8 string on multi-byte (utf8) or multi-char separator,
   *  e.g. "-->", "🥨"_u8, U'🥨', "<≠>"_u8 */
  [[nodiscard]] Utf8StringList split_after(
      Utf8String sep, qsizetype offset = 0,
      Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Splitting utf8 string on ascii 7 separators, e.g. {',',';'}
    * @see Utf8String::AsciiWhitespace */
  [[nodiscard]] Utf8StringList split(
      QList<char> seps, Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Splitting utf8 string on ascii 7 separator, e.g. ' ' */
  [[nodiscard]] Utf8StringList split(
      const char sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Splitting utf8 string on multi-byte (utf8) or multi-char separator,
   *  e.g. "-->", "🥨"_u8, U'🥨', "<≠>"_u8 */
  [[nodiscard]] Utf8StringList split(
      Utf8String sep, Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;
  /** Split the string using its first utf8 char as a delimiter.
   *  e.g. "/foo/bar/g" -> { "foo", "bar", "g" }
   *  e.g. ",/,:,g" -> { "/", ":", "g" }
   *  e.g. "§foo§bar§g" -> { "foo", "bar", "g" }
   *  e.g. "越foo越bar越g" -> { "foo", "bar", "g" }
   *  e.g. "🥨foo🥨bar🥨g" -> { "foo", "bar", "g" }
   */
  [[nodiscard]] Utf8StringList split_headed_list(
      qsizetype offset = 0) const;
  [[deprecated("use split_headed_list instead")]]
  [[nodiscard]] Utf8StringList splitByLeadingChar(
      qsizetype offset = 0) const;
  /** Analyze text content as ^(whitespace*)key(whitespace+)value$
   *  e.g. "   name \t John Doe" -> { "name", "John Doe" }
   *  e.g. "name    " -> { "name", {} }
   *  e.g. "   name \t John Doe " -> { "name", "John Doe " }
   *  Key is trimmed on both sides, value is only trimmed on left. */
  [[nodiscard]] inline std::pair<Utf8String,Utf8String> split_as_pair() const {
    qsizetype len = size(), k, w, v;
    auto bytes = data();
    if (len == 0) // must be tested because (*this)[0] would segfault
      return {};
    for (k = 0; is_ascii_whitespace(bytes[k]); ++k) // key index
      if (k == len)
        return {};
    for (w = k+1; !is_ascii_whitespace(bytes[w]); ++w) // whitespace index
      if (w == len)
        return {mid(k), {}};
    for (v = w+1; is_ascii_whitespace(bytes[v]); ++v) // value index
      if (v == len)
        return {mid(k, w-k), {}};
    [[likely]] return {mid(k, w-k), mid(v)};
  }

  // conversions to numbers
  /** Converts to floating point, supporting e notation and SI suffixes from 'f'
   *  to 'P', 'u' is used as 1e-6 suffix e.g. ".1k" -> 100.0. */
  template<bool suffixes_enabled = true>
  [[nodiscard]] inline double toDouble(
      bool *ok = nullptr, double def = 0.0) const {
    return toFloating<double, suffixes_enabled>(
          *this, ok, def, &QByteArray::toDouble);
  }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true>
  [[nodiscard]] inline double toDouble(double def) const{
    return toDouble<suffixes_enabled>(nullptr, def); }
  /** Converts to floating point, supporting e notation and SI suffixes from 'f'
   *  to 'P', 'u' is used as 1e-6 suffix e.g. ".1k" -> 100.0. */
  template<bool suffixes_enabled = true>
  [[nodiscard]] inline float toFloat(
      bool *ok = nullptr, float def = 0.0) const {
    return toFloating<float, suffixes_enabled>(
          *this, ok, def, &QByteArray::toFloat);
  }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true>
  [[nodiscard]] inline float toFloat(float def) const{
    return toFloat<suffixes_enabled>(nullptr, def); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] qlonglong toLongLong(
      bool *ok = nullptr, int base = 0, qlonglong def = 0) const {
    return toIntegral<qlonglong, suffixes_enabled, floating_point_enabled>(
          *this, ok, base, def, &QByteArray::toLongLong);
  }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline qlonglong toLongLong(qlonglong def) const {
    return toLongLong<suffixes_enabled, floating_point_enabled>(
          nullptr, 0, def, true); }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline qlonglong toLongLong(long def) const {
    return toLongLong<suffixes_enabled, floating_point_enabled>(
          nullptr, 0, def, true); }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline qlonglong toLongLong(int def) const {
    return toLongLong<suffixes_enabled, floating_point_enabled>(
          nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] qulonglong toULongLong(
      bool *ok = nullptr, int base = 0, qulonglong def = 0) const {
    return toIntegral<qulonglong, suffixes_enabled, floating_point_enabled>(
          *this, ok, base, def, &QByteArray::toULongLong);
  }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline qulonglong toULongLong(qulonglong def) const {
    return toULongLong<suffixes_enabled, floating_point_enabled>(
          nullptr, 0, def, true); }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline qulonglong toULongLong(ulong def) const {
    return toULongLong<suffixes_enabled, floating_point_enabled>(
          nullptr, 0, def, true); }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline qulonglong toULongLong(uint def) const {
    return toULongLong<suffixes_enabled, floating_point_enabled>(
          nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] long toLong(
      bool *ok = nullptr, int base = 0, long def = 0) const {
    return toIntegral<long, suffixes_enabled, floating_point_enabled>(
          *this, ok, base, def, &QByteArray::toLong);
  }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline long toLong(long def) const {
    return toLong<suffixes_enabled, floating_point_enabled>(nullptr, 0, def, true); }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline long toLong(int def) const {
    return toLong<suffixes_enabled, floating_point_enabled>(nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] ulong toULong(
      bool *ok = nullptr, int base = 0, ulong def = 0) const {
    return toIntegral<ulong, suffixes_enabled, floating_point_enabled>(
          *this, ok, base, def, &QByteArray::toULong);
  }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline ulong toULong(ulong def) const {
    return toULong<suffixes_enabled, floating_point_enabled>(nullptr, 0, def, true); }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline ulong toULong(uint def) const {
    return toULong<suffixes_enabled, floating_point_enabled>(nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] int toInt(
      bool *ok = nullptr, int base = 0, int def = 0) const {
    return toIntegral<int, suffixes_enabled, floating_point_enabled>(
          *this, ok, base, def, &QByteArray::toInt);
  }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline int toInt(int def) const {
    return toInt<suffixes_enabled, floating_point_enabled>(
          nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] uint toUInt(
      bool *ok = nullptr, int base = 0, uint def = 0) const {
    return toIntegral<uint, suffixes_enabled, floating_point_enabled>(
          *this, ok, base, def, &QByteArray::toUInt);
  }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline uint toUInt(uint def) const {
    return toUInt<suffixes_enabled, floating_point_enabled>(
          nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] short toShort(
      bool *ok = nullptr, int base = 0, short def = 0) const {
    return toIntegral<short, suffixes_enabled, floating_point_enabled>(
          *this, ok, base, def, &QByteArray::toShort);
  }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline short toShort(short def) const {
    return toShort<suffixes_enabled, floating_point_enabled>(
          nullptr, 0, def, true); }
  /** Converts to integer, supporting both SI suffixes (from 'k' to 'P') and
   *  casual suffixes ('k', 'm', 'b') e.g. "1k" -> 1000.
   *  If base == 0 C prefixes are supported "0x" "0" and "0b" e.g "0xf" -> 15.
   *  If the string content matches a floating point value, return its
   *  integer part (if it fits the integer type) e.g. "1e3" -> 1000. */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] ushort toUShort(
      bool *ok = nullptr, int base = 0, ushort def = 0) const {
    return toIntegral<ushort, suffixes_enabled, floating_point_enabled>(
          *this, ok, base, def, &QByteArray::toUShort);
  }
  /** Syntaxic sugar */
  template<bool suffixes_enabled = true, bool floating_point_enabled = true>
  [[nodiscard]] inline ushort toUShort(ushort def) const {
    return toUShort<suffixes_enabled, floating_point_enabled>(
          nullptr, 0, def, true); }
  /** Converts to bool, supporting case insensitive "true" and "false", and any
   *  integer number, 0 being false and everything else true. */
  [[nodiscard]] bool toBool(
      bool *ok = nullptr, bool def = false) const;
  /** Syntaxic sugar */
  [[nodiscard]] inline bool toBool(bool def) const {
    return toBool(nullptr, def); }
  /** Converts to any number format, calling toXXX() methods above (and
   *  concerning integers, with base = 0 auto detection of base). */
#ifdef __cpp_concepts
  template <p6::arithmetic T, bool suffixes_enabled = true,
            bool floating_point_enabled = true>
#else
  template <typename T, bool suffixes_enabled = true,
            bool floating_point_enabled = true,
            typename = std::enable_if_t<std::is_arithmetic<T>::value>>
#endif
  [[nodiscard]] inline T toNumber(
      bool *ok = nullptr, const T &def = {}) const {
    return NumberConverter<T, suffixes_enabled, floating_point_enabled>()(
          *this, ok, def);
  }
  /** Convenience methods witout bool *ok. */
#ifdef __cpp_concepts
  template <p6::arithmetic T, bool suffixes_enabled = true,
            bool floating_point_enabled = true>
#else
  template <typename T, bool suffixes_enabled = true,
            bool floating_point_enabled = true,
            typename = std::enable_if_t<std::is_arithmetic<T>::value>>
#endif
  [[nodiscard]] inline T toNumber(
      const T &def) const {
    return NumberConverter<T, suffixes_enabled, floating_point_enabled>()(
          *this, nullptr, def);
  }

  // conversions from numbers
  [[nodiscard]] static inline Utf8String number(
      std::integral auto i, int base = 10) {
    return QByteArray::number(i, base); }
  [[nodiscard]] static inline Utf8String number(
      const void *p, int base = 16) {
    return QByteArray::number((qintptr)p, base); }
  /** format pointer as "QObject(0x0)" or "QObject(0xnnnn, \"objectname\")" */
  [[nodiscard]] static Utf8String number_and_name(const QObject *p);
  [[nodiscard]] static inline Utf8String number(
      double d, char format = 'g', int precision = 6) {
    return QByteArray::number(d, format, precision); }
  // we need a converter for float even though QByteArray doesn't have one,
  // because for instance QVariant(float) is defined and so operator=() or
  // constructor would otherwise have ambiguities when called on a float
  [[nodiscard]] static inline Utf8String number(
      float f, char format = 'g', int precision = 6) {
    return QByteArray::number((double)f, format, precision); }
  /** convert to "true" or "false". */
  [[nodiscard]] static inline Utf8String number(bool b);
  /** Write a number using a given arbitrary bijective base.
   *  0 is always "", 1 is the first char of the base etc.
   *  e.g. "ABCDEFGHIJKLMNOPQRSTUVWXYZ" is used for spreadsheets column notation
   *  and (with fewer letters) for european driving plate left and right parts.
   *  1 -> A, 26 -> Z, 27 -> AA, 16384 -> XFD
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

  // geometrics conversions
  [[nodiscard]] QPointF toPointF() const;
  [[nodiscard]] QSizeF toSizeF() const;
  [[nodiscard]] QRectF toRectF() const;
  [[nodiscard]] QLineF toLineF() const;
  [[nodiscard]] QList<QPointF> toPointFList() const;
  template <p6::arithmetic T>
  [[nodiscard]] inline QList<T> toNumberList() const {
    QList<T> numbers;
    for (qsizetype i = 0, j; ; i = j+1) {
      bool ok;
      j = indexOf(',', i);
      if (j == i)
        return {};
      if (j < 0)
        numbers += sliced(i).toNumber<T>(&ok);
      numbers += sliced(i, j-i-1).toNumber<T>(&ok);
      if (!ok)
        return {};
      if (j < 0)
        return numbers;
    }
  }

  // misc conversions
  /** Return list of contained bytes, sorted and deduplicated. */
  [[nodiscard]] QList<char> toBytesSortedList() const;

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

  inline Utf8String &append(auto t) { return operator+=(t); }
  inline Utf8String &append(auto t, auto u) {
    QByteArray::append(t, u); return *this; }

  inline Utf8String &prepend(auto t) {
    QByteArray::prepend(t); return *this; }
  inline Utf8String &prepend(auto t, auto u) {
    QByteArray::prepend(t, u); return *this; }
  inline Utf8String &prepend(char32_t u) {
    QByteArray::prepend(encode_utf8(u)); return *this; }
  inline Utf8String &prepend(char8_t c) {
    QByteArray::prepend((char)c); return *this; }

  inline Utf8String &insert(auto t, auto u) {
    QByteArray::insert(t, u); return *this; }
  inline Utf8String &insert(auto t, auto u, auto v) {
    QByteArray::insert(t, u, v); return *this; }

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
  inline Utf8String &remove(char32_t u) {
    return remove(encode_utf8(u)); }
  inline Utf8String &removeAt(qsizetype pos) {
    QByteArray::removeAt(pos); return *this;}
  Utf8String &remove_ascii_chars(QList<char> chars);
  inline Utf8String &removeFirst() { QByteArray::removeFirst(); return *this;}
  inline Utf8String &removeLast() { QByteArray::removeLast(); return *this;}

  /** Empty coalesce: replace with that if this is empty. */
  inline Utf8String &coalesce(auto that) {
    return isEmpty() ? operator=(that) : *this; }
  /** Empty coalesce: replace with Utf8String(t, u) if this is empty.
   *  e.g. coalesce("foobar", 2) */
  inline Utf8String &coalesce(auto t, auto u) {
    return isEmpty() ? operator=(Utf8String(t, u)) : *this; }
  /** Empty coalesce operator */
  inline Utf8String &operator|=(auto that) { return coalesce(that); }

  /** Null coalesce: replace with Utf8String(that) if this is null. */
  inline Utf8String &null_coalesce(auto that) {
    if (isNull()) *this = that;
    return *this; }
  /** Null coalesce: replace with ""_u8 if this is null. */
  inline Utf8String &null_coalesce();
  /** Null coalesce: replace with Utf8String(t, u) if this is null.
   *  e.g. null_coalesce("foobar", 2) */
  inline Utf8String &null_coalesce(auto t, auto u) {
    if (isNull()) *this = Utf8String(t, u);
    return *this; }
  /** Null coalesced: return Utf8String(that) if this is null. */
  [[nodiscard]] inline Utf8String null_coalesced(auto that) const {
    return isNull() ? Utf8String(that) : *this; }
  /** Null coalesced: return ""_u8 if this is null. */
  [[nodiscard]] inline Utf8String null_coalesced() const;
  /** Null coalesced: return Utf8String(t, u) if this is null.
   *  e.g. null_coalesced("foobar", 2) */
  [[nodiscard]] inline Utf8String null_coalesced(auto t, auto u) const {
    return isNull() ? Utf8String(t, u) : *this; }

  inline Utf8String &operator+=(const Utf8String &s) {
    QByteArray::operator+=(s); return *this; }
  // accept types for which QByteArray::operator+= is defined
  inline Utf8String &operator+=(const QByteArray &ba) {
    QByteArray::operator+=(ba); return *this; }
  inline Utf8String &operator+=(const char *s) {
    QByteArray::operator+=(s); return *this; }
  inline Utf8String &operator+=(char ch) {
    QByteArray::operator+=(ch); return *this; }
  // accept whatever is accepted by constructor, e.g. QLatin1StringView,
  // char32_t, const QString &, unsigned, double or const char8_t *
  // same: this is needed because since operator+=(QString) and
  // operator+=(QVariant) are defined
  inline Utf8String &operator+=(auto that) {
    QByteArray::operator+=(Utf8String(that)); return *this; }

  inline Utf8String &operator=(const Utf8String &other) {
    QByteArray::operator=(other); return *this; }
  QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(Utf8String)
  inline void swap(Utf8String &other) noexcept { QByteArray::swap(other); }
  // accept types for which QByteArray::operator= is defined
  inline Utf8String &operator=(const QByteArray &ba) {
    QByteArray::operator=(ba); return *this; }
  inline Utf8String &operator=(QByteArray &&ba) {
    QByteArray::operator=(ba); return *this; }
  inline Utf8String &operator=(const char *s) {
    QByteArray::operator=(s); return *this; }
  // accept whatever is accepted by constructor, e.g. QLatin1StringView,
  // char32_t, const QString &, unsigned, double or const char8_t *
  // same: this is needed because since operator=(QString) and
  // operator=(QVariant) are defined
  inline Utf8String &operator=(auto that) {
    QByteArray::operator=(Utf8String(that)); return *this; }
  // defining operator=(std::initializer_list) avoids ambiguity on expressions
  // like: "Utf8String s; s = {};"
  inline Utf8String &operator=(const std::initializer_list<char> list) {
    return list.size() ? operator=(QByteArray(list)) : operator=(Utf8String{});}

#if __cpp_impl_three_way_comparison >= 201711
  [[nodiscard]] static inline std::strong_ordering cmp(
      const char *x, qsizetype lenx, const char *y, qsizetype leny) {
    auto n = std::min(lenx, leny);
    for (int i = 0; i < n; ++i) {
      if (x[i] < y[i])
        return std::strong_ordering::less;
      if (x[i] > y[i])
        return std::strong_ordering::greater;
    }
    return lenx <=> leny;
  }
  [[nodiscard]] static inline std::strong_ordering cmp(
      const char *x, const char *y) {
    for (int i = 0; ; ++i) {
      if (x[i] == '\0' && y[i] == '\0')
        return std::strong_ordering::equivalent;
      if (x[i] < y[i])
        return std::strong_ordering::less;
      if (x[i] > y[i])
        return std::strong_ordering::greater;
    }
  }
  [[nodiscard]] inline int compare(const Utf8String &that,
              Qt::CaseSensitivity cs = Qt::CaseSensitive) const {
    auto c = cs == Qt::CaseSensitive ? *this <=> that
                                     : toUpper() <=> that.toUpper();
    if (c == std::strong_ordering::less)
      return -1;
    if (c == std::strong_ordering::greater)
      return 1;
    return 0;
  }

  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const Utf8String &x, const Utf8String &y) {
    return Utf8String::cmp(x.constData(), y.constData()); }
  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const Utf8String &x, const QByteArray &y) {
    return Utf8String::cmp(x.constData(), x.size(), y.constData(), y.size()); }
  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const QByteArray &x, const Utf8String &y) {
    return Utf8String::cmp(x.constData(), x.size(), y.constData(), y.size()); }
  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const Utf8String &x, const char *y) {
    return Utf8String::cmp(x.constData(), y); }
  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const char *x, const Utf8String &y) {
    return Utf8String::cmp(x, y.constData()); }
  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const Utf8String &x, const QString &y) {
    return Utf8String::cmp(x.constData(), y.toUtf8().constData()); }
  [[nodiscard]] friend inline std::strong_ordering operator<=>(
      const QString &x, const Utf8String &y) {
    return Utf8String::cmp(x.toUtf8().constData(), y.constData()); }

  [[nodiscard]] friend inline bool operator==(
      const Utf8String &x, const Utf8String &y) {
    return Utf8String::cmp(x.constData(), y.constData()) == 0; }
  [[nodiscard]] friend inline bool operator==(
      const Utf8String &x, const QByteArray &y) {
    return Utf8String::cmp(x.constData(), x.size(), y.constData(), y.size()) == 0; }
  [[nodiscard]] friend inline bool operator==(
      const QByteArray &x, const Utf8String &y) {
    return Utf8String::cmp(x.constData(), x.size(), y.constData(), y.size()) == 0; }
  [[nodiscard]] friend inline bool operator==(
      const Utf8String &x, const char *y) {
    return Utf8String::cmp(x.constData(), y) == 0; }
  [[nodiscard]] friend inline bool operator==(
      const char *x, const Utf8String &y) {
    return Utf8String::cmp(x, y.constData()) == 0; }
  [[nodiscard]] friend inline bool operator==(
      const Utf8String &x, const QString &y) {
    return Utf8String::cmp(x.constData(), y.toUtf8().constData()) == 0; }
  [[nodiscard]] friend inline bool operator==(
      const QString &x, const Utf8String &y) {
    return Utf8String::cmp(x.toUtf8().constData(), y.constData()) == 0; }
#else
  [[nodiscard]] static inline int cmp(
      const char *x, qsizetype lenx, const char *y, qsizetype leny) {
    auto n = std::min(lenx, leny);
    for (int i = 0; i < n; ++i) {
      if (x[i] > y[i])
        return 1;
      if (x[i] < y[i])
        return -1;
    }
    return lenx < leny ? -1 : lenx > leny ? 1 : 0;
  }
  [[nodiscard]] static inline int cmp(
      const char *x, const char *y) {
    for (int i = 0; ; ++i) {
      if (x[i] == '\0' && y[i] == '\0')
        return 0;
      if (x[i] < y[i])
        return -1;
      if (x[i] > y[i])
        return 1;
    }
  }
  [[nodiscard]] inline int compare(const Utf8String &that,
              Qt::CaseSensitivity cs = Qt::CaseSensitive) const {
    return cs == Qt::CaseSensitive
        ? cmp(this->constData(), that.constData())
        : cmp(toUpper().constData(), that.toUpper().constData());
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
      const char *escaped, qsizetype len = -1);
  [[nodiscard]] inline static Utf8String fromCEscaped(auto escaped) {
    Utf8String s(escaped);
    return fromCEscaped(s.constData(), s.size()); }
  /** C-Escape a string, e.g. replace \ \r \n \x19 " ' with backslashed
   *  sequences, leaving alone anything > 0x7f because it's likely to be
   *  part of a valid utf8 sequence.
   *  note that the behavior > 0x7f is different from cEscaped(char c).
   *  e.g. a'béz -> a\'béz which is at byte level "a\\'b\xc3\xa9z"
   */
  [[nodiscard]] Utf8String cEscaped() const;
  /** C-Escape a string, e.g. replace \ \r \n \x19 " ' and anything above
   *  0x7f with backslashed sequences, producing pure 7 bits ascii.
   *  e.g. a'béz -> a\'b\u00e9z which is at byte level "a\\'b\\u00e9z"
   */
  [[nodiscard]] Utf8String asciiCEscaped() const;
  /** C-Escape a char, e.g. replace \ \r \n \x19 \xa7 " ' with backslashed
   *  sequences.
   *  note that the behavior > 0x7f is different from cEscaped(void),
   *  because a lone byte > 0x7f can't be a valid utf8 sequence.
   */
  [[nodiscard]] static Utf8String cEscaped(char c);
  /** C-Escape a char, e.g. replace \ \r \n \x19 " ' and anything above
   *  0x7f with backslashed sequences, producing pure 7 bits ascii. */
  [[nodiscard]] static Utf8String asciiCEscaped(char32_t u);

  /** Test if u is an ascii whitespace char: ' ', '\t', '\n'... */
  [[nodiscard]] inline static bool is_ascii_whitespace(char32_t u) {
    switch (u) {
      // horizontal whitespace
      case ' ': case '\t':
      // line, page and section separators
      case '\n': case '\r': case '\v': case '\f':
        return true;
    }
    return false;
  }
  /** Test if u is an ascii horizontal whitespace: ' ', '\t' */
  [[nodiscard]] inline static bool is_ascii_horizontal_whitespace(char32_t u) {
    switch (u) {
      case ' ': case '\t':
        return true;
    }
    return false;
  }
  /** Test if u is a unicode whitespace char: ascii plus \u0084, \u2000... */
  [[nodiscard]] inline static bool is_unicode_whitespace(char32_t u) {
    switch (u) {
      // horizontal whitespace
      case ' ': case '\t': case 0xa0: case 0x1680:
      case 0x2000: case 0x2001: case 0x2002: case 0x2003: case 0x2004:
      case 0x2005: case 0x2006: case 0x2007: case 0x2008: case 0x2009:
      case 0x200a: case 0x202f: case 0x205f: case 0x3000:
      // line, page and section separators
      case '\n': case '\r': case '\v': case '\f':
      case 0x85: case 0x2028: case 0x2029:
        return true;
    }
    return false;
  }
  /** Test if u is a unicode whitespace char: ascii plus \u0084, \u2000... */
  [[nodiscard]] inline static bool is_unicode_horizontal_whitespace(char32_t u){
    switch (u) {
      case ' ': case '\t':
      case 0xa0: case 0x1680:
      case 0x2000: case 0x2001: case 0x2002: case 0x2003: case 0x2004:
      case 0x2005: case 0x2006: case 0x2007: case 0x2008: case 0x2009:
      case 0x200a: case 0x202f: case 0x205f: case 0x3000:
        return true;
    }
    return false;
  }

private:
  struct UnicodeCaseMapping {
    char32_t utf32;
    char32_t upper_utf32, lower_utf32, title_utf32;
    // LATER add already encoded utf8 here as optimization
    operator char32_t() const { return utf32; }
  };
  const static std::vector<UnicodeCaseMapping> _case_mapping;
  template<typename F, bool suffixes_enabled>
  [[nodiscard]] static inline F toFloating(
      QByteArray ba, bool *ok, F def,
      std::function<F(const QByteArray &,bool*)> wrapped);
  template<std::integral I, bool suffixes_enabled, bool floating_point_enabled>
  [[nodiscard]] static inline I toIntegral(
      QByteArray s, bool *ok, int base, I def,
      std::function<I(const QByteArray &,bool*,int)> wrapped);
#ifdef __cpp_concepts
  template <p6::arithmetic T, bool suffixes_enabled = true,
            bool floating_point_enabled = true>
#else
  template <typename T, bool suffixes_enabled = true,
            bool floating_point_enabled = true,
            typename = std::enable_if_t<std::is_arithmetic<T>::value>>
#endif
  struct NumberConverter {
    // this struct is needed because non-class non-variable partial
    // specialization is not allowed in C++ (at less until C++20)
    // see for instance https://stackoverflow.com/questions/8061456/why-can-i-seemingly-define-a-partial-specialization-for-function-templates
    [[nodiscard]] STATIC_LAMBDA inline T operator()(
        const Utf8String &s, bool *ok = nullptr, const T &def = {}) STATIC_LAMBDA_CONST;
  };
};

Q_DECLARE_METATYPE(Utf8String)
//TODO Q_DECLARE_SHARED(Utf8String)
//or at less: Q_DECLARE_TYPEINFO(Utf8String, Q_RELOCATABLE_TYPE);

inline Utf8String operator"" _u8(const char *str, size_t size) noexcept {
  return Utf8String(QByteArrayData(nullptr, const_cast<char *>(str),
                                   qsizetype(size)));
}

#if __cpp_char8_t >= 201811
inline Utf8String operator"" _u8(const char8_t *str, size_t size) noexcept {
  return Utf8String(QByteArrayData(nullptr, (char *)(str), qsizetype(size)));
}
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

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const Utf8String &s);

template <bool skip_bom>
const char *Utf8String::go_forward_to_utf8_char(
    const char **s, const char *end) {
  forever {
    auto c = reinterpret_cast<const unsigned char *>(*s);
    if (*s >= end)
      return 0; // end reached
    if ((c[0] & 0b1100'0000) == 0b1000'0000) { // skip continuation byte
      ++*s;
      [[unlikely]] continue;
    }
    if (skip_bom && c[0] == 0xef && *s+3 <= end && c[1] == 0xbb
        && c[2] == 0xbf) { // skip byte order mark
      *s += 3;
      [[unlikely]] continue;
    }
    return *s; // begin of valid sequence found
  }
}

template <bool skip_bom>
const char *Utf8String::go_backward_to_utf8_char(
    const char **s, const char *begin) {
  forever {
    auto c = reinterpret_cast<const unsigned char *>(*s);
    if (*s < begin)
      return 0; // begin overtaken
    if ((c[0] & 0b1100'0000) == 0b1000'0000) { // skip continuation byte
      --*s;
      [[unlikely]] continue;
    }
    if (skip_bom && c[0] == 0xbf && *s-3 >= begin && c[-1] == 0xbb
        && c[-2] == 0xef) { // skip byte order mark
      *s -= 3;
      [[unlikely]] continue;
    }
    return *s; // begin of valid sequence found
  }
}

template <bool strict>
char32_t Utf8String::decode_utf8_and_step_forward(
    const char **s, const char *end) {
  auto c = reinterpret_cast<const unsigned char *>(*s);
  if (c[0] < 0b1000'0000) { // ascii, 1 byte
    *s += 1;
    [[likely]] return c[0];
  }
  if (c[0] < 0b1100'0000) { // out of sequence continuation byte
    *s += 1;
    [[unlikely]] return ReplacementCharacter;
  }
  if (c[0] < 0b1110'0000) {
    *s += 2; // 2 bytes
    if (*s > end) { // incomplete sequence at end
      *s = end;
      [[unlikely]] return ReplacementCharacter;
    }
    char32_t u = (c[0] & 0b0001'1111) << 6 | (c[1] & 0b0011'1111);
    if (strict && u < 0x80) // overlong sequence
       [[unlikely]]return ReplacementCharacter;
    if (strict && (c[1] & 0b1100'0000) != 0b1000'0000) // bad continuation byte
      [[unlikely]] return ReplacementCharacter;
    return u;
  }
  if (c[0] < 0b1111'0000) {
    *s += 3; // 3 bytes
    if (*s > end) { // incomplete sequence at end
      *s = end;
      [[unlikely]] return ReplacementCharacter;
    }
    char32_t u = ((c[0] & 0b0000'1111) << 12) | ((c[1] & 0b0011'1111) << 6)
        | (c[2] & 0b0011'1111);
    if (u >= 0xd800 && u <= 0xdfff) // RFC 3629: surrogate halves are invalid
      [[unlikely]] return ReplacementCharacter;
    if (strict && u < 0x800) // overlong sequence
      [[unlikely]] return ReplacementCharacter;
    if (strict && ((c[1] & 0b1100'0000) != 0b1000'0000 // bad continuation byte
                   || (c[2] & 0b1100'0000) != 0b1000'0000))
      [[unlikely]] return ReplacementCharacter;
    return u;
  }
  *s += 4; // 4 bytes
  if (*s > end) { // incomplete sequence at end
    *s = end;
    [[unlikely]] return ReplacementCharacter;
  }
  // note: first byte mask intentionnaly keeps 0b0000'1000 bit so that a 5+ bytes
  // sequence will be >= 0x10'ffff and discarded without specific test
  char32_t u = ((c[0] & 0b0000'1111) << 18) | ((c[1] & 0b0011'1111) << 12)
      | ((c[2] & 0b0011'1111) << 6) | (c[3] & 0b0011'1111);
  if (u >= 0x10'ffff) // RFC 3629: > 0x10ffff are invalid
    [[unlikely]] return ReplacementCharacter;
  if (strict && u < 0x10000) // overlong sequence
    [[unlikely]] return ReplacementCharacter;
  if (strict && ((c[1] & 0b1100'0000) != 0b1000'0000 // bad continuation byte
                 || (c[2] & 0b1100'0000) != 0b1000'0000
                 || (c[3] & 0b1100'0000) != 0b1000'0000))
    [[unlikely]] return ReplacementCharacter;
  return u;
}

void Utf8String::encode_utf8_and_step_forward(char **d, char32_t u) {
  auto c = reinterpret_cast<unsigned char *>(*d);
  if (u < 0x80) { // ascii, 1 byte
    c[0] = u;
    *d += 1;
    [[likely]] return;
  }
  if (u < 0x800) {
    c[0] = 0b1100'0000 + (u >> 6);
    c[1] = 0b1000'0000 + (u & 0b11'1111);
    *d += 2;
    return;
  }
  if (u >= 0xd800 && u <= 0xdfff) // RFC 3629: surrogate halves are invalid
    [[unlikely]] return;
  if (u < 0x10000) {
    c[0] = 0b1110'0000 + (u >> 12);
    c[1] = 0b1000'0000 + ((u >> 6) & 0b11'1111);
    c[2] = 0b1000'0000 + (u & 0b11'1111);
    *d += 3;
    return;
  }
  if (u > 0x10'ffff) // RFC 3629: > 0x10ffff are invalid
    [[unlikely]] return;
  c[0] = 0b1111'0000 + (u >> 18);
  c[1] = 0b1000'0000 + ((u >> 12) & 0b11'1111);
  c[2] = 0b1000'0000 + ((u >> 6) & 0b11'1111);
  c[3] = 0b1000'0000 + (u & 0b11'1111);
  *d += 4;
}

Utf8String Utf8String::encode_utf8(char32_t u) {
  if (u < 0x80) { // ascii, 1 byte
    unsigned char c = u;
    [[likely]] return Utf8String(&c, 1);
  }
  if (u < 0x800) {
    unsigned char s[2];
    s[0] = 0b1100'0000 + (u >> 6);
    s[1] = 0b1000'0000 + (u & 0b11'1111);
    return Utf8String(s, sizeof s);
  }
  if (u >= 0xd800 && u <= 0xdfff) // RFC 3629: surrogate halves are invalid
    [[unlikely]] return ReplacementCharacterUtf8;
  if (u < 0x10000) {
    unsigned char s[3];
    s[0] = 0b1110'0000 + (u >> 12);
    s[1] = 0b1000'0000 + ((u >> 6) & 0b11'1111);
    s[2] = 0b1000'0000 + (u & 0b11'1111);
    return Utf8String(s, sizeof s);
  }
  if (u > 0x10'ffff) // RFC 3629: > 0x10ffff are invalid
    [[unlikely]] return ReplacementCharacterUtf8;
  unsigned char s[4];
  s[0] = 0b1111'0000 + (u >> 18);
  s[1] = 0b1000'0000 + ((u >> 12) & 0b11'1111);
  s[2] = 0b1000'0000 + ((u >> 6) & 0b11'1111);
  s[3] = 0b1000'0000 + (u & 0b11'1111);
  return Utf8String(s, sizeof s);
}

template <bool strict, bool keep_replacement_chars, bool keep_bom>
void Utf8String::clean(char *s, const char *end) {
  char *d = s; // destination
  while (s < end) {
    auto c = decode_utf8_and_step_forward<strict>(
               const_cast<const char**>(&s), end);
    if (!keep_replacement_chars && c == ReplacementCharacter)
      [[unlikely]] continue;
    if (!keep_bom && c == ByteOrderMark)
      [[unlikely]] continue;
    encode_utf8_and_step_forward(&d, c);
  }
}

template <bool strict, bool keep_replacement_chars, bool keep_bom>
Utf8String &Utf8String::clean() {
  char *s = data(), *d = s, *begin = s, *end = s+size();
  while (s < end) {
    auto c = decode_utf8_and_step_forward<strict>(
               const_cast<const char**>(&s), end);
    if (!keep_replacement_chars && c == ReplacementCharacter)
      [[unlikely]] continue;
    if (!keep_bom && c == ByteOrderMark)
      [[unlikely]] continue;
    encode_utf8_and_step_forward(&d, c);
  }
  truncate(d-begin);
  return *this;
}

template <bool strict, bool keep_replacement_chars, bool keep_bom>
Utf8String Utf8String::cleaned(const char *s, const char *end) {
  QByteArray cleaned(end-s, Qt::Uninitialized);
  char *d = cleaned.data(), *begin = d; // destination
  while (s < end) {
    auto c = decode_utf8_and_step_forward<strict>(&s, end);
    if (!keep_replacement_chars && c == ReplacementCharacter)
      [[unlikely]] continue;
    if (!keep_bom && c == ByteOrderMark)
      [[unlikely]] continue;
    encode_utf8_and_step_forward(&d, c);
  }
  cleaned.truncate(d-begin);
  return cleaned;
}

Utf8String Utf8String::utf8value(
    qsizetype i, const char *s, const char *end, const Utf8String &def) {
  for (qsizetype j = 0; j < i && go_forward_to_utf8_char(&s, end); ++j, ++s)
    ;
  return s < end ? Utf8String(s, end-s) : def;
}

Utf8String Utf8String::utf8left(qsizetype len) const {
  auto s = constData();
  auto end = s + size();
  auto begin = go_forward_to_utf8_char(&s, end);
  for (qsizetype i = 0; i < len && go_forward_to_utf8_char(&s, end); ++s, ++i)
    ;
  return Utf8String(begin, s-begin);
}

Utf8String Utf8String::utf8right(qsizetype len) const {
  auto begin = constData();
  auto s = begin + size(), end = s;
  for (qsizetype i = 0; i < len && go_backward_to_utf8_char(&--s, begin); ++i)
    ;
  if (s < begin)
    s = begin;
  return Utf8String(s, end-s);
}

Utf8String Utf8String::utf8mid(qsizetype pos, qsizetype len) const {
  auto s = constData(), begin = s;
  auto end = s + size();
  for (qsizetype i = 0; i < pos && go_forward_to_utf8_char(&s, end); ++s, ++i)
    ;
  if (len < 0)
    return Utf8String(s, size()-(s-begin));
  begin = s;
  for (qsizetype i = 0; i < len && go_forward_to_utf8_char(&s, end); ++s, ++i)
    ;
  return Utf8String(begin, s-begin);
}

void Utf8String::utf8chop(qsizetype len) {
  auto begin = constData();
  auto s = begin + size();
  for (qsizetype i = 0; i < len && go_backward_to_utf8_char(&--s, begin); ++i)
    ;
  if (s >= begin)
    truncate(s-begin);
}

Utf8String Utf8String::utf8chopped(qsizetype len) const {
  Utf8String s = *this;
  s.utf8chop(len);
  return s;
}

qsizetype Utf8String::utf8size() const {
  auto s = constData();
  auto end = s + size();
  qsizetype i = 0;
  for (; go_forward_to_utf8_char(&s, end); ++s)
    ++i;
  return i;
}

char32_t Utf8String::toUpper(char32_t u) {
  auto cm = std::lower_bound(_case_mapping.cbegin(), _case_mapping.cend(), u);
  return cm == _case_mapping.end() || cm->utf32 != u ? u : cm->upper_utf32;
}

char32_t Utf8String::toLower(char32_t u) {
  auto cm = std::lower_bound(_case_mapping.cbegin(), _case_mapping.cend(), u);
  return cm == _case_mapping.end() || cm->utf32 != u ? u : cm->lower_utf32;
}

char32_t Utf8String::toTitle(char32_t u) {
  auto cm = std::lower_bound(_case_mapping.cbegin(), _case_mapping.cend(), u);
  return cm == _case_mapping.end() || cm->utf32 != u ? u : cm->title_utf32;
}

[[nodiscard]] inline Utf8String Utf8String::number(bool b) {
  return b ? "true"_u8: "false"_u8;
}

inline Utf8String &Utf8String::null_coalesce() {
  return null_coalesce(""_u8);
}

[[nodiscard]] inline Utf8String Utf8String::null_coalesced() const {
  return null_coalesced(""_u8);
}

template<bool suffixes_enabled, bool floating_point_enabled>
struct Utf8String::NumberConverter<double, suffixes_enabled, floating_point_enabled> {
  STATIC_LAMBDA double operator()(
      const Utf8String &s, bool *ok, const double &def) STATIC_LAMBDA_CONST {
    return s.toDouble<suffixes_enabled>(ok, def);
  }
};

template<bool suffixes_enabled, bool floating_point_enabled>
struct Utf8String::NumberConverter<float, suffixes_enabled, floating_point_enabled> {
  STATIC_LAMBDA float operator()(
      const Utf8String &s, bool *ok, const float &def) STATIC_LAMBDA_CONST {
    return s.toFloat<suffixes_enabled>(ok, def);
  }
};

template<bool suffixes_enabled, bool floating_point_enabled>
struct Utf8String::NumberConverter<qlonglong, suffixes_enabled, floating_point_enabled> {
  STATIC_LAMBDA qlonglong operator()(
      const Utf8String &s, bool *ok, const qlonglong &def) STATIC_LAMBDA_CONST {
    return s.toLongLong<suffixes_enabled, floating_point_enabled>(ok, 0, def);
  }
};

template<bool suffixes_enabled, bool floating_point_enabled>
struct Utf8String::NumberConverter<qulonglong, suffixes_enabled, floating_point_enabled> {
  STATIC_LAMBDA qulonglong operator()(
      const Utf8String &s, bool *ok, const qulonglong &def) STATIC_LAMBDA_CONST {
    return s.toULongLong<suffixes_enabled, floating_point_enabled>(ok, 0, def);
  }
};

template<bool suffixes_enabled, bool floating_point_enabled>
struct Utf8String::NumberConverter<long, suffixes_enabled, floating_point_enabled> {
  STATIC_LAMBDA long operator()(
      const Utf8String &s, bool *ok, const long &def) STATIC_LAMBDA_CONST {
    return s.toLong<suffixes_enabled, floating_point_enabled>(ok, 0, def);
  }
};

template<bool suffixes_enabled, bool floating_point_enabled>
struct Utf8String::NumberConverter<ulong, suffixes_enabled, floating_point_enabled> {
  STATIC_LAMBDA ulong operator()(
      const Utf8String &s, bool *ok, const ulong &def) STATIC_LAMBDA_CONST {
    return s.toULong<suffixes_enabled, floating_point_enabled>(ok, 0, def);
  }
};

template<bool suffixes_enabled, bool floating_point_enabled>
struct Utf8String::NumberConverter<int, suffixes_enabled, floating_point_enabled> {
  STATIC_LAMBDA int operator()(
      const Utf8String &s, bool *ok, const int &def) STATIC_LAMBDA_CONST {
    return s.toInt<suffixes_enabled, floating_point_enabled>(ok, 0, def);
  }
};

template<bool suffixes_enabled, bool floating_point_enabled>
struct Utf8String::NumberConverter<uint, suffixes_enabled, floating_point_enabled> {
  STATIC_LAMBDA uint operator()(
      const Utf8String &s, bool *ok, const uint &def) STATIC_LAMBDA_CONST {
    return s.toUInt<suffixes_enabled, floating_point_enabled>(ok, 0, def);
  }
};


template<bool suffixes_enabled, bool floating_point_enabled>
struct Utf8String::NumberConverter<short, suffixes_enabled, floating_point_enabled> {
  STATIC_LAMBDA short operator()(
      const Utf8String &s, bool *ok, const short &def) STATIC_LAMBDA_CONST {
    return s.toShort<suffixes_enabled, floating_point_enabled>(ok, 0, def);
  }
};

template<bool suffixes_enabled, bool floating_point_enabled>
struct Utf8String::NumberConverter<ushort, suffixes_enabled, floating_point_enabled> {
  STATIC_LAMBDA ushort operator()(
      const Utf8String &s, bool *ok, const ushort &def) STATIC_LAMBDA_CONST {
    return s.toUShort<suffixes_enabled, floating_point_enabled>(ok, 0, def);
  }
};

template<bool suffixes_enabled, bool floating_point_enabled>
struct Utf8String::NumberConverter<bool, suffixes_enabled, floating_point_enabled> {
  STATIC_LAMBDA bool operator()(
      const Utf8String &s, bool *ok, const bool &def) STATIC_LAMBDA_CONST {
    return s.toBool(ok, def);
  }
};

// TODO turn wrapped into template arg
template<typename F, bool suffixes_enabled>
inline F Utf8String::toFloating(
    QByteArray ba, bool *ok, F def,
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
        [[likely]] goto no_suffix;
    }
    s = s.toUpper();
    if (s.contains("NAN") || s.contains("INF")) {
      // avoid hiding nan or inf with nano or femto would-be suffix
      mul = 1.0;
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

// TODO turn wrapped into template arg
template<std::integral I, bool suffixes_enabled, bool floating_point_enabled>
inline I Utf8String::toIntegral(
    QByteArray s, bool *ok, int base, I def,
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
    double d = toFloating<double, suffixes_enabled>(
          s, &_ok, (double)NAN, &QByteArray::toDouble);
    if(_ok && p6::double_fits_in_integral_type<I>(d))
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

#endif // UTF8STRING_H
