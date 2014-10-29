/* Copyright 2014 Hallowyn and others.
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
#ifndef MULTIPLEXERLOGGER_H
#define MULTIPLEXERLOGGER_H

#include "logger.h"
#include <QList>
#include <QMutex>

/** Logger for multiplexing to log writing to several loggers.
 * Mainly intended to be used internaly as a singleton by Log.
 * @see Log */
class LIBQTSSUSHARED_EXPORT MultiplexerLogger : public Logger {
  Q_OBJECT
  Q_DISABLE_COPY(MultiplexerLogger)
  QList<Logger*> _loggers;
  QMutex _loggersMutex;

public:
  explicit MultiplexerLogger(Log::Severity minSeverity = Log::Debug,
                             bool isRootLogger = false);
  void addLogger(Logger *logger, bool autoRemovable);
  void removeLogger(Logger *logger);
  void addConsoleLogger(Log::Severity severity, bool autoRemovable);
  void addQtLogger(Log::Severity severity, bool autoRemovable);
  //void removeAutoRemovableLoggers();
  void replaceLoggers(Logger *newLogger);
  void replaceLoggers(QList<Logger*> newLoggers);
  void replaceLoggersPlusConsole(Log::Severity consoleLoggerSeverity,
                                 QList<Logger*> newLoggers);
  QString pathToLastFullestLog();
  QStringList pathsToFullestLogs();
  QStringList pathsToAllLogs();

protected:
  void doLog(const LogEntry entry);
};

#endif // MULTIPLEXERLOGGER_H
