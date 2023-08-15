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

#include "pfhandler.h"
#include <QIODevice>

PfHandler::PfHandler() : _errorLine(0), _errorColumn(0), _errorOccured(false) {
}

PfHandler::~PfHandler() {
}

bool PfHandler::startDocument(const PfOptions &options) {
  _options = options;
  return true;
}

bool PfHandler::startNode(const QStringList &names) {
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
    _errorOccured = true;
    return false;
  }
  qint64 pos = device->pos();
  if (!device->seek(offset)) {
    setErrorString(QString("PfHandler: cannot seek at %1 within data input")
                   .arg(offset));
    _errorOccured = true;
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
    _errorOccured = true;
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

bool PfHandler::endNode(const QStringList &names) {
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
  _errorLine = line;
  _errorColumn = column;
  _errorOccured = true;
  //qWarning() << "PfHandler::error line" << line << "column" << column << ":"
  //    << errorString();
}
