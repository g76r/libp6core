/* Copyright 2013-2025 Hallowyn, Gregoire Barbier and others.
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
#ifndef BASICAUTHHTTPHANDLER_H
#define BASICAUTHHTTPHANDLER_H

#include "httphandler.h"
#include "auth/authenticator.h"
#include "auth/authorizer.h"

/** Handler for HTTP Basic authentication scheme.
 * Check for Authorization header, challenge login/password against an
 * Authenticator and set userid in HttpRequestContext.
 *
 * Using an Authorizer is optional and is only used to decide if unauthenticated
 * access are allowed or not, not to actually check authorization rules.
 *
 * In the following cases, serve a 401 response and stop the handlers pipeline:
 * - There are no or invalid credentials and auth is mandatory (which is not
 *   the default).
 * - There are no or invalid credentials and an Authorizer is set and denies
 *   access with empty userid for current path (the path is givent to the
 *   authorizer as action scope); this is convenient e.g. to allow
 *   unauthenticated access to static resources matching ^/css/.* but issue a
 *   401 for other pathes.
 *
 * In all other cases the pipeline will continue (and authorization check of
 * authenticated users is up to the following handlers).
 */
class LIBP6CORESHARED_EXPORT BasicAuthHttpHandler : public HttpHandler {
  Q_OBJECT
  Q_DISABLE_COPY(BasicAuthHttpHandler)
  Authenticator *_authenticator;
  Authorizer *_authorizer;
  bool _authIsMandatory;
  QByteArray _realm, _userIdContextParamName;
  ParamSet _authContext;

public:
  explicit BasicAuthHttpHandler(QObject *parent = 0);
  ~BasicAuthHttpHandler();
  bool acceptRequest(HttpRequest &req) override;
  bool handleRequest(HttpRequest &req, HttpResponse &res,
                     ParamsProviderMerger &request_context) override;
  /** Does not take ownership */
  void setAuthenticator(Authenticator *authenticator);
  /** Does not take ownership */
  void setAuthorizer(Authorizer *authorizer);
  void setRealm(const QByteArray &realm) {
    _realm = realm;
    _authContext.insert("realm"_u8, realm);
  }
  /** Define which param name will be used to set the (principal) user id in
   * HttpRequestContext. Default is "userid". Null or empty string disable
   * setting any parameter in HttpRequestContext. */
  void setUserIdContextParamName(const QByteArray &name) {
    _userIdContextParamName = name;
  }

public slots:
  /** If no or bad basic auth, request auth (HTTP 401) and stop pipeline
   * (handRequest() returns false).
   * Otherwise let the page be served with no userId in HttpRequestContext. */
  void enableMandatoryAuth(bool mandatory = true);
};

#endif // BASICAUTHHTTPHANDLER_H
