/* Copyright 2014 Hallowyn and others.
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
#ifndef QTLOGLOGGER_H
#define QTLOGLOGGER_H

#include "logger.h"

class QtLogLogger : public Logger {
  Q_OBJECT
  Q_DISABLE_COPY(QtLogLogger)
public:
  explicit QtLogLogger(Log::Severity minSeverity = Log::Info);
  void doLog(const LogEntry entry);
};

#endif // QTLOGLOGGER_H
