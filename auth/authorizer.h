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
#ifndef AUTHORIZER_H
#define AUTHORIZER_H

#include "libp6core_global.h"
#include "usersdatabase.h"
#include <QDateTime>

/** Authorization service interface */
class LIBP6CORESHARED_EXPORT Authorizer : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(Authorizer)
  UsersDatabase *_usersDatabase;

public:
  explicit Authorizer(QObject *parent = 0);
  ~Authorizer();
  /** Test if a given user is authorized to perform a given action scope
   * (e.g. "delete", "modify.delete" or "POST") on a given data scope (e.g.
   * "business.accounting.invoices" or "/foo/bar.html") at a given time.
   * Of course there can be authorization definitions that ignore some of the
   * criteria (e.g. that only check the actionScope).
   * This method is thread-safe */
  virtual bool authorizeUserData(
      UserData user, QString actionScope, QString dataScope = {},
      QDateTime timestamp = {}) const = 0;
  /** Same as previous, using the users database to resolve UserData from
   * userId. Always return false if the users database is not set.
   * This method is thread-safe */
  virtual bool authorize(
      QString userId, QString actionScope, QString dataScope = {},
      QDateTime timestamp = {}) const;
  /** Does not take ownership. */
  Authorizer &setUsersDatabase(UsersDatabase *db);
};

#endif // AUTHORIZER_H
