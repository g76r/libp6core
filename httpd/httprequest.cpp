/* Copyright 2012-2024 Hallowyn, Gregoire Barbier and others.
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
#include "httpcommon.h"
#include "util/radixtree.h"
#include "util/containerutils.h"
#include "format/stringutils.h"
#include <QHostAddress>
#include <QRegularExpression>

using EvalContext = ParamsProvider::EvalContext;

static Utf8String _xffHeader;

static int staticInit() {
  qRegisterMetaType<HttpRequest>();
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

namespace {
struct XffHeaderInitializer {
  XffHeaderInitializer() {
    Utf8String header = qgetenv("LIBPUMPKIN_X_FORWARDED_FOR_HEADER");
    if (header.isNull())
      header = "X-Forwarded-For";
    _xffHeader = header.toInternetHeaderCase();
  }
} xffHeaderInitializer;
}

class HttpRequestData : public QSharedData {
public:
  QAbstractSocket *_input;
  HttpRequest::HttpMethod _method;
  QMultiMap<Utf8String,Utf8String> _headers;
  QMap<Utf8String,Utf8String> _cookies, _paramsCache;
  QUrl _url;
  QUrlQuery _query;
  Utf8StringList _clientAdresses;
  Utf8String _scope = "http"_u8, _path;
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

static QMap<HttpRequest::HttpMethod,Utf8String> _methodToText {
  { HttpRequest::NONE, "NONE" },
  { HttpRequest::HEAD, "HEAD" },
  { HttpRequest::GET, "GET" },
  { HttpRequest::POST, "POST" },
  { HttpRequest::PUT, "PUT" },
  { HttpRequest::DELETE, "DELETE" },
  { HttpRequest::OPTIONS, "OPTIONS" },
  { HttpRequest::ANY, "ANY" },
};

static RadixTree<HttpRequest::HttpMethod> _methodFromText =
    RadixTree<HttpRequest::HttpMethod>::reversed(_methodToText);

QSet<HttpRequest::HttpMethod> HttpRequest::_wellKnownMethods {
  HttpRequest::HEAD, HttpRequest::GET, HttpRequest::POST, HttpRequest::PUT,
      HttpRequest::DELETE, HttpRequest::OPTIONS,
};

Utf8StringSet HttpRequest::_wellKnownMethodNames = []() {
  Utf8StringSet set;
  for (HttpRequest::HttpMethod m : HttpRequest::_wellKnownMethods)
    set.insert(HttpRequest::methodName(m));
  return set;
}();

Utf8String HttpRequest::methodName(HttpMethod method) {
  return _methodToText.value(method, "UNKNOWN"_u8);
}

HttpRequest::HttpMethod HttpRequest::methodFromText(const Utf8String &name) {
  return _methodFromText.value(name, NONE);
}

bool HttpRequest::parseAndAddHeader(Utf8String rawHeader) {
  if (!d)
    return false;
  int i = rawHeader.indexOf(':');
  if (i == -1)
    return false;
  // TODO support multi-line headers
  auto key = rawHeader.left(i).trimmed().toInternetHeaderCase();
  auto value = rawHeader.right(rawHeader.size()-i-1).trimmed();
  //qDebug() << "header:" << rawHeader << key << value;
  d->_headers.insert(key, value);
  if (key == "Cookie"_u8)
    parseAndAddCookie(value);
  return true;
}

// TODO use QRegularExpression instead, but not without regression/unit testing
static const QRegularExpression _cookieHeaderValue(
      "\\s*;?\\s*(" RFC2616_TOKEN_OCTET_RE "*)\\s*=\\s*(("
      RFC6265_COOKIE_OCTET_RE "*|\"" RFC6265_COOKIE_OCTET_RE
      "+\"))\\s*;?\\s*");

void HttpRequest::parseAndAddCookie(Utf8String rawHeaderValue) {
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

Utf8String HttpRequest::param(Utf8String key) const {
  // TODO better handle parameters, including POST and multi-valued params
  Utf8String value;
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

void HttpRequest::overrideParam(Utf8String key, Utf8String value) {
  if (d)
    d->_paramsCache.insert(key, value);
}

void HttpRequest::overrideUnsetParam(Utf8String key) {
  if (d)
    d->_paramsCache.insert(key, {});
}

ParamSet HttpRequest::paramsAsParamSet() const {
  if (!d)
    return {};
  cacheAllParams();
  return ParamSet(d->_paramsCache);
}

QMap<Utf8String,Utf8String> HttpRequest::paramsAsMap() const {
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
  for (auto p: d->_query.queryItems(QUrl::FullyDecoded)) {
    auto key = p.first.toUtf8();
    if (!d->_paramsCache.contains(key))
      d->_paramsCache.insert(key, p.second.toUtf8());
  }
}

Utf8String HttpRequest::toUtf8() const {
  if (!d)
    return "HttpRequest{}"_u8;
  Utf8String s;
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
    d->_path = url.path().toUtf8();
  }
}

Utf8String HttpRequest::path() const {
  return d ? d->_path : Utf8String{};
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

Utf8String HttpRequest::header(
    const Utf8String &name, const Utf8String &defaultValue) const {
  // LATER handle case insensitivity in header names
  if (!d)
    return defaultValue;
  auto v = d->_headers.value(name.toInternetHeaderCase());
  return v.isNull() ? defaultValue : v;
}

Utf8StringList HttpRequest::headers(const Utf8String &name) const {
  // LATER handle case insensitivity in header names
  return d ? d->_headers.values(name.toInternetHeaderCase()) : Utf8StringList{};
}

QMultiMap<Utf8String, Utf8String> HttpRequest::headers() const {
  return d ? d->_headers : QMultiMap<Utf8String,Utf8String>{};
}

Utf8String HttpRequest::cookie(
    const Utf8String &name, const Utf8String &defaultValue) const {
  if (!d)
    return defaultValue;
  const auto v = d->_cookies.value(name);
  return v.isNull() ? defaultValue : v;
}

QByteArray HttpRequest::base64Cookie(
    const Utf8String &name, const QByteArray &defaultValue) const {
  if (!d)
    return defaultValue;
  const auto v = d->_cookies.value(name);
  return v.isNull() ? defaultValue : Utf8String::fromBase64(v);
}

QMap<Utf8String,Utf8String> HttpRequest::cookies() const {
  return d ? d->_cookies : QMap<Utf8String,Utf8String>{};
}

Utf8StringList HttpRequest::clientAdresses() const {
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

static RadixTree <std::function<QVariant(const HttpRequest *req, const Utf8String &key, const EvalContext &context, int ml)>> _functions {
{ "url", [](const HttpRequest *req, const Utf8String &, const EvalContext&, int) -> QVariant {
  return "http://"_u8+req->header("Host"_u8)+req->url().toString(QUrl::RemoveScheme|QUrl::RemoveAuthority).toUtf8();
}},
{ "path", [](const HttpRequest *req, const Utf8String &, const EvalContext&, int) -> QVariant {
  return req->path();
}},
{ "method", [](const HttpRequest *req, const Utf8String &, const EvalContext&, int) -> QVariant {
  return req->methodName();
}},
{ "clientaddresses", [](const HttpRequest *req, const Utf8String &, const EvalContext&, int) -> QVariant {
  return req->clientAdresses().join(' ');
}},
{ "cookie", [](const HttpRequest *req, const Utf8String &key, const EvalContext&, int ml) -> QVariant {
  return req->cookie(key.mid(ml+1));
}, true},
{ "base64cookie", [](const HttpRequest *req, const Utf8String &key, const EvalContext&, int ml) -> QVariant {
  return req->base64Cookie(key.mid(ml+1));
}, true},
{ "param", [](const HttpRequest *req, const Utf8String &key, const EvalContext&, int ml) -> QVariant {
  return req->param(key.mid(ml+1));
}, true},
{ "value", [](const HttpRequest *req, const Utf8String &key, const EvalContext&, int ml) -> QVariant {
  auto v = req->param(key.mid(ml+1));
  if (!v.isNull())
    return v;
  return req->base64Cookie(key.mid(ml+1));
}, true},
{ { "header", "requestheader" }, [](const HttpRequest *req, const Utf8String &key, const EvalContext&, int ml) -> QVariant {
  return req->header(key.mid(ml+1).toInternetHeaderCase());
}, true},
};

QVariant HttpRequest::paramRawValue(
    const Utf8String &key, const QVariant &def,
    const EvalContext &context) const {
  if (!context.hasScopeOrNone(paramScope()))
    return def;
  int ml;
  auto f = _functions.value(key, &ml);
  if (f)
    return f(this, key, context, ml);
  return def;
}

Utf8StringSet HttpRequest::paramKeys(
    const EvalContext &context) const {
  if (!context.hasScopeOrNone(paramScope()))
    return {};
  static const Utf8StringSet _const_keys {
    "url"_u8, "path"_u8, "method"_u8, "clientaddresses"_u8 };
  Utf8StringSet keys = _const_keys;
  for (auto [s,_]: cookies().asKeyValueRange())
    keys << "cookie:"_u8+s;
  for (auto [s,_]: paramsAsMap().asKeyValueRange())
    keys << "param:"_u8+s;
  for (auto [s,_]: headers().asKeyValueRange()) {
    keys << "header:"_u8+s;
    keys << "requestheader:"_u8+s;
  }
  return keys;
}

Utf8String HttpRequest::paramScope() const {
  return d ? d->_scope : Utf8String{};
}
