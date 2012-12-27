#include "ioutils.h"
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
