/* Copyright 2016-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef SESSION_H
#define SESSION_H

#include "libp6core_global.h"
#include <QString>
#include "util/paramset.h"

/** Data object representing a session, regardless the network transport.
 * Can be regarded and processed as if it were a data object, but actually
 * it's just an handler/id and any actual session data is holded in the
 * SessionManager.
 * @see SessionManager
 */
class LIBP6CORESHARED_EXPORT Session {
  qint64 _id;

public:
  Session(qint64 id = 0) : _id(id) { }
  qint64 id() const { return _id; }
  /** Get a param value, with custom or framework keys.
   * Framework-setted params may include these ones:
   * - login, when authentification has been done successfuly
   * - clientaddr, network peer identifier, e.g. "[::ffff:127.0.0.1]:34669"
   * This method is thread-safe. */
  QVariant param(const char *key) const;
  /** This method is thread-safe. */
  void setParam(const char *key, const QVariant &value) const;
  /** This method is thread-safe. */
  void unsetParam(const char *key) const;
  /** Convenience method for param(key).toString()
   * This method is thread-safe. */
  QString string(const char *key) const { return param(key).toString(); }
  /** Convenience method for param(key).toLongLong()
   * This method is thread-safe.*/
  qlonglong integer(const char *key) const { return param(key).toLongLong(); }
  bool isNull() const { return _id == 0; }
  operator bool() const { return !isNull(); }
};

Q_DECLARE_METATYPE(Session)
Q_DECLARE_TYPEINFO(Session, Q_MOVABLE_TYPE);

inline uint qHash(const Session &session) { return qHash(session.id()); }

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const Session &session);

LogHelper LIBP6CORESHARED_EXPORT operator<<(LogHelper lh,
                                             const Session &session);

#endif // SESSION_H
