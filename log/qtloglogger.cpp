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
#include "qtloglogger.h"
#include <QtDebug>

QtLogLogger::QtLogLogger(Log::Severity minSeverity)
  : Logger(minSeverity, Logger::DirectCall) {
}

void QtLogLogger::doLog(const LogEntry entry) {
  QString header = QString("%1 %2/%3 %4 %5")
      .arg(entry.timestamp().toString("yyyy-MM-ddThh:mm:ss,zzz"))
      .arg(entry.task()).arg(entry.execId()).arg(entry.sourceCode())
      .arg(entry.severityText());
  // LATER try to use QLoggingCategory e.g. using task as a category
  switch(entry.severity()) {
  case Log::Debug:
  case Log::Info:
    qDebug() << header << entry.message();
    break;
  case Log::Warning:
  case Log::Error:
    qWarning() << header << entry.message();
    break;
  case Log::Fatal:
    qCritical() << header << entry.message();
    break;
  }
}
