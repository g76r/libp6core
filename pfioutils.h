#ifndef IOUTILS_H
#define IOUTILS_H

#include <QtGlobal>
#include <QUrl>

class QIODevice;

class IOUtils {
private:
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
  // LATER support other schemes than "file" and "qrc"
  static QString url2path(const QUrl &url);
};

#endif // IOUTILS_H
