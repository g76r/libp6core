/* Copyright 2012-2013 Hallowyn and others.
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
#include "ioutils.h"
#include <QIODevice>
#include <QRegExp>
#include <QDir>
#include <QtDebug>

QString IOUtils::url2path(QUrl url) {
  if (url.scheme() == "file") {
    QString path = url.path();
    QRegExp rx("/[A-Z]:/.*");
    if (rx.exactMatch(path))
      return path.mid(1); // remove leading "/" in "/C:/path/to/file.jpg"
    return path;
  }
  if (url.scheme() == "qrc")
    return QString(":%1").arg(url.path());
  return QString();
}

qint64 IOUtils::copy(QIODevice *dest, QIODevice *src, qint64 max,
                     qint64 bufsize) {
  if (!dest || !src)
    return -1;
  char buf[bufsize];
  int total = 0, n, m;
  while (total < max) {
    if (src->bytesAvailable() < 1)
      src->waitForReadyRead(30000);
    n = src->read(buf, std::min(bufsize, max-total));
    if (n < 0)
      return -1;
    if (n == 0)
      break;
    m = dest->write(buf, n);
    if (m != n)
      return -1;
    if (dest->bytesToWrite() > bufsize)
      while (dest->waitForBytesWritten(30000) > bufsize);
    total += n;
  }
  return total;
}

qint64 IOUtils::grep(QIODevice *dest, QIODevice *src, QString pattern,
                     bool useRegexp, qint64 max, qint64 bufsize) {
  if (!dest || !src)
    return -1;
  if (useRegexp)
    return grep(dest, src, QRegExp(pattern), max, bufsize);
  char buf[bufsize];
  int total = 0, n, m;
  while (total < max) {
    if (src->bytesAvailable() < 1)
      src->waitForReadyRead(30000);
    n = src->readLine(buf, std::min(bufsize, max-total));
    if (n < 0)
      return -1;
    if (n == 0)
      break;
    if (QString::fromUtf8(buf).contains(pattern)) {
      m = dest->write(buf, n);
      if (m != n)
        return -1;
      if (dest->bytesToWrite() > bufsize)
        while (dest->waitForBytesWritten(30000) > bufsize);
      total += n;
    }
  }
  return total;
}

qint64 IOUtils::grep(QIODevice *dest, QIODevice *src,
                     QRegExp regexp, qint64 max, qint64 bufsize) {
  if (!dest || !src)
    return -1;
  char buf[bufsize];
  int total = 0, n, m;
  while (total < max) {
    if (src->bytesAvailable() < 1)
      src->waitForReadyRead(30000);
    n = src->readLine(buf, std::min(bufsize, max-total));
    if (n < 0)
      return -1;
    if (n == 0)
      break;
    if (regexp.indexIn(QString::fromUtf8(buf)) >= 0) {
      m = dest->write(buf, n);
      if (m != n)
        return -1;
      if (dest->bytesToWrite() > bufsize)
        while (dest->waitForBytesWritten(30000) > bufsize);
      total += n;
    }
  }
  return total;
}

static void findFiles(QDir dir, QStringList &files, const QRegExp pattern) {
  //qDebug() << "findFiles:" << dir.path() << dir.entryInfoList().size() << files.size() << pattern.pattern();
  foreach (const QFileInfo fi,
           dir.entryInfoList(QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot,
                             QDir::Name)) {
    const QString path = fi.filePath();
    //qDebug() << "  QFileInfo:" << path << fi.isDir() << fi.isFile();
    if (fi.isDir()) {
      //qDebug() << "  Going down:" << path;
      findFiles(QDir(path), files, pattern);
    } else if (fi.isFile() && pattern.exactMatch(path)) {
      //qDebug() << "  Appending:" << path;
      files.append(path);
    }
  }
}

QStringList IOUtils::findFiles(QString pattern) {
  QStringList files;
  QString pat = QDir().absoluteFilePath(QDir::fromNativeSeparators(pattern));
  static const QRegExp slashFollowedByWildcard("/[^/]*[*?[]|\\]");
  int i = pat.indexOf(QRegExp(slashFollowedByWildcard));
  QString dir = i >= 0 ? pat.left(i+1) : pat;
  QRegExp re(pat, Qt::CaseSensitive, QRegExp::Wildcard);
  ::findFiles(QDir(dir), files, re);
  return files;
}
