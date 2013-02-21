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
#include "httpresponse.h"
#include <QTextStream>
#include "httpcommon.h"
#include <QRegExp>
#include "log/log.h"

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

void HttpResponse::setCookie(const QString name, const QString value,
                             const QDateTime expires, const QString path,
                             const QString domain, bool secure, bool httponly) {
  // LATER enhance regexp performance
  if (!QRegExp(RFC2616_TOKEN_OCTET_RE "+").exactMatch(name)) {
    Log::warning() << "HttpResponse: incorrect name when setting cookie: "
                   << name;
    return;
  }
  if (!QRegExp(RFC6265_COOKIE_OCTET_RE "*").exactMatch(value)) {
    Log::warning() << "HttpResponse: incorrect value when setting cookie: "
                   << value;
    return;
  }
  QString s(name);
  s.append('=');
  s.append(value);
  if (!expires.isNull())
    s.append("; Expires=")
        .append(expires.toUTC().toString("ddd, dd MMM yyyy hh:mm:ss"))
        .append(" GMT");
  if (!path.isEmpty()) {
    if (QRegExp(RFC6265_PATH_VALUE_RE).exactMatch(path))
      s.append("; Path=").append(path);
    else {
      Log::warning() << "HttpResponse: incorrect path when setting cookie: "
                     << path;
      return;
    }
  }
  if (!domain.isEmpty()) {
    if (QRegExp(INTERNET_DOMAIN_RE).exactMatch(domain))
      s.append("; Domain=").append(domain);
    else {
      Log::warning() << "HttpResponse: incorrect domain when setting cookie: "
                     << domain;
      return;
    }
  }
  if (secure)
    s.append("; Secure");
  if (httponly)
    s.append("; HttpOnly");
  setHeader("Set-Cookie", s);
}
