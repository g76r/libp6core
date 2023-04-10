/* Copyright 2016-2023 Hallowyn, Gregoire Barbier and others.
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

#include "libp6core_global.h"
#include <QStringList>

using namespace Qt::Literals::StringLiterals;

class LIBP6CORESHARED_EXPORT StringUtils {
  StringUtils() = delete;

public:
  /** Ellide a string if needed, keeping its left part.
   * ("foobar",5,"...") -> "fo..."
   * Return string as is if maxsize < 0.
   * Return a subset of placeholder if maxsize < placeholder.size().
   */ 
  static QString elideRight(const QString &string, int maxsize,
                            const QString &placeholder = u"..."_s) {
    return elide<+1>(string, maxsize, placeholder); }
  /** Ellide a string if needed, keeping its right part.
   * ("foobar",5,"...") -> "...ar"
   * Return string as is if maxsize < 0.
   * Return a subset of placeholder if maxsize < placeholder.size().
   */
  static QString elideLeft(const QString &string, int maxsize,
                            const QString &placeholder = u"..."_s) {
    return elide<-1>(string, maxsize, placeholder); }
  /** Ellide a string if needed, removing the middle part.
   * ("foobar",5,"...") -> "f...r"
   * Return string as is if maxsize < 0.
   * Return a subset of placeholder if maxsize < placeholder.size().
   */
  static QString elideMiddle(const QString &string, int maxsize,
                            const QString &placeholder = u"..."_s) {
    return elide<0>(string, maxsize, placeholder); }
  /** Return a column as a QStringList from QList<QStringList> list of rows.
   * Kind of extracting a vector from a transposed text matrix.
   */
  static QStringList columnFromRows(QList<QStringList> rows, int column,
                                    QString defaultValue = QString()) {
    QStringList list;
    foreach (const QStringList &row, rows)
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
  static QString htmlEncode(QString text, bool urlAsLinks = true,
                            bool newlineAsBr = true);
  /** Convert an identifier to snake case.
   * E.g. "hello world" -> "hello_world", "HelloWorld" -> "hello_world",
   * "hello_world" -> "hello_world", "hello-World" -> "hello_world".
   */
  static QByteArray toSnakeCase(const QByteArray &anycase);
  /** Convert an identifier to snake case.
   * E.g. "hello world" -> "hello_world", "HelloWorld" -> "hello_world",
   * "hello_world" -> "hello_world", "hello-World" -> "hello_world".
   */
  static QByteArray toSnakeCase(const char *anycase) {
      return toSnakeCase(QByteArray(anycase)); }
  /** Convert an identifier to snake case.
   * E.g. "hello world" -> "hello_world", "HelloWorld" -> "hello_world",
   * "hello_world" -> "hello_world", "hello-World" -> "hello_world".
   */
  static QString toSnakeCase(const QString &anycase);
  /** Convert an identifier to kebab + upper camel case.
   * E.g. "hello-world" -> "Hello-World", "HelloWorld" -> "Helloworld",
   * "hello_world" -> "Hello_world", "Hello-world" -> "Hello-World".
   */
  static QByteArray toSnakeUpperCamelCase(const QByteArray &anycase);
  /** Convert an identifier to kebab + upper camel case.
   * E.g. "hello-world" -> "Hello-World", "HelloWorld" -> "Helloworld",
   * "hello_world" -> "Hello_world", "Hello-world" -> "Hello-World".
   */
  static QByteArray toSnakeUpperCamelCase(const char *anycase) {
      return toSnakeUpperCamelCase(QByteArray(anycase)); }
  /** Convert an identifier to kebab + upper camel case.
   * E.g. "hello-world" -> "Hello-World", "HelloWorld" -> "Helloworld",
   * "hello_world" -> "Hello_world", "Hello-world" -> "Hello-World".
   */
  static QString toSnakeUpperCamelCase(const QString &anycase);
  /** Convert an identifier to kebab + upper camel case, converting only ASCII-7
   * chars, like internet header names.
   * E.g. "hello-world" -> "Hello-World", "HelloWorld" -> "Helloworld",
   * "hello_world" -> "Hello_world", "Hello-world" -> "Hello-World".
   */
  static QByteArray toAsciiSnakeUpperCamelCase(const QByteArray &anycase);

private:
  template <int DIR> // DIR: -1 left 0 middle +1 right
  static inline QString elide(
      const QString &string, int maxsize, const QString &placeholder) {
    if (maxsize < 0 || string.size() < maxsize)
      return string;
    if (placeholder.size() > maxsize)
      return DIR >= 0 ? placeholder.left(maxsize) : placeholder.right(maxsize);
    if (DIR > 0)
      return string.left(maxsize-placeholder.size())+placeholder;
    if (DIR < 0)
      return placeholder+string.right(maxsize-placeholder.size());
    return string.left(maxsize/2-placeholder.size())+placeholder
        +string.right(maxsize-maxsize/2);
  }
};

#endif // STRINGUTILS_H

