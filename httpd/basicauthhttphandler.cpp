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
#include "basicauthhttphandler.h"

BasicAuthHttpHandler::BasicAuthHttpHandler(QObject *parent)
  : HttpHandler(parent), _authenticator(0), _ownAuthenticator(false),
    _authIsMandatory(true), _userIdContextParamName("userid") {
}

BasicAuthHttpHandler::~BasicAuthHttpHandler() {
  setAuthenticator(0, false);
}

bool BasicAuthHttpHandler::acceptRequest(HttpRequest req) {
  Q_UNUSED(req)
  return true;
}

bool BasicAuthHttpHandler::handleRequest(
    HttpRequest req, HttpResponse res, ParamsProviderMerger *processingContext) {
  static QRegExp headerRe("\\s*Basic\\s+(\\S+)\\s*");
  static QRegExp tokenRe("([^:]+):([^:]+)");
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
  if (_authIsMandatory) {
    res.setStatus(401);
    res.setHeader("WWW-Authenticate", // LATER sanitize realm
                  QString("Basic realm=\"%1\"").arg(_realm));
    return false;
  }
  return true;
}

BasicAuthHttpHandler &BasicAuthHttpHandler::setAuthenticator(
    Authenticator *authenticator, bool takeOwnership) {
  if (_ownAuthenticator && _authenticator)
    delete _authenticator;
  _authenticator = authenticator;
  _ownAuthenticator = takeOwnership;
  return *this;
}
