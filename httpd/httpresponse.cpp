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
