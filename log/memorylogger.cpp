/* Copyright 2013-2021 Hallowyn, Gregoire Barbier and others.
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
#include "memorylogger.h"
#include "logmodel.h"
#include <QMetaObject>

MemoryLogger::MemoryLogger(
    Log::Severity minSeverity, QString prefixFilter, LogModel *logmodel)
  : Logger(minSeverity, Logger::DirectCall), _prefixFilter(prefixFilter),
    _model(logmodel) {
}

void MemoryLogger::doLog(const LogEntry &entry) {
  if (!_prefixFilter.isNull() && !entry.message().startsWith(_prefixFilter))
    return;
  QMetaObject::invokeMethod(_model, [entry,this](){
    _model->changeItem(entry, SharedUiItem(), entry.idQualifier());
  });
}
