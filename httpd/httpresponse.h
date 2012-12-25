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
  QString _contentType;
  qint64 _contentLength; // -1 means unknown
  int _status;
  bool _headersSent;

public:
  HttpResponse(QAbstractSocket *output);
  /** Get the output to write content; calling this method triggers sending
    * the response status and headers.
    */
  QAbstractSocket *output();
  inline void setContentType(const QString &type) { _contentType = type; }
  inline void setContentType(const char *type) { _contentType = type; }
  inline const QString &contentType() const { return _contentType; }
  inline void setContentLength(qint64 length) { _contentLength = length; }
  inline qint64 contentLength() const { return _contentLength; }
  inline void setStatus(int status) { _status = status; }
  inline int status() const { return _status; }
  // LATER handle headers, redirect, cookies, session?
};

#endif // HTTPRESPONSE_H
