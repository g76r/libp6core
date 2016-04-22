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
#ifndef MEMORYLOGGER_H
#define MEMORYLOGGER_H

#include "logger.h"

class LogModel;

/** Logger used internaly by LogModel.
 * @see LogModel */
class LIBQTSSUSHARED_EXPORT MemoryLogger : public Logger {
  friend class LogModel;
  Q_OBJECT
  Q_DISABLE_COPY(MemoryLogger)
  QString _prefixFilter;
  LogModel *_model;

  // only LogModel can create a MemoryLogger object, ensuring they share the
  // same thread, therefore the constructor must not be public
  MemoryLogger(Log::Severity minSeverity, QString prefixFilter,
               LogModel *logmodel);

protected:
  void doLog(const LogEntry &entry);
};

#endif // MEMORYLOGGER_H
