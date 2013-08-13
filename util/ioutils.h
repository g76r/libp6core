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

#include <QUrl>
#include <QStringList>
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

  /** Copy at most max bytes from dest to src, copying only lines that match
   * pattern.
   * There may be some strange behaviour if lines are longer than maxLineSize.
   * @param useRegexp otherwise pattern is plain data
   */
  static qint64 grepString(QIODevice *dest, QIODevice *src, qint64 max,
                           const QString pattern, bool useRegexp,
                           qint64 maxLineSize = 65535);

  /** Copy at most max bytes from dest to src, copying only lines that match
   * pattern.
   * There may be some strange behaviour if lines are longer than maxLineSize.
   */
  static qint64 grepRegexp(QIODevice *dest, QIODevice *src, qint64 max,
                           const QString pattern,
                           qint64 maxLineSize = 65535);

  /** Convert QUrl object to local path usable with e.g. QFile
    * Only support "file" and "qrc" schemes.
    * @return path, QString::isNull() if URL not supported (e.g. its scheme)
    */
  static QString url2path(const QUrl &url);

  /** Return paths of all existing files that match pattern.
    * Pattern is a globing pattern (@see QRegExp::Wildcard).
    * Beware that this method can take a lot of time depending on filesystem
    * tree size. */
  static QStringList findFiles(const QString pattern);

  /** Return paths of all existing files that match patterns.
    * Pattern is a globing pattern (@see QRegExp::Wildcard).
    * Beware that this method can take a lot of time depending on filesystem
    * tree size. */
  inline static QStringList findFiles(const QStringList patterns) {
    QStringList files;
    foreach (const QString pattern, patterns)
      files.append(findFiles(pattern));
    return files;
  }
};

#endif // IOUTILS_H
