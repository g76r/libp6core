/* Copyright 2012-2016 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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
#include <QSharedData>
#include <QMultiHash>
#include "util/timeformats.h"
#include "net/dummysocket.h"

class HttpResponseData : public QSharedData {
public:
  QAbstractSocket *_output;
  int _status;
  bool _headersSent, _disableBodyOutput;
  QMultiHash<QString,QString> _headers;
  explicit HttpResponseData(QAbstractSocket *output)
    : _output(output), _status(200), _headersSent(false),
      _disableBodyOutput(false) { }
};

HttpResponse::HttpResponse(QAbstractSocket *output)
  : d(new HttpResponseData(output)) {
}

HttpResponse::HttpResponse() {
}

HttpResponse::HttpResponse(const HttpResponse &other) : d(other.d) {
}

HttpResponse::~HttpResponse() {
}

HttpResponse &HttpResponse::operator=(const HttpResponse &other) {
  if (this != &other)
    d.operator=(other.d);
  return *this;
}

void HttpResponse::disableBodyOutput() {
  if (d)
    d->_disableBodyOutput = true;
}

QAbstractSocket *HttpResponse::output() {
  if (!d)
    return DummySocket::singletonInstance();
  if (!d->_headersSent) {
    QTextStream ts(d->_output);
    ts << "HTTP/1.0 " << d->_status << " " << statusAsString(d->_status)
       << "\r\n";
    // LATER sanitize well-known headers (Content-Type...) values
    // LATER handle multi-line headers and special chars
    foreach (QString name, d->_headers.keys().toSet())
      foreach (QString value, d->_headers.values(name))
        ts << name << ": " << value << "\r\n";
    if (header(QStringLiteral("Content-Type")).isEmpty())
      ts << "Content-Type: text/plain;charset=UTF-8\r\n";
    ts << "\r\n";
    d->_headersSent = true;
  }
  return d->_disableBodyOutput ? DummySocket::singletonInstance() : d->_output;
}

void HttpResponse::setStatus(int status) {
  if (d && !d->_headersSent) {
    d->_status = status;
  } else
    Log::warning() << "HttpResponse: cannot set status after writing data";
}

int HttpResponse::status() const {
  return d ? d->_status : 0;
}

void HttpResponse::setHeader(QString name, QString value) {
  // LATER handle case insensitivity in header names
  if (d && !d->_headersSent) {
    d->_headers.remove(name);
    d->_headers.insert(name, value);
  } else
    Log::warning() << "HttpResponse: cannot set header after writing data";
}

void HttpResponse::addHeader(QString name, QString value) {
  // LATER handle case insensitivity in header names
  if (d && !d->_headersSent) {
    d->_headers.insertMulti(name, value);
  } else
    Log::warning() << "HttpResponse: cannot set header after writing data";
}

void HttpResponse::redirect(QString location, int status) {
  if (!d)
    return;
  setStatus(status);
  setHeader(QStringLiteral("Location"), location);
  setContentType(QStringLiteral("text/html;charset=UTF-8"));
  // LATER url encode
  output()->write(QStringLiteral(
                    "<html><body>Moved. Please click on <a href=\"%1"
                    "\">this link</a>").arg(location).toUtf8().constData());
}

// LATER convert to QRegularExpression, but not without regression/unit testing
static const QRegExp nameRegexp(RFC2616_TOKEN_OCTET_RE "+");
static const QRegExp valueRegexp(RFC6265_COOKIE_OCTET_RE "*");
static const QRegExp pathRegexp(RFC6265_PATH_VALUE_RE);
static const QRegExp domainRegexp(INTERNET_DOMAIN_RE);

void HttpResponse::setCookie(QString name, QString value,
                             QDateTime expires, QString path,
                             QString domain, bool secure, bool httponly) {
  if (!QRegExp(nameRegexp).exactMatch(name)) {
    Log::warning() << "HttpResponse: incorrect name when setting cookie: "
                   << name;
    return;
  }
  if (!QRegExp(valueRegexp).exactMatch(value)) {
    Log::warning() << "HttpResponse: incorrect value when setting cookie: "
                   << value;
    return;
  }
  QString s(name);
  s.append('=');
  s.append(value);
  if (!expires.isNull())
    s.append("; Expires=").append(TimeFormats::toRfc2822DateTime(expires));
  if (!path.isEmpty()) {
    if (QRegExp(pathRegexp).exactMatch(path))
      s.append("; Path=").append(path);
    else {
      Log::warning() << "HttpResponse: incorrect path when setting cookie: "
                     << path;
      return;
    }
  }
  if (!domain.isEmpty()) {
    if (QRegExp(domainRegexp).exactMatch(domain))
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
  // LATER maintain a memory map of set cookies and write them only when starting the response write
  // this would enable to write only the last same cookie value
  // LATER update qron's webconsole to use such a feature to clearCookie("message") at start, knowning that a later setCookie("message") will prevent the clearCookie to output a Set-Cookie header
  addHeader("Set-Cookie", s);
}

QString HttpResponse::header(QString name, QString defaultValue) const {
  // LATER handle case insensitivity in header names
  if (!d)
    return defaultValue;
  const QString v = d->_headers.value(name);
  return v.isNull() ? defaultValue : v;
}

QStringList HttpResponse::headers(QString name) const {
  // LATER handle case insensitivity in header names
  return d ? d->_headers.values(name) : QStringList();
}

QMultiHash<QString,QString> HttpResponse::headers() const {
  return d ? d->_headers : QMultiHash<QString,QString>();
}

QString HttpResponse::statusAsString(int status) {
  switch(status) {
  // LATER other statuses
  case 200:
    return QStringLiteral("Ok");
  case 201:
    return QStringLiteral("Created");
  case 202:
    return QStringLiteral("Accepted");
  case 300:
    return QStringLiteral("Multiple choices");
  case 301:
    return QStringLiteral("Moving permantently");
  case 302:
    return QStringLiteral("Found");
  case 303:
    return QStringLiteral("See other");
  case 304:
    return QStringLiteral("Not modified");
  case 305:
    return QStringLiteral("Use proxy");
  case 306:
    return QStringLiteral("Switch proxy");
  case 307:
    return QStringLiteral("Temporary redirect");
  case 308:
    return QStringLiteral("Permanent redirect");
  case 400:
    return QStringLiteral("Bad request");
  case 401:
    return QStringLiteral("Authentication required");
  case 402:
    return QStringLiteral("Insert coin");
  case 403:
    return QStringLiteral("Forbidden");
  case 404:
    return QStringLiteral("Not Found");
  case 405:
    return QStringLiteral("Method not allowed");
  case 408:
    return QStringLiteral("Request timeout");
  case 413:
    return QStringLiteral("Request entity too large");
  case 414:
    return QStringLiteral("Request URI too large");
  case 415:
    return QStringLiteral("Unsupported media type");
  case 418:
    return QStringLiteral("I'm a teapot");
  case 500:
    return QStringLiteral("Internal server error");
  case 501:
    return QStringLiteral("Not implemented");
  default:
    return QStringLiteral("Status %1").arg(status);
  }
}
