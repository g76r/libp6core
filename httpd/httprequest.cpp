/* Copyright 2012-2025 Hallowyn, Gregoire Barbier and others.
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
#include "httpcommon.h"
#include "httpworker.h"
#include "util/radixtree.h"
#include "util/containerutils.h"
#include "format/stringutils.h"
#include <QHostAddress>
#include <QRegularExpression>
#include <QPointer>

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
    Utf8String header = qEnvironmentVariable("P6_X_FORWARDED_FOR_HEADER");
    if (header.isEmpty())
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
  QMap<Utf8String,Utf8String> _cookies, _params;
  Utf8StringList _clientAdresses;
  Utf8String _url, _scope = "http"_u8, _path;
  QPointer<HttpWorker> _worker;
  HttpRequestData(QAbstractSocket *input, HttpWorker *worker)
    : _input(input), _method(HttpRequest::NONE), _worker(worker) {}
};

HttpRequest::HttpRequest(QAbstractSocket *input, HttpWorker *worker)
  : d(new HttpRequestData(input, worker)) {
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

Utf8String HttpRequest::httpParam(const Utf8String &key,
                              const Utf8String &def) const {
  if (!d)
    return def;
  return d->_params.value(key, def);
}

void HttpRequest::setHttpParam(const Utf8String &key, const Utf8String &value) {
  if (!d)
    return;
  d->_params.insert(key, value);
}

void HttpRequest::unsetHttpParam(const Utf8String &key) {
  if (!d)
    return;
  d->_params.insert(key, {});
}

ParamSet HttpRequest::httpParamset() const {
  if (!d)
    return {};
  return ParamSet(d->_params);
}

QMap<Utf8String,Utf8String> HttpRequest::httpParams() const {
  if (!d)
    return {};
  return d->_params;
}

Utf8String HttpRequest::toUtf8() const {
  if (!d)
    return "HttpRequest{}"_u8;
  Utf8String s;
  s += "HttpRequest{ " + methodName() + ", " + url()
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

void HttpRequest::setUrl(const Utf8String &url) {
  if (d)
    d->_url = url;
}

Utf8String HttpRequest::path() const {
  return d ? d->_path : Utf8String{};
}

void HttpRequest::setPath(const Utf8String &path) {
  if (d)
    d->_path = path;
}

Utf8String HttpRequest::url() const {
  return d ? d->_url : Utf8String{};
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
  return "http://"_u8+req->header("Host"_u8)+req->url();
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
{ "cookie:", [](const HttpRequest *req, const Utf8String &key, const EvalContext&, int ml) -> QVariant {
  return req->cookie(key.mid(ml));
}, true},
{ "base64cookie:", [](const HttpRequest *req, const Utf8String &key, const EvalContext&, int ml) -> QVariant {
  return req->base64Cookie(key.mid(ml));
}, true},
{ "param:", [](const HttpRequest *req, const Utf8String &key, const EvalContext&, int ml) -> QVariant {
  return req->httpParam(key.mid(ml));
}, true},
{ "value:", [](const HttpRequest *req, const Utf8String &key, const EvalContext&, int ml) -> QVariant {
  auto v = req->httpParam(key.mid(ml));
  if (!v.isNull())
    return v;
  return req->base64Cookie(key.mid(ml));
}, true},
{ { "header:", "requestheader:" }, [](const HttpRequest *req, const Utf8String &key, const EvalContext&, int ml) -> QVariant {
  return req->header(key.mid(ml).toInternetHeaderCase());
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
  auto v = httpParam(key);
  if (!v.isNull())
    return v;
  v = base64Cookie(key);
  if (!v.isNull())
    return v;
  v = header(key);
  if (!v.isNull())
    return v;
  return def;
}

Utf8StringSet HttpRequest::paramKeys(
    const EvalContext &context) const {
  if (!context.hasScopeOrNone(paramScope()))
    return {};
  static const Utf8StringSet _const_keys {
    "url"_u8, "path"_u8, "method"_u8, "clientaddresses"_u8 };
  Utf8StringSet keys = _const_keys;
  for (auto [s,_]: cookies().asKeyValueRange()) {
    keys << "cookie:"_u8+s;
    keys << s;
  }
  for (auto [s,_]: httpParams().asKeyValueRange()) {
    keys << "param:"_u8+s;
    keys << s;
  }
  for (auto [s,_]: headers().asKeyValueRange()) {
    keys << "header:"_u8+s;
    keys << "requestheader:"_u8+s;
    keys << s;
  }
  return keys;
}

Utf8String HttpRequest::paramScope() const {
  return d ? d->_scope : Utf8String{};
}

HttpWorker *HttpRequest::worker() const {
  return d ? d->_worker.data() : nullptr;
}
