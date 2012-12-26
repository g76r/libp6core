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
#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <QString>
#include <QAbstractSocket>
#include "libqtssu_global.h"

class LIBQTSSUSHARED_EXPORT HttpResponse {
private:
  QAbstractSocket *_output;
  int _status;
  bool _headersSent;
  QMultiMap<QString,QString> _headers;

public:
  HttpResponse(QAbstractSocket *output);
  /** Get the output to write content; calling this method triggers sending
    * the response status and headers.
    */
  QAbstractSocket *output();
  /** Syntaxic sugar for setHeader("Content-Type", type).
   * Default content type is "text/plain;charset=UTF-8".
   */
  inline void setContentType(const QString type) {
    setHeader("Content-Type", type); }
  /** Syntaxic sugar for setHeader("Content-Length", length).
   */
  inline void setContentLength(qint64 length) {
    setHeader("Content-Length", QString::number(length)); }
  /** Set HTTP status code. Default is 200.
   * Must be called before output().
   */
  void setStatus(int status);
  /** Replace any header of this name by one header with this value.
   * Must be called before output().
   */
  void setHeader(const QString name, const QString value);
  /** Append a header regardless one already exists with the same name.
   * Must be called before output().
   */
  void addHeader(const QString name, const QString value);
  inline const QMultiMap<QString,QString> headers() const { return _headers; }
  /** Respond with a temporary moved page (302).
   * Must be called before output().
   */
  void redirect(const QString location);
  // LATER cookies, session
};

#endif // HTTPRESPONSE_H
