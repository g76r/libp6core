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
#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "libp6core_global.h"
#include "util/paramset.h"
#include <QAbstractSocket>
#include <QUrlQuery>

class HttpRequestData;
class HttpWorker;

/** Class holding all information and actions about an HTTP incoming request.
 * This class uses Qt explicit sharing idiom, i.e. it can be copied for a
 * very low cost in thread-safe manner, however it must not be accessed from
 * several threads at a time.
 */
class LIBP6CORESHARED_EXPORT HttpRequest : public ParamsProvider {
public:
  enum HttpMethod : signed char {
    NONE = 0,
    HEAD = 1,
    GET = 2,
    POST = 4,
    PUT = 8,
    DELETE = 16,
    OPTIONS = 32,
    ANY = -1,
  };

private:
  QExplicitlySharedDataPointer<HttpRequestData> d;
  static QSet<HttpMethod> _wellKnownMethods;
  static Utf8StringSet _wellKnownMethodNames;

public:
  HttpRequest(QAbstractSocket *input, HttpWorker *worker);
  HttpRequest();
  HttpRequest(const HttpRequest &other);
  ~HttpRequest();
  HttpRequest &operator=(const HttpRequest &other);
  [[nodiscard]] QAbstractSocket *input();
  void setMethod(HttpMethod method);
  [[nodiscard]] HttpRequest::HttpMethod method() const;
  /** @return protocol and human readable string, e.g. "GET" */
  [[nodiscard]] Utf8String methodName() const { return methodName(method()); }
  /** @return protocol and human readable string, e.g. "GET" */
  [[nodiscard]] static Utf8String methodName(HttpMethod method);
  /** @return enum from protocol and human readable string, e.g. "GET"
   * @param name case sensitive, must be upper case */
  [[nodiscard]] static HttpMethod methodFromText(const Utf8String &name);
  [[nodiscard]] static QSet<HttpMethod> wellKnownMethods() {
    return _wellKnownMethods; }
  [[nodiscard]] static Utf8StringSet wellKnownMethodNames() {
    return _wellKnownMethodNames; }
  [[nodiscard]] bool parseAndAddHeader(Utf8String rawHeader);
  /** Value associated to a request header.
   * If the header is found several time, last value is returned. */
  [[nodiscard]] Utf8String header(
      const Utf8String &name, const Utf8String &defaultValue = {}) const;
  /** Values associated to a request header, last occurrence first. */
  [[nodiscard]] Utf8StringList headers(const Utf8String &name) const;
  /** Full header hash */
  [[nodiscard]] QMultiMap<Utf8String,Utf8String> headers() const;
  /* Value of a given cookie, as is. */
  [[nodiscard]] Utf8String cookie(
      const Utf8String &name, const Utf8String &defaultValue = {}) const;
  /* Value of a given cookie, decoded from base64 (implies that the cookie
   * content was encoded using base64). */
  [[nodiscard]] QByteArray base64Cookie(
      const Utf8String &name, const QByteArray &defaultValue = {}) const;
  [[nodiscard]] QMap<Utf8String,Utf8String> cookies() const;
  /** Replace url. If params have already been queried and new url has
   * different query items than former one, one should also call
   * discardParamsCache(). */ // LATER this behaviour is optimisable since Qt5
  void overrideUrl(QUrl url);
  [[nodiscard]] Utf8String path() const;
  [[nodiscard]] QUrl url() const;
  [[nodiscard]] QUrlQuery urlQuery() const;
  /** Return an url param (query item) value.
   * Only first value of multi-valued items is kept. */
  // LATER manage to keep last value instead
  [[nodiscard]] Utf8String param(Utf8String key) const;
  void overrideParam(Utf8String key, Utf8String value);
  void overrideUnsetParam(Utf8String key);
  /** Retrieve url params (query items) as a ParamSet.
   * Only first value of multi-valued items is kept. */
  // LATER manage to keep last value instead
  [[nodiscard]] ParamSet paramsAsParamSet() const;
  [[nodiscard]] QMap<Utf8String,Utf8String> paramsAsMap() const;
  [[nodiscard]] Utf8String toUtf8() const;
  /** Client addresses.
   * Contains only one address for direct connection, or several when acceeded
   * through (reverse) proxies.
   * Same as X-Forwarded-For content, plus socket peer address at the end of
   * the list. */
  [[nodiscard]] Utf8StringList clientAdresses() const;
  /** Create a ParamsProvider wrapper object to give access to ! pseudo params,
   * url params (query items) and base64 cookies, in this order (url params hide
   * cookies). */
  using ParamsProvider::paramRawValue;
  /** Expose as a ParamsProvider the following data/metadata:
   *  - url URL without password e.g. "http://foobar.io/baz?a=b"
   *  - method e.g. "GET"
   *  - clientaddresses e.g. "127.0.0.1 1.2.3.4"
   *  - param:xxx e.g. param:a -> "b" (works also with POST params)
   *  - header:xxx e.g. header:Host -> "foobar.io"
   *  - requestheader:xxx e.g. requestheader:Host -> "foobar.io"
   *  - cookie:xxx content of xxx cookie
   *  - base64cookie:xxx content of xxx cookie, decoded as base64
   *  - value:xxx take value from param xxx if set, otherwise base64 cookie xxx
   */
  [[nodiscard]] QVariant paramRawValue(
      const Utf8String &key, const QVariant &def = {},
      const EvalContext &context = {}) const override;
  using ParamsProvider::paramKeys;
  [[nodiscard]] Utf8StringSet paramKeys(
      const EvalContext &context = {}) const override;
  using ParamsProvider::paramScope;
  /** Default: "http" */
  [[nodiscard]] Utf8String paramScope() const override;
  /** Set param scope to something else than the default "http". */
  HttpRequest &setScope(const Utf8String &scope);
  HttpWorker *worker() const;
  // LATER handle sessions

private:
  inline void parseAndAddCookie(Utf8String rawHeaderValue);
  inline void cacheAllParams() const;
};

Q_DECLARE_METATYPE(HttpRequest)
Q_DECLARE_TYPEINFO(HttpRequest, Q_RELOCATABLE_TYPE);

#endif // HTTPREQUEST_H
