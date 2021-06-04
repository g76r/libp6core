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
#ifndef INMEMORYAUTHENTICATOR_H
#define INMEMORYAUTHENTICATOR_H

#include "authenticator.h"
#include <QMutex>

/** In-memory users-passwords database.
 * Apart from plain (clear text) passwords, some current hash algorithms are
 * also supported.
 * All of them also allow salt (right bytes of a hash, after its expected
 * length depending on the algorithm, are expected to be the salt bytes).
 */
class LIBP6CORESHARED_EXPORT InMemoryAuthenticator : public Authenticator {
  Q_OBJECT
  Q_DISABLE_COPY(InMemoryAuthenticator)
  class User;
  QHash<QString,User> _users;
  mutable QMutex _mutex;

public:
  enum Encoding { Plain, Md4Hex, Md4Base64, Md5Hex, Md5Base64, Sha1Hex,
                  Sha1Base64, OpenLdapStyle, Unknown = -1 };
  // LATER support Sha224 to 512 when switching to Qt5
  explicit InMemoryAuthenticator(QObject *parent = 0);
  ~InMemoryAuthenticator();
  /** This method is thread-safe */
  QString authenticate(QString login, QString password,
                       ParamSet ctxt = ParamSet()) const;
  /** This method is thread-safe */
  InMemoryAuthenticator &insertUser(
      QString userId, QString encodedPassword, Encoding encoding);
  /** This method is thread-safe */
  InMemoryAuthenticator &clearUsers();
  /** This method is thread-safe */
  bool containsUser(QString login) const ;
  static InMemoryAuthenticator::Encoding encodingFromString(QString text);
  static QString encodingToString(InMemoryAuthenticator::Encoding encoding);
};

#endif // INMEMORYAUTHENTICATOR_H
