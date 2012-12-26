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
#include "httpresponse.h"
#include <QTextStream>

HttpResponse::HttpResponse(QAbstractSocket *output) : _output(output),
  _status(200), _headersSent(false) {
}

QAbstractSocket *HttpResponse::output() {
  if (!_headersSent) {
    QTextStream ts(_output);
    // LATER give a label for each well known status codes
    ts << "HTTP/1.0 " << _status << " Status " << _status << "\r\n";
    // LATER sanitize well-known headers (Content-Type...) values
    // LATER handle multi-line headers and special chars
    foreach (QString name, _headers.keys())
      foreach (QString value, _headers.values(name))
        ts << name << ": " << value << "\r\n";
    if (!_headers.contains("Content-Type"))
      ts << "Content-Type: text/plain;charset=UTF-8\r\n";
    ts << "\r\n";
    _headersSent = true;
  }
  return _output;
}

void HttpResponse::setStatus(int status) {
  if (!_headersSent) {
    _status = status;
  } else
    qWarning() << "HttpResponse: cannot set status after writing data";
}

void HttpResponse::setHeader(const QString name, const QString value) {
  // LATER handle case sensitivity in header names
  if (!_headersSent) {
    _headers.remove(name);
    _headers.insert(name, value);
  } else
    qWarning() << "HttpResponse: cannot set header after writing data";
}

void HttpResponse::addHeader(const QString name, const QString value) {
  // LATER handle case sensitivity in header names
  if (!_headersSent) {
    _headers.insertMulti(name, value);
  } else
    qWarning() << "HttpResponse: cannot set header after writing data";
}

void HttpResponse::redirect(const QString location) {
  setStatus(302);
  setHeader("Location", location);
  setContentType("text/html;charset=UTF-8");
  // LATER html escape
  output()->write(QString("<html><body>Moved. Please click on <a href=\"%1"
                  "\">this link</a>").arg(location).toUtf8().constData());
}
