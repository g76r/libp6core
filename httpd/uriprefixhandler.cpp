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
#include "uriprefixhandler.h"

UriPrefixHandler::UriPrefixHandler(QObject *parent, const QString &prefix,
                                   int allowedMethods)
  : HttpHandler(parent), _prefix(prefix), _allowedMethods(allowedMethods) {
}

QString UriPrefixHandler::name() const {
  return "UriPrefixHandler:" + _prefix;
}

bool UriPrefixHandler::acceptRequest(const HttpRequest &req) {
  if ((req.method()&_allowedMethods) && req.url().path().startsWith(_prefix))
    return true;
  return false;
}
