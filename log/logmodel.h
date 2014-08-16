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
#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QAbstractListModel>
#include "logger.h"

class MemoryLogger;

/** Model to hold and optionnaly (see constructors) collect log entries.
 * Contains a log entry per row, the first row being the last recorded entry
 * when automatically collecting.
 * @see MemoryLogger */
class LIBQTSSUSHARED_EXPORT LogModel : public QAbstractListModel {
  Q_OBJECT
  Q_DISABLE_COPY(LogModel)
  QList<Logger::LogEntry> _log;
  int _maxrows; // LATER remove log entries depending on their age too
  MemoryLogger *_logger;

public:
  /** Create a model that collects log entries with severity >= minSeverity. */
  LogModel(QObject *parent, Log::Severity minSeverity, int maxrows = 100);
  /** Create a model that collects log entries with severity >= minSeverity. */
  explicit LogModel(Log::Severity minSeverity, int maxrows = 100);
  /** Create a model that do not collect any log entry (prependLogEntry() must
   * be called to fill-in the model by hand). */
  explicit LogModel(QObject *parent, int maxrows = 100);
  /** Create a model that do not collect any log entry (prependLogEntry() must
   * be called to fill-in the model by hand). */
  explicit LogModel(int maxrows = 100);
  ~LogModel();
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  MemoryLogger *logger() const { return _logger; }
  /** This method is not thread-safe, it must be called within the LogModel
   * thread (generally speaking, it is the gui thread).
   * If needed this can be done through QMetaObject::invokeMethod(). */
  Q_INVOKABLE void prependLogEntry(Logger::LogEntry entry);
};

#endif // LOGMODEL_H
