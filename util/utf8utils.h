/* Copyright 2025 Gregoire Barbier and others.
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
#ifndef UTF8UTILS_H
#define UTF8UTILS_H

#include "util/utf8string.h"
#include <QIODevice>

namespace p6 {

/** Read one or several bytes from input until they constitute a valid utf8
 *  character or an invalid utf8 bytes sequence.
 *  @return {bytes_read, unicode_char}
 *    or {-1, Utf8String::ReplacementCharacter} */
inline std::pair<qint64,char32_t> get_utf8(
    QIODevice *input, Utf8String *error = nullptr, bool strict = true,
    bool skip_bom = true) {
  char buf[4];
  unsigned char *c = reinterpret_cast<unsigned char *>(buf);
first_byte:
  if (auto r = input->read(buf, 1); r < 0) [[unlikely]] {
    if (error)
      *error = input->errorString();
    return {-1, Utf8String::ReplacementCharacter};
  } else if (r == 0) {
    return {0, 0};
  }
  if (c[0] < 0b1000'0000) [[likely]]
    return {1, c[0]}; // ascii, 1 byte
  if (c[0] < 0b1100'0000) [[unlikely]] {
    if (error)
      *error = "invalid first byte "_u8+c[0];
    return {-1, Utf8String::ReplacementCharacter}; // out of sequence continuation byte
  }
  if (c[0] < 0b1110'0000) {
    if (input->read(buf+1, 1) != 1) [[unlikely]] {
      if (error)
        *error = input->errorString();
      return {-1, Utf8String::ReplacementCharacter};
    }
    char32_t u = (c[0] & 0b0001'1111) << 6 | (c[1] & 0b0011'1111);
    if (strict && u < 0x80) [[unlikely]]
      return {-1, Utf8String::ReplacementCharacter}; // overlong sequence
    if (strict && (c[1] & 0b1100'0000) != 0b1000'0000) [[unlikely]]
      return {-1, Utf8String::ReplacementCharacter}; // invalid continuation byte
    return {2, u};
  }
  if (c[0] < 0b1111'0000) {
    if (input->read(buf+1, 2) != 2) [[unlikely]]
      return {-1, Utf8String::ReplacementCharacter};
    char32_t u = ((c[0] & 0b0000'1111) << 12) | ((c[1] & 0b0011'1111) << 6)
        | (c[2] & 0b0011'1111);
    if (u >= 0xd800 && u <= 0xdfff) [[unlikely]]
      return {-1, Utf8String::ReplacementCharacter}; // RFC 3629: surrogate halves are invalid
    if (strict && u < 0x800) [[unlikely]]
      return {-1, Utf8String::ReplacementCharacter}; // overlong sequence
    if (strict && ((c[1] & 0b1100'0000) != 0b1000'0000
                   || (c[2] & 0b1100'0000) != 0b1000'0000)) [[unlikely]]
      return {-1, Utf8String::ReplacementCharacter}; // invalid continuation byte
    if (skip_bom && u == Utf8String::ByteOrderMark) [[unlikely]]
      goto first_byte; // byte order mark found -> read next character instead
    return {3, u};
  }
  if (input->read(buf+1, 3) != 3) [[unlikely]]
    return {-1, Utf8String::ReplacementCharacter};
  // note: first byte mask intentionnaly keeps 0b0000'1000 bit so that a 5+ bytes
  // sequence will be >= 0x10'ffff and discarded without specific test
  char32_t u = ((c[0] & 0b0000'1111) << 18) | ((c[1] & 0b0011'1111) << 12)
      | ((c[2] & 0b0011'1111) << 6) | (c[3] & 0b0011'1111);
  if (u >= 0x10'ffff) [[unlikely]]
    return {-1, Utf8String::ReplacementCharacter};  // RFC 3629: > 0x10ffff are invalid
  if (strict && u < 0x10000) [[unlikely]]
    return {-1, Utf8String::ReplacementCharacter}; // overlong sequence
  if (strict && ((c[1] & 0b1100'0000) != 0b1000'0000
                 || (c[2] & 0b1100'0000) != 0b1000'0000
                 || (c[3] & 0b1100'0000) != 0b1000'0000)) [[unlikely]]
    return {-1, Utf8String::ReplacementCharacter}; // invalid continuation byte
  return {4, u};
}

/** Read one or several bytes from input until they constitute a valid utf8
 *  character or an invalid utf8 bytes sequence.
 *  @param buf will be set to 1 char string or to {}
 *  @return bytes_read or -1 */
inline qint64 read_utf8(QIODevice *input, Utf8String *buf, bool strict = true) {
  auto [r, u] = get_utf8(input, nullptr, strict);
  *buf = r >= 0 ? Utf8String{u} : Utf8String{};
  return r;
}

/** Read one or several bytes from input until they constitute a valid utf8
 *  character or an invalid utf8 bytes sequence.
 *  @param c unicode character read or Utf8String::ReplacementCharacter
 *  @return bytes_read or -1 */
inline qint64 read_utf8(QIODevice *input, char32_t *c, bool strict = true) {
  auto [r, u] = get_utf8(input, nullptr, strict);
  *c = u;
  return r;
}

} // namespace p6

#endif // UTF8UTILS_H
