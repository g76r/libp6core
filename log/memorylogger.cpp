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
#include "memorylogger.h"
#include "logmodel.h"

MemoryLogger::MemoryLogger(QObject *parent, Log::Severity minSeverity) :
  Logger(parent, minSeverity), _model(new LogModel(this)) {
}

void MemoryLogger::doLog(QDateTime timestamp, QString message,
                         Log::Severity severity, QString task, QString execId,
                         QString sourceCode) {
  _model->log(timestamp, message, severity, task, execId, sourceCode);
}
