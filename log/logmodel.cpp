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
#include "logmodel.h"

LogModel::LogModel(QObject *parent, int maxrows) : QAbstractListModel(parent),
  _maxrows(maxrows) {
}

#define COLUMNS 6

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
    const LogEntry &le = _log.at(index.row());
    switch(role) {
    case Qt::DisplayRole:
      switch(index.column()) {
      case 0:
        return le._timestamp;
      case 1:
        return le._task;
      case 2:
        return le._execId;
      case 3:
        return le._sourceCode;
      case 4:
        return le._severityText;
      case 5:
        return le._message;
      }
      break;
    case HtmlPrefixRole:
      if (index.column() == 5)
        switch (le._severity) {
        case Log::Warning:
          return _warningIcon;
        case Log::Error:
        case Log::Fatal:
          return _errorIcon;
        default:
          ;
        }
      break;
    case TrClassRole:
      switch (le._severity) {
      case Log::Warning:
        return _warningTrClass;
      case Log::Error:
      case Log::Fatal:
        return _errorTrClass;
      default:
        ;
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

void LogModel::log(QDateTime timestamp, QString message, Log::Severity severity,
                   QString task, QString execId, QString sourceCode) {
  beginInsertRows(QModelIndex(), 0, 0);
  _log.prepend(LogEntry(timestamp, message, severity, task, execId,
                        sourceCode));
  endInsertRows();
  if (_log.size() > _maxrows) {
    beginRemoveRows(QModelIndex(), _maxrows, _maxrows);
    _log.removeAt(_maxrows);
    endRemoveRows();
  }
}
