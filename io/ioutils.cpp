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
#include "ioutils.h"
#include <QIODevice>
#include <QUrl>
#include <QDir>
#include <QFileInfo>

static QRegularExpression _slashBeforeDriveLetterRE{"^/[A-Z]:/"};

QString IOUtils::url2path(QUrl url) {
  if (url.scheme() == "file") {
    QString path = url.path();
    if (_slashBeforeDriveLetterRE.match(path).hasMatch())
      return path.mid(1); // remove leading "/" in "/C:/path/to/file.jpg"
    return path;
  }
  if (url.scheme() == "qrc")
    return ':'+url.path();
  return QString();
}

qint64 IOUtils::copy(QIODevice *dest, QIODevice *src, qint64 max,
                     qint64 bufsize, int readTimeout, int writeTimeout) {
  if (!dest || !src)
    return -1;
  char buf[bufsize];
  int total = 0;
  while (total < max) {
    int shouldBeRead, actuallyRead, actuallyWritten;
    shouldBeRead = std::min(bufsize, max-total);
    if (src->bytesAvailable() < shouldBeRead) {
      src->waitForReadyRead(readTimeout);
    }
    actuallyRead = src->read(buf, shouldBeRead);
    if (actuallyRead < 0)
      return -1;
    if (actuallyRead == 0)
      break;
    actuallyWritten = dest->write(buf, actuallyRead);
    if (actuallyWritten != actuallyRead)
      return -1;
    if (dest->bytesToWrite() > bufsize)
      while (dest->waitForBytesWritten(writeTimeout));
    total += actuallyRead;
  }
  return total;
}

static inline qint64 grep(
    QIODevice *dest, QIODevice *src,
    std::function<bool(QString subject)> matchCondition,
    qint64 max, qint64 bufsize,
    int readTimeout, int writeTimeout) {
  if (!dest || !src)
    return -1;
  char buf[bufsize];
  int total = 0;
  while (total < max) {
    if (src->bytesAvailable() < 1)
      src->waitForReadyRead(readTimeout);
    int n = src->readLine(buf, std::min(bufsize, max-total));
    if (n < 0)
      return -1;
    if (n == 0)
      break;
    if (matchCondition(QString::fromUtf8(buf))) {
      int m = dest->write(buf, n);
      if (m != n)
        return -1;
      if (dest->bytesToWrite() > bufsize)
        while (dest->waitForBytesWritten(writeTimeout));
      total += n;
    }
  }
  return total;
}

qint64 IOUtils::grep(QIODevice *dest, QIODevice *src, QString pattern,
                     bool useRegexp, qint64 max, qint64 bufsize,
                     int readTimeout, int writeTimeout) {
  if (useRegexp) {
    QRegularExpression regexp(pattern);
    return ::grep(
          dest, src,
          [&regexp](QString line) { return regexp.match(line).hasMatch(); },
          max, bufsize, readTimeout, writeTimeout);
  } else
    return ::grep(
          dest, src,
          [&pattern](QString line) { return line.contains(pattern); },
          max, bufsize, readTimeout, writeTimeout);
}

#if QT_VERSION < 0x060000
qint64 IOUtils::grep(QIODevice *dest, QIODevice *src,
                     QRegExp regexp, qint64 max, qint64 bufsize,
                     int readTimeout, int writeTimeout) {
  return ::grep(
        dest, src,
        [&regexp](QString line) { return regexp.indexIn(line) >= 0; },
        max, bufsize, readTimeout, writeTimeout);
}
#endif

qint64 IOUtils::grep(QIODevice *dest, QIODevice *src,
                     QRegularExpression regexp, qint64 max, qint64 bufsize,
                     int readTimeout, int writeTimeout) {
  return ::grep(
        dest, src,
        [&regexp](QString line) { return regexp.match(line).hasMatch(); },
        max, bufsize, readTimeout, writeTimeout);
}

static inline qint64 grepWithContinuation(
    QIODevice *dest, QIODevice *src,
    std::function<bool(QString subject)> matchCondition,
    QString continuationLinePrefix, qint64 max, qint64 bufsize,
    int readTimeout, int writeTimeout) {
  if (!dest || !src)
    return -1;
  char buf[bufsize];
  int total = 0;
  bool continuation = false;
  while (total < max) {
    if (src->bytesAvailable() < 1)
      src->waitForReadyRead(readTimeout);
    int n = src->readLine(buf, std::min(bufsize, max-total));
    if (n < 0)
      return -1;
    if (n == 0)
      break;
    QString line = QString::fromUtf8(buf);
    if ((continuation && line.startsWith(continuationLinePrefix))
        || matchCondition(line)) {
      int m = dest->write(buf, n);
      if (m != n)
        return -1;
      if (dest->bytesToWrite() > bufsize)
        while (dest->waitForBytesWritten(writeTimeout));
      total += n;
      continuation = true;
    } else
      continuation = false;
  }
  return total;
}

qint64 IOUtils::grepWithContinuation(
    QIODevice *dest, QIODevice *src, QString pattern,
    QString continuationLinePrefix, qint64 max, qint64 bufsize,
    int readTimeout, int writeTimeout) {
  return ::grepWithContinuation(
        dest, src,
        [&pattern](QString line) { return line.contains(pattern); },
        continuationLinePrefix, max, bufsize, readTimeout, writeTimeout);
}

#if QT_VERSION < 0x060000
qint64 IOUtils::grepWithContinuation(
    QIODevice *dest, QIODevice *src, QRegExp regexp,
    QString continuationLinePrefix, qint64 max, qint64 bufsize,
    int readTimeout, int writeTimeout) {
  return ::grepWithContinuation(
        dest, src,
        [&regexp](QString line) { return regexp.indexIn(line) >= 0; },
        continuationLinePrefix, max, bufsize, readTimeout, writeTimeout);
}
#endif

qint64 IOUtils::grepWithContinuation(
    QIODevice *dest, QIODevice *src, QRegularExpression regexp,
    QString continuationLinePrefix, qint64 max, qint64 bufsize,
    int readTimeout, int writeTimeout) {
  return ::grepWithContinuation(
        dest, src,
        [&regexp](QString line) { return regexp.match(line).hasMatch(); },
        continuationLinePrefix, max, bufsize, readTimeout, writeTimeout);
}

static void findFiles(QDir dir, QStringList &files,
                      const QRegularExpression &pattern) {
  //qDebug() << "findFiles:" << dir.path() << dir.entryInfoList().size() << files.size() << pattern.pattern();
  foreach (const QFileInfo fi,
           dir.entryInfoList(QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot,
                             QDir::Name)) {
    const QString path = fi.filePath();
    //qDebug() << "  QFileInfo:" << path << fi.isDir() << fi.isFile();
    if (fi.isDir()) {
      //qDebug() << "  Going down:" << path;
      findFiles(QDir(path), files, pattern);
    } else if (fi.isFile() && pattern.match(path).hasMatch()) {
      //qDebug() << "  Appending:" << path;
      files.append(path);
    }
  }
}

static const QRegularExpression slashFollowedByWildcard("/[^/]*[*?[]|\\]");

QStringList IOUtils::findFiles(QString regexp) {
  QStringList files;
  QString pat = QDir().absoluteFilePath(QDir::fromNativeSeparators(regexp));
  int i = pat.indexOf(slashFollowedByWildcard);
  QString dir = i >= 0 ? pat.left(i+1) : pat;
  ::findFiles(QDir(dir), files, QRegularExpression("^"+pat+"$"));
  //qDebug() << "returned file list:" << files;
  return files;
}
