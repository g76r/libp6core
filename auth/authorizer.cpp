/* Copyright 2013-2022 Hallowyn, Gregoire Barbier and others.
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
#include "authorizer.h"

Authorizer::Authorizer(QObject *parent)
  : QObject(parent), _usersDatabase(0) {
}

Authorizer::~Authorizer() {
}

bool Authorizer::authorize(QString userId, QString actionScope,
                           QString dataScope, QDateTime timestamp) const {
  if (_usersDatabase)
    return authorizeUserData(_usersDatabase->userData(userId), actionScope,
                             dataScope, timestamp);
  return false;
}

Authorizer &Authorizer::setUsersDatabase(UsersDatabase *db) {
  _usersDatabase = db;
  return *this;
}

bool Authorizer::authorizeUserData(
  UserData, QString, QString, QDateTime) const {
  return false;
}
