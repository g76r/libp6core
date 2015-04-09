/* Copyright 2013-2015 Hallowyn and others.
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
#ifndef HTTPREQUESTCONTEXT_H
#define HTTPREQUESTCONTEXT_H

#include <QSharedData>
#include "util/paramsprovider.h"

class HttpRequestContextData;

/** HTTP request context, which contain server-side enriched information
 * related to authentication, server-side session data, etc.
 * This class uses Qt's explicit sharing, therefore it is not thread-safe
 * but its modifications will be propagated to other copies (especially
 * usefull along a PipelineHttpHandler pipeline). */
class LIBQTSSUSHARED_EXPORT HttpRequestContext : public ParamsProvider {
  QExplicitlySharedDataPointer<HttpRequestContextData> d;

public:
  HttpRequestContext();
  HttpRequestContext(const HttpRequestContext &other);
  explicit HttpRequestContext(QString scope);
  explicit HttpRequestContext(ParamsProvider *params);
  ~HttpRequestContext();
  HttpRequestContext &operator=(const HttpRequestContext &other);
  bool operator==(const HttpRequestContext &other) const {
    return d == other.d; }
  QVariant paramValue(
      QString key, QVariant defaultValue = QVariant(),
      QSet<QString> alreadyEvaluated = QSet<QString>()) const;
  /** Parameters set through overrideParamValue() will override any
   * ParamsProvider, even those prepended. */
  HttpRequestContext &overrideParamValue(QString key, QString value);
  /** Parameters set through overrideParamValue() will override any
   * ParamsProvider, even those prepended. */
  inline HttpRequestContext &overrideParamValue(QString key, QVariant value) {
    return overrideParamValue(key, value.toString()); }
  /** Parameters set through overrideParamValue() will override any
   * ParamsProvider, even those prepended. */
  inline HttpRequestContext &overrideParamValue(QString key, const char *value){
    return overrideParamValue(key, QString(value)); }
  HttpRequestContext &appendParamsProvider(ParamsProvider *params);
  HttpRequestContext &prependParamsProvider(ParamsProvider *params);
  QString scope() const;
  HttpRequestContext &setScope(QString scope);
};

#endif // HTTPREQUESTCONTEXT_H
