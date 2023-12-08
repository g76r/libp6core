/* Copyright 2013-2023 Hallowyn, Gregoire Barbier and others.
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
#ifndef USERSDATABASE_H
#define USERSDATABASE_H

#include "libp6core_global.h"
#include <QSharedDataPointer>
#include "util/utf8stringset.h"

class UserDataData;

class LIBP6CORESHARED_EXPORT UserData {
  QSharedDataPointer<UserDataData> d;

public:
  UserData();
  UserData(const UserData &other);
  /** this constructor must only be called by UserData implementation */
  UserData(UserDataData *data);
  ~UserData();
  UserData &operator=(const UserData &other);
  [[nodiscard]] inline bool isNull() const { return !d; }
  [[nodiscard]] inline bool operator!() const { return isNull(); }
  QString userId() const;
  QString userName() const;
  bool hasRole(QString role) const;
  QSet<QString> roles() const;
  QString mainGroupId() const;
  QSet<QString> allGroupIds() const;
};

/** Users database service interface.
 * Provide information about a user knowing its (principal) id. */
class LIBP6CORESHARED_EXPORT UsersDatabase : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(UsersDatabase)

public:
  explicit UsersDatabase(QObject *parent = 0);
  ~UsersDatabase();
  /** This method is thread-safe */
  virtual UserData userData(QString userId) const = 0;
  /** syntaxic sugar over userData() */
  QString userName(QString userId) const {
    return userData(userId).userName(); }
  /** syntaxic sugar over userData() */
  bool hasRole(QString userId, QString role) const {
    return userData(userId).hasRole(role); }
  /** syntaxic sugar over userData() */
  QSet<QString> roles(QString userId) const {
    return userData(userId).roles(); }
  /** syntaxic sugar over userData() */
  QString mainGroupId(QString userId) const {
    return userData(userId).mainGroupId(); }
  /** syntaxic sugar over userData() */
  QSet<QString> allGroupIds(QString userId) const {
    return userData(userId).allGroupIds(); }
};

#endif // USERSDATABASE_H
