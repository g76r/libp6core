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
#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QString>
#include <QMultiMap>
#include <QUrl>
#include <QAbstractSocket>
#include "libqtssu_global.h"

// LATER should be implicitly shared
class LIBQTSSUSHARED_EXPORT HttpRequest {
public:
  enum HttpRequestMethod { NONE = 0, HEAD = 1, GET = 2, POST = 4, PUT = 8,
                           DELETE = 16, ANY = 0xffff} ;

private:
  QAbstractSocket *_input;
  HttpRequestMethod _method;
  QMultiMap<QString,QString> _headers;
  QMap<QString,QString> _cookies;
  QUrl _url;

public:
  HttpRequest(QAbstractSocket *input);
  inline QAbstractSocket *input() { return _input; }
  inline void setMethod(HttpRequestMethod method) { _method = method; }
  inline HttpRequestMethod method() const { return _method; }
  QString methodName() const; // human readable, e.g. "GET"
  bool parseAndAddHeader(const QString rawHeader);
  inline const QString header(const QString name,
                              const QString defaultValue = QString()) const {
    const QString v = _headers.value(name);
    return v.isNull() ? defaultValue : v;
    // if multiple, the last one is returned
    // whereas in the J2EE API it's the first one
  }
  inline const QList<QString> headers(const QString name) const {
    return _headers.values(name); }
  inline const QMultiMap<QString,QString> headers() const {
    return _headers; }
  inline QString cookie(
      const QString name, const QString defaultValue = QString()) {
    const QString v = _cookies.value(name);
    return v.isNull() ? defaultValue : v;
  }
  inline QString base64Cookie(
      const QString name, const QString defaultValue = QString()) {
    const QString v = _cookies.value(name);
    return v.isNull() ? defaultValue
                      : QString::fromUtf8(QByteArray::fromBase64(v.toAscii()));
  }
  inline QByteArray base64BinaryCookie(
      const QString name, const QByteArray defaultValue = QByteArray()) {
    const QString v = _cookies.value(name);
    return v.isNull() ? defaultValue
                      : QByteArray::fromBase64(cookie(name).toAscii());
  }
  inline void setUrl(const QUrl &url) { _url = url; }
  inline const QUrl &url() const { return _url; }
  QString param(const QString &key) const;
  operator QString() const;
  // LATER handle cookies and sessions

private:
  void parseAndAddCookie(const QString rawHeaderValue);
};

#endif // HTTPREQUEST_H
