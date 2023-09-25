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
#include "log/log.h"
#include "format/timeformats.h"
#include "io/dummysocket.h"
#include <QRegularExpression>

using namespace Qt::Literals::StringLiterals;

class HttpResponseData : public QSharedData {
public:
  QAbstractSocket *_output;
  int _status;
  bool _headersSent, _disableBodyOutput;
  QMultiMap<Utf8String,Utf8String> _headers;
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
    Utf8String ba;
    ba = "HTTP/1.1 "_u8 + Utf8String::number(d->_status) + " "_u8
        + statusAsString(d->_status) + "\r\n"_u8;
    // LATER sanitize well-known headers (Content-Type...) values
    // LATER handle multi-line headers and special chars
    for (auto name: Utf8StringList(d->_headers.keys())
         .toSortedDeduplicatedList())
      for (auto value: d->_headers.values(name))
        ba += name + ": "_u8 + value + "\r\n"_u8;
    if (header("Content-Type"_u8).isEmpty())
      ba += "Content-Type: text/plain;charset=UTF-8\r\n"_u8;
    ba += "Connection: close\r\n\r\n"_u8;
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

void HttpResponse::setHeader(const Utf8String &name, const Utf8String &value) {
  // LATER handle case insensitivity in header names
  if (d && !d->_headersSent) {
    d->_headers.replace(name, value);
  } else
    Log::warning() << "HttpResponse: cannot set header after writing data";
}

void HttpResponse::addHeader(const Utf8String &name, const Utf8String &value) {
  // LATER handle case insensitivity in header names
  if (d && !d->_headersSent) {
    d->_headers.insert(name, value);
  } else
    Log::warning() << "HttpResponse: cannot set header after writing data";
}

void HttpResponse::appendValueToHeader(
    const Utf8String &name, const Utf8String &value,
    const Utf8String &separator) {
  // LATER handle case insensitivity in header names
  if (!d)
    return;
  if (!d->_headersSent) {
    Log::warning() << "HttpResponse: cannot set header after writing data";
    return;
  }
  Utf8StringList values = d->_headers.values(name);
  values.append(value);
  d->_headers.insert(name, values.join(separator));
}

void HttpResponse::redirect(Utf8String location, int status) {
  if (!d)
    return;
  setStatus(status);
  setHeader("Location"_u8, location);
  setContentType("text/html;charset=UTF-8"_u8);
  // LATER url encode
  output()->write("<html><body>Moved. Please click on <a href=\""
                  +location+"\">this link</a>");
}

static const QRegularExpression _nameRegexp {
  "\\A" RFC2616_TOKEN_OCTET_RE "+\\z" };
static const QRegularExpression _valueRegexp {
  "\\A" RFC6265_COOKIE_OCTET_RE "*\\z" };
static const QRegularExpression _pathRegexp {
  "\\A" RFC6265_PATH_VALUE_RE "\\z" };
static const QRegularExpression _domainRegexp {
  "\\A" INTERNET_DOMAIN_RE "\\z" };

void HttpResponse::setCookie(const Utf8String &name, const Utf8String &value,
    QDateTime expires, const Utf8String &path,
    const Utf8String &domain, bool secure, bool httponly) {
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
  auto s = name+"="_u8+value;
  if (!expires.isNull())
    s += "; Expires="_u8 + TimeFormats::toRfc2822DateTime(expires).toUtf8();
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

Utf8String HttpResponse::header(
    const Utf8String &name, const Utf8String &def) const {
  // LATER handle case insensitivity in header names
  if (!d)
    return def;
  auto v = d->_headers.value(name);
  return v.isNull() ? def : v;
}

Utf8StringList HttpResponse::headers(const Utf8String &name) const {
  // LATER handle case insensitivity in header names
  return d ? d->_headers.values(name) : Utf8StringList{};
}

QMultiMap<Utf8String, Utf8String> HttpResponse::headers() const {
  return d ? d->_headers : QMultiMap<Utf8String,Utf8String>();
}

Utf8String HttpResponse::statusAsString(int status) {
  switch(status) {
  case 200:
    return "Ok"_u8;
  case 201:
    return "Created"_u8;
  case 202:
    return "Accepted"_u8;
  case 300:
    return "Multiple choices"_u8;
  case 301:
    return "Moving permantently"_u8;
  case 302:
    return "Found"_u8;
  case 303:
    return "See other"_u8;
  case 304:
    return "Not modified"_u8;
  case 305:
    return "Use proxy"_u8;
  case 306:
    return "Switch proxy"_u8;
  case 307:
    return "Temporary redirect"_u8;
  case 308:
    return "Permanent redirect"_u8;
  case 400:
    return "Bad request"_u8;
  case 401:
    return "Authentication required"_u8;
  case 402:
    return "Insert coin"_u8;
  case 403:
    return "Forbidden"_u8;
  case 404:
    return "Not Found"_u8;
  case 405:
    return "Method not allowed"_u8;
  case 408:
    return "Request timeout"_u8;
  case 413:
    return "Request entity too large"_u8;
  case 414:
    return "Request URI too large"_u8;
  case 415:
    return "Unsupported media type"_u8;
  case 418:
    return "I'm a teapot"_u8;
  case 500:
    return "Internal server error"_u8;
  case 501:
    return "Not implemented"_u8;
  default:
    return "Status "_u8+Utf8String::number(status);
  }
}
