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
#include "httprequest.h"
#include <QtDebug>
#include <QRegularExpression>
#include "httpcommon.h"
#include <QSharedData>
#include <QHostAddress>
#include <QHash>
#include "util/radixtree.h"
#include "util/containerutils.h"
#include "format/stringutils.h"

static QByteArray _xffHeader;

namespace {
struct XffHeaderInitializer {
  XffHeaderInitializer() {
    QByteArray header = qgetenv("LIBPUMPKIN_X_FORWARDED_FOR_HEADER");
    if (header.isNull())
      header = "X-Forwarded-For";
    _xffHeader = header;
  }
} xffHeaderInitializer;
}

class HttpRequestData : public QSharedData {
public:
  QAbstractSocket *_input;
  HttpRequest::HttpMethod _method;
  QMultiMap<QByteArray,QByteArray> _headers;
  QMap<QByteArray,QByteArray> _cookies, _paramsCache;
  QUrl _url;
  QUrlQuery _query;
  QByteArrayList _clientAdresses;
  explicit HttpRequestData(QAbstractSocket *input) : _input(input),
    _method(HttpRequest::NONE) { }
};

HttpRequest::HttpRequest(QAbstractSocket *input)
  : d(new HttpRequestData(input)) {
}

HttpRequest::HttpRequest() {
}

HttpRequest::HttpRequest(const HttpRequest &other) : d(other.d) {
}

HttpRequest::~HttpRequest() {
}

HttpRequest &HttpRequest::operator=(const HttpRequest &other) {
  if (this != &other)
    d.operator=(other.d);
  return *this;
}

static QHash<HttpRequest::HttpMethod,QByteArray> _methodToText {
  { HttpRequest::NONE, "NONE" },
  { HttpRequest::HEAD, "HEAD" },
  { HttpRequest::GET, "GET" },
  { HttpRequest::POST, "POST" },
  { HttpRequest::PUT, "PUT" },
  { HttpRequest::DELETE, "DELETE" },
  { HttpRequest::OPTIONS, "OPTIONS" },
  { HttpRequest::ANY, "ANY" },
};

static QHash<QByteArray,HttpRequest::HttpMethod> _methodFromText {
  ContainerUtils::reversed(_methodToText)
};

QSet<HttpRequest::HttpMethod> HttpRequest::_wellKnownMethods {
  HttpRequest::HEAD, HttpRequest::GET, HttpRequest::POST, HttpRequest::PUT,
      HttpRequest::DELETE, HttpRequest::OPTIONS,
};

QSet<QByteArray> HttpRequest::_wellKnownMethodNames = []() {
  QSet<QByteArray> set;
  for (HttpRequest::HttpMethod m : HttpRequest::_wellKnownMethods)
    set.insert(HttpRequest::methodName(m));
  return set;
}();

QByteArray HttpRequest::methodName(HttpMethod method) {
  return _methodToText.value(method, "UNKNOWN"_ba);
}

HttpRequest::HttpMethod HttpRequest::methodFromText(const QByteArray &name) {
  return _methodFromText.value(name, NONE);
}

bool HttpRequest::parseAndAddHeader(QByteArray rawHeader) {
  if (!d)
    return false;
  int i = rawHeader.indexOf(':');
  if (i == -1)
    return false;
  // MAYDO remove special chars from keys and values?
  // TODO support multi-line headers
  QByteArray key = StringUtils::toAsciiSnakeUpperCamelCase(
        rawHeader.left(i).trimmed());
  QByteArray value = rawHeader.right(rawHeader.size()-i-1).trimmed();
  //qDebug() << "header:" << rawHeader << key << value;
  d->_headers.insert(key, value);
  if (key == "Cookie"_ba)
    parseAndAddCookie(value);
  return true;
}

// TODO use QRegularExpression instead, but not without regression/unit testing
static const QRegularExpression _cookieHeaderValue(
      "\\s*;?\\s*(" RFC2616_TOKEN_OCTET_RE "*)\\s*=\\s*(("
      RFC6265_COOKIE_OCTET_RE "*|\"" RFC6265_COOKIE_OCTET_RE
      "+\"))\\s*;?\\s*");

void HttpRequest::parseAndAddCookie(QByteArray rawHeaderValue) {
  // LATER use QNetworkCookie::parseCookies
  // LATER ensure that utf8 is supported as specified in RFC6265
  if (!d)
    return;
  auto i = _cookieHeaderValue.globalMatch(rawHeaderValue);
  while (i.hasNext()) {
    auto m = i.next();
    auto name = m.captured(1).toUtf8(), value = m.captured(2).toUtf8();
    d->_cookies.insert(name, value);
  }
}

QByteArray HttpRequest::param(QByteArray key) const {
  // TODO better handle parameters, including POST and multi-valued params
  QByteArray value;
  if (d) {
    if (d->_paramsCache.contains(key))
      return d->_paramsCache.value(key);
    // note: + in values is replaced with space in HttpWorker::handleConnection()
    // so even if QUrl::FullyDecoded does not decode + it will be decoded anyway
    value = d->_query.queryItemValue(key, QUrl::FullyDecoded).toUtf8();
    d->_paramsCache.insert(key, value);
  }
  return value;
}

void HttpRequest::overrideParam(QByteArray key, QByteArray value) {
  if (d)
    d->_paramsCache.insert(key, value);
}

void HttpRequest::overrideUnsetParam(QByteArray key) {
  if (d)
    d->_paramsCache.insert(key, {});
}

ParamSet HttpRequest::paramsAsParamSet() const {
  if (!d)
    return {};
  cacheAllParams();
  return ParamSet(d->_paramsCache);
}

QMap<QByteArray,QByteArray> HttpRequest::paramsAsMap() const {
  if (d) {
    cacheAllParams();
    return d->_paramsCache;
  }
  return {};
}

void HttpRequest::cacheAllParams() const {
  if (!d)
    return;
  // note: + in values is replaced with space in HttpWorker::handleConnection()
  // so even if QUrl::FullyDecoded does not decode + it will be decoded anyway
  foreach (const auto &p, d->_query.queryItems(QUrl::FullyDecoded)) {
    auto key = p.first.toUtf8();
    if (!d->_paramsCache.contains(key))
      d->_paramsCache.insert(key, p.second.toUtf8());
  }
}

QByteArray HttpRequest::toUtf8() const {
  if (!d)
    return "HttpRequest{}"_ba;
  QByteArray s;
  s += "HttpRequest{ " + methodName() + ", " + url().toString().toUtf8()
      + ", { ";
  for (auto key: d->_headers.keys()) {
    s += key + ":{ ";
    for (auto value: d->_headers.values(key)) {
      s += value + " ";
    }
    s += "} ";
  }
  s += "} }";
  return s;
}

void HttpRequest::overrideUrl(QUrl url) {
  if (d) {
    d->_url = url;
    d->_query = QUrlQuery(url);
  }
}

QUrl HttpRequest::url() const {
  return d ? d->_url : QUrl();
}

QUrlQuery HttpRequest::urlQuery() const {
  return d ? d->_query : QUrlQuery();
}

QAbstractSocket *HttpRequest::input() {
  return d ? d->_input : 0;
}

void HttpRequest::setMethod(HttpMethod method) {
  if (d)
    d->_method = method;
}

HttpRequest::HttpMethod HttpRequest::method() const {
  return d ? d->_method : NONE;
}

QByteArray HttpRequest::header(
    const QByteArray &name, const QByteArray &defaultValue) const {
  // LATER handle case insensitivity in header names
  if (!d)
    return defaultValue;
  auto v = d->_headers.value(name);
  return v.isNull() ? defaultValue : v;
}

QByteArrayList HttpRequest::headers(const QByteArray &name) const {
  // LATER handle case insensitivity in header names
  return d ? d->_headers.values(name) : QByteArrayList{};
}

QMultiMap<QByteArray, QByteArray> HttpRequest::headers() const {
  return d ? d->_headers : QMultiMap<QByteArray,QByteArray>{};
}

QByteArray HttpRequest::cookie(
    const QByteArray &name, const QByteArray &defaultValue) const {
  if (!d)
    return defaultValue;
  const auto v = d->_cookies.value(name);
  return v.isNull() ? defaultValue : v;
}

QByteArray HttpRequest::base64Cookie(
    const QByteArray &name, const QByteArray &defaultValue) const {
  if (!d)
    return defaultValue;
  const auto v = d->_cookies.value(name);
  return v.isNull() ? defaultValue : QByteArray::fromBase64(v);
}

QMap<QByteArray,QByteArray> HttpRequest::cookies() const {
  return d ? d->_cookies : QMap<QByteArray,QByteArray>{};
}

QByteArrayList HttpRequest::clientAdresses() const {
  if (!d)
    return {};
  if (d->_clientAdresses.isEmpty()) {
    auto xff = headers(_xffHeader);
    for (int i = xff.size()-1; i >= 0; --i) {
      auto addresses = xff[i].split(',');
      for (auto a: addresses)
        d->_clientAdresses += a.trimmed();
    }
    QHostAddress peerAddress = d->_input->peerAddress();
    if (peerAddress.isNull()) {
      Log::debug() << "HttpRequest::clientAdresses() cannot find socket peer "
                      "address";
      d->_clientAdresses.append("0.0.0.0");
    } else
      d->_clientAdresses.append(peerAddress.toString().toUtf8());
  }
  return d->_clientAdresses;
}

const QVariant HttpRequestPseudoParamsProvider::paramValue(
  const QString &key, const ParamsProvider *, const QVariant &defaultValue,
  QSet<QString> *) const {
  if (key.startsWith('!')) {
    if (key == "!url") {
      return _request.url().toString(QUrl::RemovePassword);
    } else if (key == "!method") {
      return _request.methodName();
    } else if (key == "!clientaddresses") {
      return _request.clientAdresses().join(' ');
    } else if (key.startsWith("!cookie")) {
      return _request.cookie(key.mid(8).toUtf8());
    } else if (key.startsWith("!base64cookie")) {
      return _request.base64Cookie(key.mid(14).toUtf8());
    } else if (key.startsWith("!param")) {
      return _request.param(key.mid(7).toUtf8());
    } else if (key.startsWith("!header")) {
      return _request.header(key.mid(8).toUtf8());
    }
  }
  return defaultValue;
}

const QSet<QString> HttpRequestPseudoParamsProvider::keys() const {
  QSet<QString> keys { "!url", "!method", "!clientaddresses" };
  for (auto s: _request.cookies().keys())
    keys << "!cookie:"+s;
  for (auto s: _request.paramsAsMap().keys())
    keys << "!param:"+s;
  for (auto s: _request.headers().keys())
    keys << "!header:"+s;
  return keys;
}
