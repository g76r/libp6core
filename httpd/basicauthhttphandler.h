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
#ifndef BASICAUTHHTTPHANDLER_H
#define BASICAUTHHTTPHANDLER_H

#include "httphandler.h"
#include "auth/authenticator.h"

/** Handler for HTTP Basic authentication scheme.
 * Check for Authorization header, challenge login/password against an
 * Authenticator and set userid in HttpRequestContext.
 * If auth is mandatory (which is the default), no or bad credentials will
 * lead to an HTTP 401 and will stop the handlers pipeline (handleRequest()
 * returning false).
 */
class LIBQTSSUSHARED_EXPORT BasicAuthHttpHandler : public HttpHandler {
  Q_OBJECT
  Q_DISABLE_COPY(BasicAuthHttpHandler)
  Authenticator *_authenticator;
  bool _ownAuthenticator, _authIsMandatory;
  QString _realm, _userIdContextParamName;
  ParamSet _authContext;

public:
  explicit BasicAuthHttpHandler(QObject *parent = 0);
  ~BasicAuthHttpHandler();
  bool acceptRequest(HttpRequest req);
  bool handleRequest(HttpRequest req, HttpResponse res,
                     ParamsProviderMerger *processingContext);
  BasicAuthHttpHandler &setAuthenticator(Authenticator *authenticator,
                                         bool takeOwnership);
  BasicAuthHttpHandler &setRealm(QString realm) {
    _realm = realm; _authContext.setValue("realm", realm); return *this; }
  /** Define which param name will be used to set the (principal) user id in
   * HttpRequestContext. Default is "userid". Null or empty string disable
   * setting any parameter in HttpRequestContext. */
  BasicAuthHttpHandler &setUserIdContextParamName(QString name) {
    _userIdContextParamName = name; return *this; }

public slots:
  /** If no or bad basic auth, request auth (HTTP 401) and stop pipeline
   * (handRequest() returns false).
   * Otherwise let the page be served with no userId in HttpRequestContext. */
  BasicAuthHttpHandler &setAuthIsMandatory(bool mandatory) {
    _authIsMandatory = mandatory; return *this; }
};

#endif // BASICAUTHHTTPHANDLER_H
