/* Copyright 2013-2015 Hallowyn and others.
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
#include "memorylogger.h"
#include "logmodel.h"
#include <QMetaObject>

MemoryLogger::MemoryLogger(
    Log::Severity minSeverity, QString prefixFilter, LogModel *logmodel)
  : Logger(minSeverity, Logger::DirectCall), _prefixFilter(prefixFilter),
    _model(logmodel) {
}

void MemoryLogger::doLog(const LogEntry entry) {
  if (!_prefixFilter.isNull() && !entry.message().startsWith(_prefixFilter))
    return;
  QMetaObject::invokeMethod(_model, "changeItem",
                            Q_ARG(SharedUiItem, entry),
                            Q_ARG(SharedUiItem, SharedUiItem()));
}
