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

QtLogLogger::QtLogLogger(QObject *parent)
  : Logger(0) {
  Q_UNUSED(parent)
}

QtLogLogger::QtLogLogger(Log::Severity minSeverity, QObject *parent)
  : Logger(0, minSeverity) {
  Q_UNUSED(parent)
}

void QtLogLogger::doLog(QDateTime timestamp, QString message,
                        Log::Severity severity,
                        QString task, QString execId,
                        QString sourceCode) {
  QString header = QString("%1 %2/%3 %4 %5")
      .arg(timestamp.toString("yyyy-MM-ddThh:mm:ss,zzz")).arg(task)
      .arg(execId).arg(sourceCode).arg(Log::severityToString(severity));
  switch(severity) {
  case Log::Debug:
  case Log::Info:
    qDebug() << header << message;
    break;
  case Log::Warning:
  case Log::Error:
    qWarning() << header << message;
    break;
  case Log::Fatal:
    qCritical() << header << message;
    break;
  }
}
