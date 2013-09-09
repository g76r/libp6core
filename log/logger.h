/* Copyright 2013 Hallowyn and others.
 * This file is part of qron, see <http://qron.hallowyn.com/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include "log.h"
#include <QDateTime>
#include "util/paramset.h"

class LIBQTSSUSHARED_EXPORT Logger : public QObject {
  friend class Log;
  Q_OBJECT
  Q_DISABLE_COPY(Logger)
  Log::Severity _minSeverity;
  bool _removable;

public:
  explicit Logger(QObject *parent = 0, Log::Severity minSeverity = Log::Info);
  /** This method is thread-safe. */
  void log(QDateTime timestamp, QString message, Log::Severity severity,
           QString task, QString execId, QString sourceCode);
  virtual QString currentPath() const;
  /** Return the path pattern, e.g. "/var/log/qron-%!yyyy%!mm%!dd.log" */
  virtual QString pathPattern() const;
  /** Return the path regexp pattern, e.g. "/var/log/qron-.*\\.log" */
  inline QString pathMathchingPattern() const {
    return ParamSet::matchingPattern(pathPattern()); }
  /** Return the path regexp pattern, e.g. "/var/log/qron-.*\\.log" */
  inline QRegExp pathMatchingRegexp() const {
    return ParamSet::matchingRegexp(pathPattern()); }
  Log::Severity minSeverity() const { return _minSeverity; }

protected:
  Q_INVOKABLE virtual void doLog(QDateTime timestamp, QString message,
                                 Log::Severity severity, QString task,
                                 QString execId, QString sourceCode) = 0;
};

#endif // LOGGER_H
