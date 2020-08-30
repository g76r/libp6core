/* Copyright 2012-2019 Hallowyn, Gregoire Barbier and others.
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
#include <QRegExp>
#include "httpcommon.h"
#include <QSharedData>
#include <QHostAddress>
#include <QHash>
#include "util/radixtree.h"
#include "util/containerutils.h"
#include "format/stringutils.h"

class HttpRequestData : public QSharedData {
public:
  QAbstractSocket *_input;
  HttpRequest::HttpRequestMethod _method;
  QMultiHash<QString,QString> _headers;
  QHash<QString,QString> _cookies, _paramsCache;
  QUrl _url;
  QUrlQuery _query;
  QStringList _clientAdresses;
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

static QHash<HttpRequest::HttpRequestMethod,QString> _methodToText {
  { HttpRequest::NONE, "NONE" },
  { HttpRequest::HEAD, "HEAD" },
  { HttpRequest::GET, "GET" },
  { HttpRequest::POST, "POST" },
  { HttpRequest::PUT, "PUT" },
  { HttpRequest::DELETE, "DELETE" },
  { HttpRequest::OPTIONS, "OPTIONS" },
  { HttpRequest::ANY, "ANY" },
};

static QHash<QString,HttpRequest::HttpRequestMethod> _methodFromText {
  ContainerUtils::reversed(_methodToText)
};

QString HttpRequest::methodName(HttpRequestMethod method) {
  return _methodToText.value(method, QStringLiteral("UNKNOWN"));
}

HttpRequest::HttpRequestMethod HttpRequest::methodFromText(QString name) {
  return _methodFromText.value(name, NONE);
}

bool HttpRequest::parseAndAddHeader(QString rawHeader) {
  if (!d)
    return false;
  int i = rawHeader.indexOf(':');
  if (i == -1)
    return false;
  // MAYDO remove special chars from keys and values?
  // TODO support multi-line headers
  QString key = StringUtils::normalizeRfc841HeaderCase(
              rawHeader.left(i).trimmed());
  QString value = rawHeader.right(rawHeader.size()-i-1).trimmed();
  //qDebug() << "header:" << rawHeader << key << value;
  d->_headers.insert(key, value);
  if (key.compare("Cookie", Qt::CaseInsensitive) == 0)
    parseAndAddCookie(value);
  return true;
}

// TODO use QRegularExpression instead, but not without regression/unit testing
static const QRegExp cookieHeaderValue(
      "\\s*;?\\s*(" RFC2616_TOKEN_OCTET_RE "*)\\s*=\\s*(("
      RFC6265_COOKIE_OCTET_RE "*|\"" RFC6265_COOKIE_OCTET_RE
      "+\"))\\s*;?\\s*");

void HttpRequest::parseAndAddCookie(QString rawHeaderValue) {
  // LATER use QNetworkCookie::parseCookies
  // LATER ensure that utf8 is supported as specified in RFC6265
  if (!d)
    return;
  QRegExp re(cookieHeaderValue);
  int pos = 0;
  //qDebug() << "parseAndAddCookie" << rawHeaderValue;
  while ((re.indexIn(rawHeaderValue, pos)) != -1) {
    const QString name = re.cap(1), value = re.cap(2);
    //qDebug() << "  " << name << value << pos;
    d->_cookies.insert(name, value);
    pos += re.matchedLength();
  }
}

QString HttpRequest::param(QString key) const {
  // TODO better handle parameters, including POST and multi-valued params
  QString value;
  if (d) {
    if (d->_paramsCache.contains(key))
      return d->_paramsCache.value(key);
    value = d->_query.queryItemValue(key, QUrl::FullyDecoded);
    d->_paramsCache.insert(key, value);
  }
  return value;
}

void HttpRequest::overrideParam(QString key, QString value) {
  if (d)
    d->_paramsCache.insert(key, value);
}

void HttpRequest::overrideUnsetParam(QString key) {
  if (d)
    d->_paramsCache.insert(key, QString());
}

ParamSet HttpRequest::paramsAsParamSet() const {
  if (d) {
    cacheAllParams();
    return d->_paramsCache;
  } else
    return ParamSet();
}

void HttpRequest::cacheAllParams() const {
  if (!d)
    return;
  foreach (const auto &p, d->_query.queryItems(QUrl::FullyDecoded))
    if (!d->_paramsCache.contains(p.first))
      d->_paramsCache.insert(p.first, p.second);
}

HttpRequest::operator QString() const {
  if (!d)
    return QStringLiteral("HttpRequest{}");
  QString s;
  QTextStream ts(&s, QIODevice::WriteOnly);
  ts << "HttpRequest{ " << methodName() << ", " << url().toString() << ", { ";
  foreach (QString key, d->_headers.keys()) {
    ts << key << ":{ ";
    foreach (QString value, d->_headers.values(key)) {
      ts << value << " ";
    }
    ts << "} ";
  }
  ts << "} }";
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

void HttpRequest::setMethod(HttpRequestMethod method) {
  if (d)
    d->_method = method;
}

HttpRequest::HttpRequestMethod HttpRequest::method() const {
  return d ? d->_method : NONE;
}

QString HttpRequest::header(QString name, QString defaultValue) const {
  // LATER handle case insensitivity in header names
  if (!d)
    return defaultValue;
  const QString v = d->_headers.value(name);
  return v.isNull() ? defaultValue : v;
}

QStringList HttpRequest::headers(QString name) const {
  // LATER handle case insensitivity in header names
  return d ? d->_headers.values(name) : QStringList();
}

QMultiHash<QString, QString> HttpRequest::headers() const {
  return d ? d->_headers : QMultiHash<QString,QString>();
}

QString HttpRequest::cookie(QString name, QString defaultValue) const {
  if (!d)
    return defaultValue;
  const QString v = d->_cookies.value(name);
  return v.isNull() ? defaultValue : v;
}

QString HttpRequest::base64Cookie(QString name, QString defaultValue) const {
  if (!d)
    return defaultValue;
  const QString v = d->_cookies.value(name);
  return v.isNull() ? defaultValue
                    : QString::fromUtf8(QByteArray::fromBase64(v.toLatin1()));
}

QByteArray HttpRequest::base64BinaryCookie(QString name,
                                           QByteArray defaultValue) const {
  if (!d)
    return defaultValue;
  const QString v = d->_cookies.value(name);
  return v.isNull() ? defaultValue
                    : QByteArray::fromBase64(cookie(name).toLatin1());
}

static QRegExp xffSeparator("\\s*,\\s*");
static QString xffHeader;

namespace {
struct XffHeaderInitializer {
  XffHeaderInitializer() {
    QByteArray header = qgetenv("LIBPUMPKIN_X_FORWARDED_FOR_HEADER");
    if (header.isNull())
      header = "X-Forwarded-For";
    xffHeader = QString::fromUtf8(header);
  }
} xffHeaderInitializer;
}

QStringList HttpRequest::clientAdresses() const {
  if (!d)
    return QStringList();
  if (d->_clientAdresses.isEmpty()) {
    QStringList xff = headers(xffHeader);
    for (int i = xff.size()-1; i >= 0; --i) {
      const QString &oneHeader = xff[i];
      d->_clientAdresses.append(oneHeader.split(xffSeparator));
    }
    QHostAddress peerAddress = d->_input->peerAddress();
    if (peerAddress.isNull()) {
      Log::debug() << "HttpRequest::clientAdresses() cannot find socket peer "
                      "address";
      d->_clientAdresses.append("0.0.0.0");
    } else
      d->_clientAdresses.append(peerAddress.toString());
  }
  return d->_clientAdresses;
}

QVariant HttpRequestPseudoParamsProvider::paramValue(
        QString key, const ParamsProvider *context, QVariant defaultValue,
        QSet<QString> alreadyEvaluated) const {
  Q_UNUSED(context)
  Q_UNUSED(alreadyEvaluated)
  if (key.startsWith('!')) {
    if (key == "!url") {
      return _request.url().toString(QUrl::RemovePassword);
    } else if (key == "!method") {
      return _request.methodName();
    } else if (key == "!clientaddresses") {
      return _request.clientAdresses().join(' ');
    } else if (key.startsWith("!cookie")) {
      return _request.base64Cookie(key.mid(8));
    } else if (key.startsWith("!param")) {
      return _request.param(key.mid(7));
    }
  }
  return defaultValue;
}
