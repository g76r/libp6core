#include "pfhandler.h"
#include <QtDebug>

PfHandler::PfHandler() {
}

PfHandler::~PfHandler() {
}

bool PfHandler::startDocument(const PfOptions &options) {
  _options = options;
  return true;
}

bool PfHandler::startNode(const QList<QString> &names) {
  Q_UNUSED(names);
  return true;
}

bool PfHandler::text(const QString &text) {
  Q_UNUSED(text);
  return true;
}

bool PfHandler::binary(QIODevice *device, qint64 length, qint64 offset,
                       const QString &surface) {
  if (device->isSequential()) {
    setErrorString("PfHandler: binary fragment lazy loading cannot handle"
                   "sequential (= not seekable) data input");
    return false;
  }
  qint64 pos = device->pos();
  if (!device->seek(offset)) {
    setErrorString(QString("PfHandler: cannot seek at %1 within data input")
                   .arg(offset));
    return false;
  }
  // waiting for bytes being available seems useless since there are no
  // devices that are not sequential and on which bytes may not be immediately
  // available
  QByteArray data = device->read(length);
  if (data.size() != length) {
    setErrorString(QString("PfHandler: cannot read %1 bytes at %2 within data "
                           "input").arg(length).arg(offset));
    device->seek(pos);
    return false;

  }
  device->seek(pos);
  return binary(data, surface);
}

bool PfHandler::binary(const QByteArray &data, const QString &surface) {
  Q_UNUSED(data);
  Q_UNUSED(surface);
  return true;
}

bool PfHandler::array(const PfArray &array) {
  Q_UNUSED(array);
  return true;
}

bool PfHandler::endNode(const QList<QString> &names) {
  Q_UNUSED(names);
  return true;
}

bool PfHandler::comment(const QString &content) {
  Q_UNUSED(content);
  return true;
}

bool PfHandler::endDocument() {
  return true;
}

void PfHandler::error(int line, int column) {
  qWarning() << "PfHandler::error line" << line << "column" << column << ":"
      << errorString();
}
