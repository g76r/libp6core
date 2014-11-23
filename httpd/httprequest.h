/* Copyright 2012-2014 Hallowyn and others.
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
#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QStringList>
#include <QMultiHash>
#include <QUrl>
#include <QUrlQuery>
#include <QAbstractSocket>
#include "libqtssu_global.h"
#include <QExplicitlySharedDataPointer>
#include "util/paramset.h"

class HttpRequestData;

/** Class holding all information and actions about an HTTP incoming request.
 * This class uses Qt explicit sharing idiom, i.e. it can be copied for a
 * very low cost in thread-safe manner, however it must not be accessed from
 * several threads at a time.
 */
// LATER implement ParamsProviders, giving either queryItems or cookies values
class LIBQTSSUSHARED_EXPORT HttpRequest {
public:
  enum HttpRequestMethod { NONE = 0, HEAD = 1, GET = 2, POST = 4, PUT = 8,
                           DELETE = 16, ANY = 0xffff} ;

private:
  QExplicitlySharedDataPointer<HttpRequestData> d;

public:
  HttpRequest(QAbstractSocket *input);
  HttpRequest();
  HttpRequest(const HttpRequest &other);
  ~HttpRequest();
  HttpRequest &operator=(const HttpRequest &other);
  QAbstractSocket *input();
  void setMethod(HttpRequestMethod method);
  HttpRequest::HttpRequestMethod method() const;
  /** @return protocol and human readable string, e.g. "GET" */
  inline QString methodName() const;
  /** @return protocol and human readable string, e.g. "GET" */
  static QString methodName(HttpRequestMethod method);
  bool parseAndAddHeader(QString rawHeader);
  /** Value associated to a request header.
   * If the header is found several time, last value is returned. */
  QString header(QString name, QString defaultValue = QString()) const;
  /** Values associated to a request header, last occurrence first. */
  QStringList headers(QString name) const;
  /** Full header hash */
  QMultiHash<QString,QString> headers() const;
  /* Value of a given cookie, as is. */
  QString cookie(QString name, QString defaultValue = QString()) const;
  /* Value of a given cookie, decoded from base64 (implies that the cookie
   * content was encoded using base64 and utf-8). */
  QString base64Cookie(QString name, QString defaultValue = QString()) const;
  /* Value of a given cookie, decoded from base64 (implies that the cookie
   * content was encoded using base64). */
  QByteArray base64BinaryCookie(QString name,
                                QByteArray defaultValue = QByteArray()) const;
  /** Replace url. If params have already been queried and new url has
   * different query items than former one, one should also call
   * discardParamsCache(). */ // LATER this behaviour is optimisable since Qt5
  void overrideUrl(QUrl url);
  QUrl url() const;
  QUrlQuery urlQuery() const;
  QString param(QString key) const;
  void overrideParam(QString key, QString value);
  void overrideUnsetParam(QString key);
  /** Discard params cache built by calls to param(). These also discard any
   * overiding done on params. */
  void discardParamsCache();
  /** Retrieve request parameters as a ParamSet.
   * Only first value of multi-valued parameters is kept. */
  ParamSet paramsAsParamSet() const;
  operator QString() const;
  /** Client addresses.
   * Contains only one address for direct connection, or several when acceeded
   * through (reverse) proxies.
   * Same as X-Forwarded-For content, plus socket peer address at the end of
   * the list. */
  QStringList clientAdresses() const;
  // LATER handle sessions

private:
  void parseAndAddCookie(QString rawHeaderValue);
  void cacheAllParams() const;
};

#endif // HTTPREQUEST_H
