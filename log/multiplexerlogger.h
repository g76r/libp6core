/* Copyright 2014-2025 Hallowyn, Gregoire Barbier and others.
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

namespace p6::log {

/** Logger for multiplexing to log writing to several loggers.
 * Mainly intended to be used internaly as a singleton by Log.
 * @see Log */
class LIBP6CORESHARED_EXPORT MultiplexerLogger : public Logger {
  Q_OBJECT
  Q_DISABLE_COPY(MultiplexerLogger)
  QList<Logger*> _loggers;
  QMutex _loggersMutex;

public:
  explicit MultiplexerLogger(Severity minSeverity = Debug,
                             bool isRootLogger = false);
  ~MultiplexerLogger();
  /** Add logger to loggers list and take ownership of it, i.e. will
   * delete it on removal. */
  void addLogger(Logger *logger, bool autoRemovable);
  /** Remove and delete a logger. Will only delete the object if it is currently
   * registred in the logger list, otherwise the method does nothing. */
  void removeLogger(Logger *logger);
  void addConsoleLogger(Severity severity, bool autoRemovable,
                        FILE *stream);
  void addQtLogger(Severity severity, bool autoRemovable);
  /** Replace current auto-removable loggers with new ones.
   *  Optionaly prepend a console logger.
   * This method is thread-safe and switches loggers in an atomic way. */
  void replace_loggers(
      QList<Logger*> &new_loggers, bool prepend_console = false,
      Severity console_min_severity = Fatal);
  QString pathToLastFullestLog();
  QStringList pathsToFullestLogs();
  QStringList pathsToAllLogs();

protected:
  void do_log(const Record &record) override;
  void do_shutdown() override;
};

} // ns p6::log

#endif // MULTIPLEXERLOGGER_H
