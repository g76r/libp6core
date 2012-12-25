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
#include "httprequest.h"
#include <QtDebug>

HttpRequest::HttpRequest(QAbstractSocket *input) : _input(input),
  _method(NONE) {
}

QString HttpRequest::methodName() const {
  switch(_method) {
  case NONE:
    return "NONE";
  case HEAD:
    return "HEAD";
  case GET:
    return "GET";
  case POST:
    return "POST";
  case PUT:
    return "PUT";
  case DELETE:
    return "DELETE";
  case ANY:
    return "ANY";
  }
  return "UNKNOWN";
}

bool HttpRequest::parseAndAddHeader(const QString &rawHeader) {
  int i = rawHeader.indexOf(':');
  if (i == -1)
    return false;
  // MAYDO remove special chars from keys and values?
  // TODO support multi-line headers
  QStringRef key = rawHeader.leftRef(i);
  QStringRef value = rawHeader.rightRef(rawHeader.size()-i-1);
  //qDebug() << "header:" << rawHeader << key << value;
  _headers.insertMulti(key.toString().trimmed(), value.toString().trimmed());
  return true;
}

QString HttpRequest::param(const QString &key) const {
  // TODO better handle parameters, including POST parameters
  return _url.queryItemValue(key);
}

HttpRequest::operator QString() const {
  QString s;
  QTextStream ts(&s, QIODevice::WriteOnly);
  ts << "HttpRequest{ " << methodName() << ", " << url().toString() << ", { ";
  foreach (QString key, _headers.keys()) {
    ts << key << ":{ ";
    foreach (QString value, _headers.values(key)) {
      ts << value << " ";
    }
    ts << "} ";
  }
  ts << "} }";
  return s;
}
