/* Copyright 2012 Hallowyn and others.
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
#ifndef IOUTILS_H
#define IOUTILS_H

#include <QtGlobal>
#include <QUrl>
#include "libqtssu_global.h"

class QIODevice;

class LIBQTSSUSHARED_EXPORT IOUtils {
  IOUtils() { }
public:
  /** Copy content of src into dest until reaching src's end.
    */
  static qint64 copyAll(QIODevice &dest, QIODevice &src,
                        qint64 bufsize = 65536);
  /** syntaxic sugar */
  inline static qint64 copyAll(QIODevice *dest, QIODevice &src,
                               qint64 bufsize = 65536) {
    return copyAll(*dest, src, bufsize);
  }
  /** syntaxic sugar */
  inline static qint64 copyAll(QIODevice &dest, QIODevice *src,
                               qint64 bufsize = 65536) {
    return copyAll(dest, *src, bufsize);
  }
  /** syntaxic sugar */
  inline static qint64 copyAll(QIODevice *dest, QIODevice *src,
                               qint64 bufsize = 65536) {
    return copyAll(*dest, *src, bufsize);
  }
  /** Copy content of src into dest until max bytes or src's end is reached.
    */
  static qint64 copy(QIODevice &dest, QIODevice &src, qint64 max,
                     qint64 bufsize = 65536);
  /** syntaxic sugar */
  inline static qint64 copy(QIODevice *dest, QIODevice &src, qint64 max,
                            qint64 bufsize = 65536) {
    return copy(*dest, src, max, bufsize);
  }
  /** syntaxic sugar */
  inline static qint64 copy(QIODevice &dest, QIODevice *src, qint64 max,
                            qint64 bufsize = 65536) {
    return copy(dest, *src, max, bufsize);
  }
  /** syntaxic sugar */
  inline static qint64 copy(QIODevice *dest, QIODevice *src, qint64 max,
                            qint64 bufsize = 65536) {
    return copy(*dest, *src, max, bufsize);
  }

  /** Convert QUrl object to local path usable with e.g. QFile
    * Only support "file" and "qrc" schemes.
    * @return path, QString::isNull() if URL not supported (e.g. its scheme)
    */
  static QString url2path(const QUrl &url);
};

#endif // IOUTILS_H
