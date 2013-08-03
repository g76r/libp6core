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
#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include "libqtssu_global.h"
#include <QString>
#include "util/paramset.h"
#include <QObject>

/** Authentication service interface. */
class LIBQTSSUSHARED_EXPORT Authenticator : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(Authenticator)

public:
  explicit Authenticator(QObject *parent = 0);
  ~Authenticator();
  /** @param password plain clear text password
   * @param ctxt if implementation need a context (e.g. realm)
   * @return principal user id or null string if failed
   * This method is thread-safe */
  virtual QString authenticate(QString login, QString password,
                               ParamSet ctxt = ParamSet()) const = 0;
};

#endif // AUTHENTICATOR_H
