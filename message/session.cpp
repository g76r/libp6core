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
#include "session.h"
#include "sessionmanager.h"

QVariant Session::param(const char *key) const {
  return SessionManager::param(_id, key);
}

void Session::setParam(const char *key, const QVariant &value) const {
  SessionManager::setParam(_id, key, value);
}

void Session::unsetParam(const char *key) const {
  SessionManager::unsetParam(_id, key);
}

QDebug operator<<(QDebug dbg, const Session &session) {
  dbg.nospace() << session.id() << SessionManager::params(session.id());
  return dbg.space();
}

LogHelper operator<<(LogHelper lh, const Session &session) {
  lh << "{ " << session.id() << ", { ";
  auto params = SessionManager::params(session.id());
  foreach(const char *key, params.keys())
    lh << key << "=" << params.value(key) << " ";
  return lh << " } }";
}
