/* Copyright 2016-2024 Hallowyn, Gregoire Barbier and others.
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
#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include "util/utf8string.h"

using namespace Qt::Literals::StringLiterals;

namespace StringUtils {

/** Ellide a string if needed, keeping its left part.
   * ("foobar",5,"...") -> "fo..."
   * Return string as is if maxsize < 0.
   * Return a subset of placeholder if maxsize < placeholder.size().
   */
QString elideRight(const QString &string, int maxsize,
                   const QString &placeholder = u"..."_s);
/** Ellide a string if needed, keeping its right part.
   * ("foobar",5,"...") -> "...ar"
   * Return string as is if maxsize < 0.
   * Return a subset of placeholder if maxsize < placeholder.size().
   */
QString elideLeft(const QString &string, int maxsize,
                  const QString &placeholder = u"..."_s);
/** Ellide a string if needed, removing the middle part.
   * ("foobar",5,"...") -> "f...r"
   * Return string as is if maxsize < 0.
   * Return a subset of placeholder if maxsize < placeholder.size().
   */
QString elideMiddle(const QString &string, int maxsize,
                    const QString &placeholder = u"..."_s);
/** Return a column as a QStringList from QList<QStringList> list of rows.
   * Kind of extracting a vector from a transposed text matrix.
   */
inline QStringList columnFromRows(QList<QStringList> rows, int column,
                                  QString defaultValue = {}) {
  QStringList list;
  for (auto row: rows)
    list << row.value(column, defaultValue);
  return list;
}
/** Encode raw text to make it includable in an html document.
   * Special chars (such as '<') are replaced with entities.
   * Non ascii (> 127) chars are left unchanged (i.e. returned QString is still
   * unicode and must be converted to suited encoding before written to a
   * document, e.g. using QString::toUtf8()).
   * @urlAsLinks if true strings like "http://foo/bar" will be converted into html links
   */
QString htmlEncode(QString text, bool urlAsLinks = true,
                   bool newlineAsBr = true);
/** Convert an identifier to snake case.
   * E.g. "hello world" -> "hello_world", "HelloWorld" -> "hello_world",
   * "hello_world" -> "hello_world", "hello-World" -> "hello_world".
   */
Utf8String toSnakeCase(const Utf8String &anycase);
/** Convert an identifier to snake case.
   * E.g. "hello world" -> "hello_world", "HelloWorld" -> "hello_world",
   * "hello_world" -> "hello_world", "hello-World" -> "hello_world".
   */
inline Utf8String toSnakeCase(const char *anycase) {
  return toSnakeCase(Utf8String(anycase)); }
/** Convert an identifier to snake case.
   * E.g. "hello world" -> "hello_world", "HelloWorld" -> "hello_world",
   * "hello_world" -> "hello_world", "hello-World" -> "hello_world".
   */
QString toSnakeCase(const QString &anycase);

} // namespace

#endif // STRINGUTILS_H

