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
#include "authorizer.h"

Authorizer::Authorizer(QObject *parent)
  : QObject(parent), _usersDatabase(0), _ownUsersDatabase(false) {
}

Authorizer::~Authorizer() {
  setUsersDatabase(0, false);
}

bool Authorizer::authorize(QString userId, QString actionScope,
                           QString dataScope, QDateTime timestamp) const {
  if (_usersDatabase)
    return authorizeUserData(_usersDatabase->userData(userId), actionScope,
                             dataScope, timestamp);
  return false;
}

Authorizer &Authorizer::setUsersDatabase(UsersDatabase *db, bool takeOwnership) {
  if (_ownUsersDatabase && _usersDatabase)
    delete _usersDatabase;
  _usersDatabase = db;
  _ownUsersDatabase = takeOwnership;
  if (takeOwnership)
    db->setParent(this);
  return *this;
}
