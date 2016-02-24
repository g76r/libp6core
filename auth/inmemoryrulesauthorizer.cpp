/* Copyright 2013-2016 Hallowyn and others.
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
#include "inmemoryrulesauthorizer.h"

InMemoryRulesAuthorizer::InMemoryRulesAuthorizer(QObject *parent)
  : Authorizer(parent) {
}

InMemoryRulesAuthorizer::InMemoryRulesAuthorizer(UsersDatabase *db) {
  setUsersDatabase(db);
}

InMemoryRulesAuthorizer::~InMemoryRulesAuthorizer() {
}

//#include "log/log.h"
bool InMemoryRulesAuthorizer::authorizeUserData(
    UserData userData, QString actionScope, QString dataScope,
    QDateTime timestamp) const {
  Q_UNUSED(timestamp)
  QMutexLocker locker(&_mutex);
  //Log::fatal() << "authorize: i:" << userData.userId()
  //             << " r:" << userData.roles() << " a:" << actionScope
  //             << " d:" << dataScope << " t:" << timestamp;
  foreach(const Rule &rule, _rules) {
    //Log::fatal() << "authorize rule: r:" << rule._roles
    //             << " a:" << rule._actionScopePattern.pattern()
    //             << " d:" << rule._dataScopePattern.pattern()
    //             << " t:" << rule._timestampPattern.pattern()
    //             << " allow:" << rule._allow;
    if (rule._roles.isEmpty())
      goto roleok;
    foreach (const QString &role, userData.roles())
      if (rule._roles.contains(role))
        goto roleok;
    //Log::fatal() << "authorize rule: roles not matching";
    continue; // no role match between rule and user data
roleok:;
    if (!rule._actionScopePattern.isEmpty()
        && !rule._actionScopePattern.exactMatch(actionScope)) {
      //Log::fatal() << "authorize rule: action scope not matching";
      continue;
    }
    if (!rule._dataScopePattern.isEmpty()
        && !rule._dataScopePattern.exactMatch(dataScope)) {
      //Log::fatal() << "authorize rule: data scope not matching";
      continue;
    }
    //Log::fatal() << "authorize rule: matching";
    // LATER implement timestamp-based authorization
    return rule._allow;
  }
  return false;
}

InMemoryRulesAuthorizer &InMemoryRulesAuthorizer::clearRules() {
  QMutexLocker locker(&_mutex);
  _rules.clear();
  return *this;
}

InMemoryRulesAuthorizer &InMemoryRulesAuthorizer::appendRule(
    QSet<QString> roles, QRegExp actionScopePattern,
    QRegExp dataScopePattern, QRegExp timestampPattern, bool allow) {
  QMutexLocker locker(&_mutex);
  _rules.append(Rule(roles, actionScopePattern, dataScopePattern,
                     timestampPattern, allow));
  return *this;
}

InMemoryRulesAuthorizer &InMemoryRulesAuthorizer::prependRule(
    QSet<QString> roles, QRegExp actionScopePattern,
    QRegExp dataScopePattern, QRegExp timestampPattern, bool allow) {
  QMutexLocker locker(&_mutex);
  _rules.prepend(Rule(roles, actionScopePattern, dataScopePattern,
                      timestampPattern, allow));
  return *this;
}
