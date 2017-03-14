/* Copyright 2016-2017 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include "libqtssu_global.h"
#include <QStringList>

class LIBQTSSUSHARED_EXPORT StringUtils {
  StringUtils() = delete;

public:
  /** Equals to "..." */
  static const QString _ellipsis;
  /** Ellide a string if needed, keeping its left part.
   * ("foobar",5,"...") -> "fo..."
   * Return string as is if maxsize < 0.
   * Return a subset of placeholder if maxsize < placeholder.size().
   */
  static QString elideRight(const QString &string, int maxsize,
                            const QString &placeholder = _ellipsis) {
    if (maxsize < 0 || string.size() < maxsize)
      return string;
    if (placeholder.size() > maxsize)
      return placeholder.left(maxsize);
    return string.left(maxsize-placeholder.size())+placeholder;
  }
  /** Ellide a string if needed, keeping its right part.
   * ("foobar",5,"...") -> "...ar"
   * Return string as is if maxsize < 0.
   * Return a subset of placeholder if maxsize < placeholder.size().
   */
  static QString elideLeft(const QString &string, int maxsize,
                            const QString &placeholder = _ellipsis) {
    if (maxsize < 0 || string.size() < maxsize)
      return string;
    if (placeholder.size() > maxsize)
      return placeholder.right(maxsize);
    return placeholder+string.right(maxsize-placeholder.size());
  }
  /** Ellide a string if needed, removing the middle part.
   * ("foobar",5,"...") -> "f...r"
   * Return string as is if maxsize < 0.
   * Return a subset of placeholder if maxsize < placeholder.size().
   */
  static QString elideMiddle(const QString &string, int maxsize,
                            const QString &placeholder = _ellipsis) {
    if (maxsize < 0 || string.size() < maxsize)
      return string;
    if (placeholder.size() > maxsize)
      return placeholder.left(maxsize);
    return string.left(maxsize/2-placeholder.size())+placeholder
        +string.right(maxsize-maxsize/2);
  }
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
   * E.g. "hello world" -> "hello_world", "HelloWorld" -> "hello_world".
   */
  static QByteArray toSnakeCase(const QByteArray &anycase);
  // LATER static QString toSnakeCase(const QString &anycase);
  // LATER toCamelCase() etc.
};

#endif // STRINGUTILS_H

