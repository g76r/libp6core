/* Copyright 2012-2022 Hallowyn and others.
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

#include "pfhandler.h"
#include <QtDebug>
#include <QByteArray>
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
