/* Copyright 2013 Hallowyn and others.
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
#include "httprequestcontext.h"

class HttpRequestContextData : public QSharedData {
public:
  QString _scope;
  QList<ParamsProvider*> _params;
  HttpRequestContextData() { }
  HttpRequestContextData(QString scope) : _scope(scope) { }
  HttpRequestContextData(ParamsProvider *params) { _params.append(params); }
};

HttpRequestContext::HttpRequestContext() : d(new HttpRequestContextData) {
}

HttpRequestContext::HttpRequestContext(const HttpRequestContext &other)
  : d(other.d) {
}

HttpRequestContext::HttpRequestContext(QString scope)
  : d(new HttpRequestContextData(scope)) {
}

HttpRequestContext::HttpRequestContext(ParamsProvider *params)
  : d(new HttpRequestContextData(params)) {
}

HttpRequestContext::~HttpRequestContext() {
}

HttpRequestContext &HttpRequestContext::operator=(
    const HttpRequestContext &other) {
  if (this != &other)
    d = other.d;
  return *this;
}

/*bool HttpRequestContext::operator==(const HttpRequestContext &other) const {
  return d == other.d;
}*/

QVariant HttpRequestContext::paramValue(const QString key,
                                        const QVariant defaultValue) const {
  foreach (ParamsProvider *params, d->_params)
    if (params) {
      QVariant v = params->paramValue(key);
      if (v.isValid())
        return v;
    }
  return defaultValue;
}

HttpRequestContext &HttpRequestContext::appendParamsProvider(
    ParamsProvider *params) {
  d->_params.append(params);
  return *this;
}

HttpRequestContext &HttpRequestContext::prependParamsProvider(
    ParamsProvider *params) {
  d->_params.prepend(params);
  return *this;
}

QString HttpRequestContext::scope() const {
  return d->_scope;
}

HttpRequestContext &HttpRequestContext::setScope(QString scope) {
  d->_scope = scope;
  return *this;
}
