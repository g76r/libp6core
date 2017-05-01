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
#include "sessionmanager.h"

static QMutex _mutex;
static SessionManager *_singleton = 0;

Session SessionManager::createSession() {
  SessionManager *sm = instance();
  QMutexLocker ml(&_mutex);
  qint64 id = ++(sm->_lastSessionId);
  Session session = Session(id);
  sm->_sessions.insert(id, session);
  return session;
}

Session SessionManager::session(qint64 sessionid) {
  SessionManager *sm = instance();
  QMutexLocker ml(&_mutex);
  return sm->_sessions.value(sessionid);
}

SessionManager *SessionManager::instance() {
  QMutexLocker ml(&_mutex);
  if (!_singleton)
    _singleton = new SessionManager;
  return _singleton;
}

void SessionManager::closeSession(qint64 sessionid) {
  SessionManager *sm = instance();
  QMutexLocker ml(&_mutex);
  Session session = sm->_sessions.value(sessionid);
  if (session.isNull())
    return;
  sm->_sessions.remove(sessionid);
  ml.unlock();
  emit sm->sessionClosed(session);
}

QVariant SessionManager::param(qint64 sessionid, const char *key) {
  SessionManager *sm = instance();
  QMutexLocker ml(&_mutex);
  const QHash<const char*,QVariant> &paramset = sm->_params.value(sessionid);
  return paramset.value(key);
}

void SessionManager::setParam(
    qint64 seesionid, const char *key, const QVariant &value) {
  SessionManager *sm = instance();
  QMutexLocker ml(&_mutex);
  if (!sm->_sessions.contains(seesionid))
    return; // do not set param to inexistent session
  sm->_params[seesionid][key] = value;
}

void SessionManager::unsetParam(
    qint64 seesionid, const char *key) {
  SessionManager *sm = instance();
  QMutexLocker ml(&_mutex);
  if (!sm->_sessions.contains(seesionid))
    return; // do not set param to inexistent session
  sm->_params[seesionid].remove(key);
}

const QHash<const char*,QVariant> SessionManager::params(qint64 sessionid) {
  SessionManager *sm = instance();
  QMutexLocker ml(&_mutex);
  QHash<const char *,QVariant> params;
  if (sm->_sessions.contains(sessionid)) {
    params = sm->_params.value(sessionid);
    params.detach();
  }
  return params;
}
