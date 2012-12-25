/*
Copyright 2012 Hallowyn and others.
See the NOTICE file distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file to you under
the Apache License, Version 2.0 (the "License"); you may not use this
file except in compliance with the License. You may obtain a copy of the
License at http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations
under the License.
*/
#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QString>
#include <QMultiMap>
#include <QUrl>
#include <QAbstractSocket>
#include "libqtssu_global.h"

class LIBQTSSUSHARED_EXPORT HttpRequest {
public:
  enum HttpRequestMethod { NONE = 0, HEAD = 1, GET = 2, POST = 4, PUT = 8,
                           DELETE = 16, ANY = 0xffff} ;

private:
  QAbstractSocket *_input;
  HttpRequestMethod _method;
  QMultiMap<QString,QString> _headers;
  QUrl _url;

public:
  HttpRequest(QAbstractSocket *input);
  inline QAbstractSocket *input() { return _input; }
  inline void setMethod(HttpRequestMethod method) { _method = method; }
  inline HttpRequestMethod method() const { return _method; }
  QString methodName() const; // human readable, e.g. "GET"
  bool parseAndAddHeader(const QString &rawHeader);
  inline const QString header(const QString &name) const {
    return _headers.value(name); // if multiple, the last one is returned
    // whereas in the J2EE API this is the first one
  }
  inline const QList<QString> headers(const QString &name) const {
    return _headers.values(name);
  }
  inline void setUrl(const QUrl &url) { _url = url; }
  inline const QUrl &url() const { return _url; }
  QString param(const QString &key) const;
  operator QString() const;
  // LATER handle cookies and sessions
};

#endif // HTTPREQUEST_H
