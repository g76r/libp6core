/*
Copyright 2012 Hallowyn and others.
See the NOTICE file distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file to you under
the Apache License, Version 2.0 (the "License"); you may not use this
file except in compliance with the License. You may obtain a copy of the
License at http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations
under the License.
*/

#include "pfioutils.h"
#include <QIODevice>
#include <QRegExp>
#include <QtDebug>

QString IOUtils::url2path(const QUrl &url) {
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

qint64 IOUtils::copyAll(QIODevice &dest, QIODevice &src, qint64 bufsize) {
  char buf[bufsize];
  int total = 0, n, m;
  for (;;) {
    if (src.bytesAvailable() < 1)
      src.waitForReadyRead(30000);
    n = src.read(buf, bufsize);
    if (n < 0)
      return -1;
    if (n == 0)
      return total;
    m = dest.write(buf, n);
    if (m != n)
      return -1;
    total += n;
  }
}

qint64 IOUtils::copy(QIODevice &dest, QIODevice &src, qint64 max,
                            qint64 bufsize) {
  char buf[bufsize];
  int total = 0, n, m;
  while (total < max) {
    if (src.bytesAvailable() < 1)
      src.waitForReadyRead(30000);
    n = src.read(buf, std::min(bufsize, max-total));
    if (n < 0)
      return -1;
    if (n == 0)
      break;
    m = dest.write(buf, n);
    if (m != n)
      return -1;
    total += n;
  }
  return total;
}
