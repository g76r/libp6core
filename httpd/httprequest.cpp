/* Copyright 2012 Hallowyn and others.
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
