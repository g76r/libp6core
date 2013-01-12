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
#include "logger.h"

Logger::Logger(QObject *parent, Log::Severity minSeverity) : QObject(parent),
  _minSeverity(minSeverity), _removable(true) {
  qRegisterMetaType<Log::Severity>("Log::Severity");
}

void Logger::log(QDateTime timestamp, QString message, Log::Severity severity,
                 QString task, QString execId, QString sourceCode) {
  // this method must be callable from any thread, whereas the logger
  // implementation may not be threadsafe and/or may need a protection
  // against i/o latency: slow disk, NFS stall (for those fool enough to
  // write logs over NFS), etc.
  if (severity >= _minSeverity)
    QMetaObject::invokeMethod(this, "doLog",
                              Q_ARG(QDateTime, timestamp),
                              Q_ARG(QString, message),
                              Q_ARG(Log::Severity, severity),
                              Q_ARG(QString, task),
                              Q_ARG(QString, execId),
                              Q_ARG(QString, sourceCode));
}
