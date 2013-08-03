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
#include "usersdatabase_spi.h"

bool UserDataData::hasRole(QString role) const {
  return roles().contains(role);
}

UserDataData::~UserDataData() {
}

UserData::UserData() {
}

UserData::UserData(const UserData &other) : d(other.d) {
}

UserData::UserData(UserDataData *data) : d(data) {
}

UserData::~UserData() {
}

UserData &UserData::operator=(const UserData &other) {
  if (this != &other)
    d = other.d;
  return *this;
}

QString UserData::userId() const {
  return d ? d->userId() : QString();
}

QString UserData::userName() const {
  return d ? d->userName() : QString();
}

bool UserData::hasRole(QString role) const {
  return d ? d->hasRole(role) : false;
}

QSet<QString> UserData::roles() const {
  return d ? d->roles() : QSet<QString>();
}

QString UserData::mainGroupId() const {
  return d ? d->mainGroupId() : QString();
}

QSet<QString> UserData::allGroupIds() const {
  return d ? d->allGroupIds() : QSet<QString>();
}

UsersDatabase::UsersDatabase(QObject *parent) : QObject(parent) {
}

UsersDatabase::~UsersDatabase() {
}
