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
#ifndef LOGMODEL_H
#define LOGMODEL_H

#include "modelview/shareduiitemstablemodel.h"
#include "logger.h"

class MemoryLogger;

// LATER remove log entries depending on their age too

/** Model to hold and optionnaly (see constructors) collect log entries.
 * Contains a log entry per row, the first row being the last recorded entry
 * when automatically collecting.
 * @see MemoryLogger */
class LIBQTSSUSHARED_EXPORT LogModel : public SharedUiItemsTableModel {
  Q_OBJECT
  Q_DISABLE_COPY(LogModel)
  MemoryLogger *_logger;

public:
  /** Create a model that collects log entries with severity >= minSeverity. */
  LogModel(QObject *parent, Log::Severity minSeverity, int maxrows = 100,
           QString prefixFilter = QString());
  /** Create a model that collects log entries with severity >= minSeverity. */
  explicit LogModel(Log::Severity minSeverity, int maxrows = 100,
                    QString prefixFilter = QString())
    : LogModel(0, minSeverity, maxrows, prefixFilter) { }
  /** Create a model that collects log entries with severity >= minSeverity. */
  LogModel(Log::Severity minSeverity, QString prefixFilter)
    : LogModel(0, minSeverity, 100, prefixFilter) { }
  /** Create a model that collects log entries with severity >= minSeverity. */
  LogModel(Log::Severity minSeverity, const char *prefixFilter)
    : LogModel(0, minSeverity, 100, prefixFilter) { }
  /** Create a model that collects log entries with severity >= minSeverity. */
  LogModel(QObject *parent, Log::Severity minSeverity, const char *prefixFilter)
    : LogModel(parent, minSeverity, 100, prefixFilter) { }
  /** Create a model that do not collect any log entry (prependLogEntry() must
   * be called to fill-in the model by hand). */
  explicit LogModel(QObject *parent, int maxrows = 100);
  /** Create a model that do not collect any log entry (prependLogEntry() must
   * be called to fill-in the model by hand). */
  explicit LogModel(int maxrows = 100) : LogModel(0, maxrows) { }
  ~LogModel();
  MemoryLogger *logger() const { return _logger; }
};

#endif // LOGMODEL_H
