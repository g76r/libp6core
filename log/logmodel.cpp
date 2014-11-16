/* Copyright 2013-2014 Hallowyn and others.
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
#include "logmodel.h"
#include "memorylogger.h"

#define COLUMNS 6

LogModel::LogModel(QObject *parent, Log::Severity minSeverity, int maxrows,
                   QString prefixFilter)
  : QAbstractListModel(parent), _maxrows(maxrows),
    _logger(new MemoryLogger(minSeverity, this)), _prefixFilter(prefixFilter) {
  Log::addLogger(_logger, false);
}

LogModel::LogModel(Log::Severity minSeverity, int maxrows, QString prefixFilter)
  : QAbstractListModel(0), _maxrows(maxrows),
    _logger(new MemoryLogger(minSeverity, this)), _prefixFilter(prefixFilter) {
  Log::addLogger(_logger, false);
}

LogModel::LogModel(QObject *parent, int maxrows, QString prefixFilter)
  : QAbstractListModel(parent), _maxrows(maxrows), _logger(0),
    _prefixFilter(prefixFilter) {
}

LogModel::LogModel(int maxrows, QString prefixFilter)
  : QAbstractListModel(0), _maxrows(maxrows), _logger(0),
    _prefixFilter(prefixFilter) {
}

LogModel::~LogModel() {
  if (_logger)
    Log::removeLogger(_logger);
}

int LogModel::rowCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  return parent.isValid() ? 0 : _log.size();
}

int LogModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  return COLUMNS;
}

QVariant LogModel::data(const QModelIndex &index, int role) const {
  if (index.isValid() && index.row() >= 0 && index.row() < _log.size()) {
    Logger::LogEntry le = _log[index.row()];
    switch(role) {
    case Qt::DisplayRole:
      switch(index.column()) {
      case 0:
        return le.timestamp().toString("yyyy-MM-dd hh:mm:ss,zzz");
      case 1:
        return le.task();
      case 2:
        return le.execId();
      case 3:
        return le.sourceCode();
      case 4:
        return le.severityText();
      case 5:
        return le.message();
      }
      break;
    default:
      ;
    }
  }
  return QVariant();
}

QVariant LogModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    switch(section) {
    case 0:
      return "Timestamp";
    case 1:
      return "Task";
    case 2:
      return "Execution id";
    case 3:
      return "Source";
    case 4:
      return "Severity";
    case 5:
      return "Message";
    }
  }
  return QVariant();
}

void LogModel::prependLogEntry(Logger::LogEntry entry) {
  if (!_prefixFilter.isNull() && !entry.message().startsWith(_prefixFilter))
    return;
  beginInsertRows(QModelIndex(), 0, 0);
  _log.prepend(entry);
  endInsertRows();
  if (_log.size() > _maxrows) {
    beginRemoveRows(QModelIndex(), _maxrows, _maxrows);
    _log.removeAt(_maxrows);
    endRemoveRows();
  }
}
