/* Copyright 2013 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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
#include "inmemoryusersdatabase.h"
#include "usersdatabase_spi.h"

class InMemoryUserDataData : public UserDataData {
  QString _userId;
  QSet<QString> _roles;
  QString _mainGroupId;
  QSet<QString> _allGroupIds;
  QString _userName;

public:
  InMemoryUserDataData() { }
  InMemoryUserDataData(
      QString userId, QSet<QString> roles, QString mainGroupId,
      QSet<QString> allGroupIds, QString userName)
    : _userId(userId), _roles(roles), _mainGroupId(mainGroupId),
      _allGroupIds(allGroupIds),
      _userName(userName.isEmpty() ? userId : userName) { }
  ~InMemoryUserDataData() { }
  QString userId() const { return _userId; }
  QString userName() const { return _userName; }
  QSet<QString> roles() const { return _roles; }
  QString mainGroupId() const { return _mainGroupId; }
  QSet<QString> allGroupIds() const { return _allGroupIds; }
};

InMemoryUsersDatabase::InMemoryUsersDatabase(QObject *parent)
  : UsersDatabase(parent) {
}

InMemoryUsersDatabase::~InMemoryUsersDatabase() {
}

UserData InMemoryUsersDatabase::userData(QString userId) const {
  QMutexLocker locker(&_mutex);
  return _users.value(userId);
}

InMemoryUsersDatabase &InMemoryUsersDatabase::insertUser(
    QString userId, QSet<QString> roles, QString mainGroupId,
    QSet<QString> allGroupIds, QString userName) {
  if (!userId.isEmpty()) {
    QMutexLocker locker(&_mutex);
    allGroupIds.insert(mainGroupId);
    _users.insert(userId,
                  UserData(new InMemoryUserDataData(userId, roles, mainGroupId,
                                                    allGroupIds, userName)));
  }
  return *this;
}

InMemoryUsersDatabase &InMemoryUsersDatabase::clearUsers() {
  QMutexLocker locker(&_mutex);
  _users.clear();
  return *this;
}
