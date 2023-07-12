/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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

#ifndef PFUTILS_H
#define PFUTILS_H

#include "libp6core_global.h"
#include <QString>
#include "pfoptions.h"

class LIBP6CORESHARED_EXPORT PfUtils {
  Q_DISABLE_COPY(PfUtils)
public:
  /** Return a string with all PF special chars escaped excepted single spaces
    * in the middle of the string, e.g.
    * foo 'bar      ->      foo \'bar
    *  foo  bar     ->      \ foo\  bar
    * foo\\bar      ->      foo\\\\bar
    * "foo"(|       ->      \"foo\"\(\|
    *
    * Set escapeEvenSingleSpaces to true to escape every spaces, which is useful
    * e.g. for node names containing spaces.
    */
  static QString escape(
      const QString &string, const PfOptions &options = PfOptions(),
      bool escapeEvenSingleSpaces = false);
  /** @return integer value if the string content is a valid integer
   * C-like prefixes are supported and both kmb and kMGTP suffixes are supported
   * surrounding whitespace is trimmed
   * e.g. 0x1f means 15, 12k means 12000, 12b and 12G mean 12000000000.
   * T and P are supported with long long, not int. */
  static qlonglong stringAsLongLong(QString key, qlonglong defaultValue = 0,
                                    bool *ok = 0);
  static int stringAsInt(QString s, int defaultValue = 0, bool *ok = 0);
  /** @return bool value if the string content is a valid boolean
   * "false" "true" and valid C-like integer are supported e.g. 0 or 0xf */
  static bool stringAsBool(
    QStringView s, bool defaultValue = false, bool *ok = 0);
  /** @return bool value if the string content is a valid C-like double
   * (including e notation). */
  static double stringAsDouble(QStringView s, double defaultValue = 0.0,
                               bool *ok = 0);
  /** Split a string on any ascii whitespace (space, \r, etc.), but for escaped
   * (with backslash) whitespace.
   * Leading and trailing spaces are ignored, multiple whitespace characters are
   * processed as one (there are no empty parts).
   */
  static const QStringList stringSplittedOnWhitespace(QStringView s);
  /** Split string into two strings on first non-leading whitespace.
   * e.g. "foo bar baz" and "    foo  bar baz" are both interpreted as the same
   * 2 items list: { "foo", "bar baz" }.
   * List may contain only 1 or 0 element, depending on node content.
   * Whitespace cannot be escaped. */
  static const QStringList stringSplittedOnFirstWhitespace(QStringView s);
};

#endif // PFUTILS_H
