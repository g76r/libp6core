/* Copyright 2013-2017 Hallowyn, Gregoire Barbier and others.
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

static QRegExp headerRe("\\s*Basic\\s+(\\S+)\\s*");
static QRegExp tokenRe("([^:]+):([^:]+)");

bool BasicAuthHttpHandler::handleRequest(
    HttpRequest req, HttpResponse res, ParamsProviderMerger *processingContext) {
  QString header = req.header("Authorization"), token;
  QRegExp re = headerRe;
  if (re.exactMatch(header)) {
    token = QString::fromUtf8(QByteArray::fromBase64(re.cap(1).toLatin1()));
    re = tokenRe;
    if (re.exactMatch(token)) {
      QString login = re.cap(1), password = re.cap(2);
      if (_authenticator) {
        QString userId = _authenticator->authenticate(login, password,
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
            QString(), req.methodName(), req.url().path(),
            QDateTime::currentDateTime()))) {
    res.setStatus(401);
    res.setHeader("WWW-Authenticate", // LATER sanitize realm
                  QString("Basic realm=\"%1\"").arg(_realm));
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

void BasicAuthHttpHandler::setAuthIsMandatory(bool mandatory) {
  _authIsMandatory = mandatory;
}
