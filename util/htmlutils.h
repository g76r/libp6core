/* Copyright 2013 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
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
#ifndef HTMLUTILS_H
#define HTMLUTILS_H

#include <QString>
#include "libqtssu_global.h"

class LIBQTSSUSHARED_EXPORT HtmlUtils {
  HtmlUtils() { }
public:
  /** Encode raw text to make it includable in an html document.
   * Special chars (such as '<') are replaced with entities.
   * Non ascii (> 127) chars are left unchanged (i.e. returned QString is still
   * unicode and must be converted to suited encoding before written to a
   * document, e.g. using QString::toUtf8()).
   * @urlAsLinks if true strings like "http://foo/bar" will be converted into html links
   */
  static QString htmlEncode(QString text, bool urlAsLinks = true);
};

#endif // HTMLUTILS_H
