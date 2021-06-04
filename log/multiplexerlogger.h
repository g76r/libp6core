/* Copyright 2014-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef MULTIPLEXERLOGGER_H
#define MULTIPLEXERLOGGER_H

#include "logger.h"
#include <QList>
#include <QMutex>

/** Logger for multiplexing to log writing to several loggers.
 * Mainly intended to be used internaly as a singleton by Log.
 * @see Log */
class LIBP6CORESHARED_EXPORT MultiplexerLogger : public Logger {
  Q_OBJECT
  Q_DISABLE_COPY(MultiplexerLogger)
  QList<Logger*> _loggers;
  QSet<Logger*> _ownedLoggers;
  QMutex _loggersMutex;

public:
  explicit MultiplexerLogger(Log::Severity minSeverity = Log::Debug,
                             bool isRootLogger = false);
  /** Add logger to loggers list and optionaly take ownership of it, i.e. will
   * delete it on removal. */
  void addLogger(Logger *logger, bool autoRemovable, bool takeOwnership);
  /** Remove and delete a logger. Will only delete the object if it is currently
   * registred in the logger list, otherwise the method does nothing. */
  void removeLogger(Logger *logger);
  void addConsoleLogger(Log::Severity severity, bool autoRemovable);
  void addQtLogger(Log::Severity severity, bool autoRemovable);
  /** Replace current auto-removable loggers with a new one.
   * This method is thread-safe and switches loggers in an atomic way. */
  void replaceLoggers(Logger *newLogger, bool takeOwnership);
  /** Replace current auto-removable loggers with new ones.
   * This method is thread-safe and switches loggers in an atomic way. */
  void replaceLoggers(QList<Logger*> newLoggers, bool takeOwnership);
  /** Replace current auto-removable loggers with new ones plus a new
   * console logger with given logger severity.
   * This method is thread-safe and switches loggers in an atomic way. */
  void replaceLoggersPlusConsole(Log::Severity consoleLoggerSeverity,
                                 QList<Logger*> newLoggers, bool takeOwnership);
  QString pathToLastFullestLog();
  QStringList pathsToFullestLogs();
  QStringList pathsToAllLogs();

protected:
  void doLog(const LogEntry &entry);

private:
  inline void doReplaceLoggers(QList<Logger*> newLoggers, bool takeOwnership);
};

#endif // MULTIPLEXERLOGGER_H
