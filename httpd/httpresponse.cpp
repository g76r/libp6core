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
#include "httpresponse.h"
#include "httpcommon.h"
#include <QRegularExpression>
#include "log/log.h"
#include <QSharedData>
#include <QMultiHash>
#include "format/timeformats.h"
#include "io/dummysocket.h"

using namespace Qt::Literals::StringLiterals;

class HttpResponseData : public QSharedData {
public:
  QAbstractSocket *_output;
  int _status;
  bool _headersSent, _disableBodyOutput;
  QMultiMap<QByteArray,QByteArray> _headers;
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
    QByteArray ba;
    ba = "HTTP/1.1 "_ba + QByteArray::number(d->_status) + " "_ba
        + statusAsString(d->_status) + "\r\n"_ba;
    // LATER sanitize well-known headers (Content-Type...) values
    // LATER handle multi-line headers and special chars
    auto keys = d->_headers.keys();
#if QT_VERSION >= 0x050f00
    for (auto name: QSet<QByteArray>(keys.begin(), keys.end()))
#else
    for (auto name: keys.toSet())
#endif
      for (auto value: d->_headers.values(name))
        ba += name + ": "_ba + value + "\r\n"_ba;
    if (header("Content-Type"_ba).isEmpty())
      ba += "Content-Type: text/plain;charset=UTF-8\r\n"_ba;
    ba += "Connection: close\r\n\r\n"_ba;
    d->_output->write(ba);
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

void HttpResponse::setHeader(const QByteArray &name, const QByteArray &value) {
  // LATER handle case insensitivity in header names
  if (d && !d->_headersSent) {
    d->_headers.replace(name, value);
  } else
    Log::warning() << "HttpResponse: cannot set header after writing data";
}

void HttpResponse::addHeader(
    const QByteArray &name, const QByteArray &value) {
  // LATER handle case insensitivity in header names
  if (d && !d->_headersSent) {
    d->_headers.insert(name, value);
  } else
    Log::warning() << "HttpResponse: cannot set header after writing data";
}

void HttpResponse::appendValueToHeader(
    const QByteArray &name, const QByteArray &value,
    const QByteArray &separator) {
  // LATER handle case insensitivity in header names
  if (d && !d->_headersSent) {
    auto values = d->_headers.values(name);
    values.append(value);
    d->_headers.insert(name, values.join(separator));
  } else
    Log::warning() << "HttpResponse: cannot set header after writing data";
}

void HttpResponse::redirect(QByteArray location, int status) {
  if (!d)
    return;
  setStatus(status);
  setHeader("Location"_ba, location);
  setContentType("text/html;charset=UTF-8"_ba);
  // LATER url encode
  output()->write("<html><body>Moved. Please click on <a href=\""_ba
                  +location+"\">this link</a>"_ba);
}

static const QRegularExpression _nameRegexp {
  "\\A" RFC2616_TOKEN_OCTET_RE "+\\z" };
static const QRegularExpression _valueRegexp {
  "\\A" RFC6265_COOKIE_OCTET_RE "*\\z" };
static const QRegularExpression _pathRegexp {
  "\\A" RFC6265_PATH_VALUE_RE "\\z" };
static const QRegularExpression _domainRegexp {
  "\\A" INTERNET_DOMAIN_RE "\\z" };

void HttpResponse::setCookie(
    const QByteArray &name, const QByteArray &value,
    QDateTime expires, const QByteArray &path,
    const QByteArray &domain, bool secure, bool httponly) {
  if (!_nameRegexp.match(name).hasMatch()) {
    Log::warning() << "HttpResponse: incorrect name when setting cookie: "
                   << name;
    return;
  }
  if (!_valueRegexp.match(value).hasMatch()) {
    Log::warning() << "HttpResponse: incorrect value when setting cookie: "
                   << value;
    return;
  }
  QByteArray s = name+'='+value;
  if (!expires.isNull())
    s += "; Expires="_ba + TimeFormats::toRfc2822DateTime(expires).toUtf8();
  if (!path.isEmpty()) {
    if (_pathRegexp.match(path).hasMatch())
      s.append("; Path=").append(path);
    else {
      Log::warning() << "HttpResponse: incorrect path when setting cookie: "
                     << path;
      return;
    }
  }
  if (!domain.isEmpty()) {
    if (_domainRegexp.match(domain).hasMatch())
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

QByteArray HttpResponse::header(
    const QByteArray &name, const QByteArray &defaultValue) const {
  // LATER handle case insensitivity in header names
  if (!d)
    return defaultValue;
  auto v = d->_headers.value(name);
  return v.isNull() ? defaultValue : v;
}

QByteArrayList HttpResponse::headers(const QByteArray &name) const {
  // LATER handle case insensitivity in header names
  return d ? d->_headers.values(name) : QByteArrayList{};
}

QMultiMap<QByteArray,QByteArray> HttpResponse::headers() const {
  return d ? d->_headers : QMultiMap<QByteArray,QByteArray>();
}

QByteArray HttpResponse::statusAsString(int status) {
  switch(status) {
  case 200:
    return "Ok"_ba;
  case 201:
    return "Created"_ba;
  case 202:
    return "Accepted"_ba;
  case 300:
    return "Multiple choices"_ba;
  case 301:
    return "Moving permantently"_ba;
  case 302:
    return "Found"_ba;
  case 303:
    return "See other"_ba;
  case 304:
    return "Not modified"_ba;
  case 305:
    return "Use proxy"_ba;
  case 306:
    return "Switch proxy"_ba;
  case 307:
    return "Temporary redirect"_ba;
  case 308:
    return "Permanent redirect"_ba;
  case 400:
    return "Bad request"_ba;
  case 401:
    return "Authentication required"_ba;
  case 402:
    return "Insert coin"_ba;
  case 403:
    return "Forbidden"_ba;
  case 404:
    return "Not Found"_ba;
  case 405:
    return "Method not allowed"_ba;
  case 408:
    return "Request timeout"_ba;
  case 413:
    return "Request entity too large"_ba;
  case 414:
    return "Request URI too large"_ba;
  case 415:
    return "Unsupported media type"_ba;
  case 418:
    return "I'm a teapot"_ba;
  case 500:
    return "Internal server error"_ba;
  case 501:
    return "Not implemented"_ba;
  default:
    return "Status "_ba+QByteArray::number(status);
  }
}
