/* Copyright 2016-2023 Hallowyn, Gregoire Barbier and others.
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
#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "session.h"
#include <QHash>

class LIBP6CORESHARED_EXPORT SessionManager : public QObject {
  Q_OBJECT
  QHash<qint64,Session> _sessions;
  QHash<qint64,QHash<const char*,QVariant>> _params;
  qint64 _lastSessionId;

  explicit SessionManager() : QObject(0), _lastSessionId(0) { }

public:
  /** This method is thread-safe. */
  static Session createSession();
  /** This method is thread-safe. */
  static Session session(qint64 sessionid);
  /** This method is thread-safe. */
  static void closeSession(qint64 sessionid);
  /** This method is thread-safe. */
  static SessionManager *instance();
  /** This method is thread-safe. */
  static QVariant param(qint64 sessionid, const char *key);
  /** This method is thread-safe. */
  static void setParam(qint64 seesionid, const char *key,
                       const QVariant &value);
  /** This method is thread-safe. */
  static void unsetParam(qint64 seesionid, const char *key);
  /** Returned params are detached from real params and modification will have
   * no effect on actual session params.
   * This method is thread-safe. */
  static const QHash<const char*,QVariant> params(qint64 sessionid);

signals:
  void sessionClosed(const Session &session);
};

#endif // SESSIONMANAGER_H
