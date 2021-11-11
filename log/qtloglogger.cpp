/* Copyright 2014-2021 Hallowyn, Gregoire Barbier and others.
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
#include "qtloglogger.h"
#include <QtDebug>

QtLogLogger::QtLogLogger(Log::Severity minSeverity)
  : Logger(minSeverity, Logger::DirectCall) {
}

void QtLogLogger::doLog(const LogEntry &entry) {
  QString header = QString("%1 %2/%3 %4 %5")
      .arg(entry.timestamp().toString("yyyy-MM-ddThh:mm:ss,zzz"), entry.task(),
           entry.execId(), entry.sourceCode(), entry.severityToString());
  // LATER try to use QLoggingCategory e.g. using task as a category
  switch(entry.severity()) {
  case Log::Debug:
  case Log::Info:
    qDebug().noquote() << header << entry.message();
    break;
  case Log::Warning:
  case Log::Error:
    qWarning().noquote() << header << entry.message();
    break;
  case Log::Fatal:
    qCritical().noquote() << header << entry.message();
    break;
  }
}
