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
#include "basicauthhttphandler.h"
#include <QRegularExpression>

BasicAuthHttpHandler::BasicAuthHttpHandler(QObject *parent)
  : HttpHandler(parent), _authenticator(0), _authorizer(0),
    _authIsMandatory(false), _userIdContextParamName("userid") {
}

BasicAuthHttpHandler::~BasicAuthHttpHandler() {
}

bool BasicAuthHttpHandler::acceptRequest(HttpRequest req) {
  Q_UNUSED(req)
  return true;
}

static QRegularExpression _headerRe("\\A\\s*Basic\\s+(\\S+)\\s*\\z");
static QRegularExpression _tokenRe("\\A([^:]+):([^:]+)\\z"); // LATER : in pwd ?

bool BasicAuthHttpHandler::handleRequest(
    HttpRequest req, HttpResponse res, ParamsProviderMerger *processingContext) {
  auto header = req.header("Authorization");
  QByteArray token;
  auto m = _headerRe.match(header);
  if (m.hasMatch()) {
    token = QByteArray::fromBase64(m.captured(1).toUtf8());
    m = _tokenRe.match(token);
    if (m.hasMatch()) {
      auto login = m.captured(1), password = m.captured(2);
      if (_authenticator) {
        auto userId = _authenticator->authenticate(login, password,
                                                   _authContext);
        if (!userId.isEmpty()) {
          if (!_userIdContextParamName.isEmpty())
            processingContext
                ->overrideParamValue(_userIdContextParamName, userId);
          return true;
        }
      }
    }
  }
  if (_authIsMandatory
      || (_authorizer && !_authorizer->authorize(
            {}, req.method_name(), req.path(),
            QDateTime::currentDateTime()))) {
    res.set_status(HttpResponse::HTTP_Authentication_Required);
    res.set_header("WWW-Authenticate", // LATER sanitize realm
                  "Basic realm=\""+_realm+"\"");
    return false;
  }
  return true;
}

void BasicAuthHttpHandler::setAuthenticator(Authenticator *authenticator) {
  _authenticator = authenticator;
}

void BasicAuthHttpHandler::setAuthorizer(Authorizer *authorizer) {
  _authorizer = authorizer;
}

void BasicAuthHttpHandler::enableMandatoryAuth(bool mandatory) {
  _authIsMandatory = mandatory;
}
