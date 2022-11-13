/* Copyright 2013-2022 Hallowyn, Gregoire Barbier and others.
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
#include "logmodel.h"
#include "memorylogger.h"

LogModel::LogModel(QObject *parent, Log::Severity minSeverity, int maxrows,
                   QString prefixFilter)
  : SharedUiItemsTableModel(parent),
    _logger(new MemoryLogger(minSeverity, prefixFilter, this)) {
  setMaxrows(maxrows);
  setDefaultInsertionPoint(SharedUiItemsTableModel::FirstItem);
  setHeaderDataFromTemplate(
        Logger::LogEntry(QDateTime(), QString(), Log::Debug, QString(),
                         QString(), QString()));
  Log::addLogger(_logger, false);
}

LogModel::LogModel(QObject *parent, int maxrows)
  : SharedUiItemsTableModel(parent), _logger(0) {
  setMaxrows(maxrows);
  setDefaultInsertionPoint(SharedUiItemsTableModel::FirstItem);
  setHeaderDataFromTemplate(
        Logger::LogEntry(QDateTime(), QString(), Log::Debug, QString(),
                         QString(), QString()));
}

LogModel::~LogModel() {
  if (_logger)
    Log::removeLogger(_logger);
}
