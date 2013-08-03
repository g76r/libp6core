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
#ifndef INMEMORYRULESAUTHORIZER_H
#define INMEMORYRULESAUTHORIZER_H

#include "authorizer.h"
#include <QSet>
#include <QRegExp>
#include <QMutex>

/** In-memory rules-list based authorizer.
 * The rules are evaluated in the list order.
 * Allow and deny rules can be appended or prepended to the list.
 * If no rule matches the authorization is denied (however one can append a last
 * rule that allow everything, allow() with no parameters will do that).
 * In a rule, an empty or null criterion matches all authorization requests
 * (e.g. using QString() or QString("") or QRegExp() as actionScopePattern will
 * match any actionScope value). This is true even for roles criterion. */
class LIBQTSSUSHARED_EXPORT InMemoryRulesAuthorizer : public Authorizer {
  Q_OBJECT
  Q_DISABLE_COPY(InMemoryRulesAuthorizer)
  class Rule {
  public:
    QSet<QString> _roles;
    QRegExp _actionScopePattern, _dataScopePattern, _timestampPattern;
    bool _allow; // otherwise deny
    Rule() {}
    Rule(QSet<QString> roles, QRegExp actionScopePattern,
         QRegExp dataScopePattern, QRegExp timestampPattern, bool allow)
      : _roles(roles), _actionScopePattern(actionScopePattern),
        _dataScopePattern(dataScopePattern),
        _timestampPattern(timestampPattern), _allow(allow) { }
  };
  QList<Rule> _rules;
  mutable QMutex _mutex;

public:
  explicit InMemoryRulesAuthorizer(QObject *parent = 0);
  InMemoryRulesAuthorizer(UsersDatabase *db, bool takeOwnership);
  ~InMemoryRulesAuthorizer();
  /** This method is thread-safe */
  bool authorizeUserData(UserData user, QString actionScope,
                         QString dataScope = QString(),
                         QDateTime timestamp = QDateTime()) const;
  /** This method is thread-safe */
  InMemoryRulesAuthorizer &clearRules();
  /** This method is thread-safe */
  InMemoryRulesAuthorizer &appendRule(
      QSet<QString> roles, QRegExp actionScopePattern = QRegExp(),
      QRegExp dataScopePattern = QRegExp(),
      QRegExp timestampPattern = QRegExp(), bool allow = true);
  /** This method is thread-safe */
  InMemoryRulesAuthorizer &prependRule(
      QSet<QString> roles, QRegExp actionScopePattern = QRegExp(),
      QRegExp dataScopePattern = QRegExp(),
      QRegExp timestampPattern = QRegExp(), bool allow = true);
  /** Syntaxic sugar for QRegExp. Append an allow rule. */
  inline InMemoryRulesAuthorizer &allow(
      QSet<QString> roles, QString actionScopePattern = QString(),
      QString dataScopePattern = QString(),
      QString timestampPattern = QString()) {
    return appendRule(roles, QRegExp(actionScopePattern),
                      QRegExp(dataScopePattern), QRegExp(timestampPattern),
                      true);
  }
  /** Syntaxic sugar for QRegExp. Append an allow rule. */
  inline InMemoryRulesAuthorizer &deny(
      QSet<QString> roles, QString actionScopePattern = QString(),
      QString dataScopePattern = QString(),
      QString timestampPattern = QString()) {
    return appendRule(roles, QRegExp(actionScopePattern),
                      QRegExp(dataScopePattern), QRegExp(timestampPattern),
                      false);
  }
  /** Syntaxic sugar for QRegExp and QSet. Append an allow rule. */
  inline InMemoryRulesAuthorizer &allow(
      QString role = QString(), QString actionScopePattern = QString(),
      QString dataScopePattern = QString(),
      QString timestampPattern = QString()) {
    QSet<QString> roles;
    if (!role.isEmpty())
      roles.insert(role);
    return appendRule(roles, QRegExp(actionScopePattern),
                      QRegExp(dataScopePattern), QRegExp(timestampPattern),
                      true);
  }
  /** Syntaxic sugar for QRegExp and QSet. Append an deny rule. */
  inline InMemoryRulesAuthorizer &deny(
      QString role = QString(), QString actionScopePattern = QString(),
      QString dataScopePattern = QString(),
      QString timestampPattern = QString()) {
    QSet<QString> roles;
    if (!role.isEmpty())
      roles.insert(role);
    return appendRule(roles, QRegExp(actionScopePattern),
                      QRegExp(dataScopePattern), QRegExp(timestampPattern),
                      false);
  }
};

#endif // INMEMORYRULESAUTHORIZER_H
