/* Copyright 2013-2025 Hallowyn, Gregoire Barbier and others.
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
#ifndef LOGRECORDITEMMODEL_H
#define LOGRECORDITEMMODEL_H

#include "modelview/shareduiitemstablemodel.h"
#include "logger.h"

namespace p6::log {

class LogRecordItemLogger;

// LATER remove log records depending on their age too

/** Model to hold and collect log records.
 *  Contains a log record per row, the first row being the last recorded record
 *  when automatically collecting.
 *  Internaly uses (= starts and registers) a SharedUiItemLogger.
 *  @see LogRecordItemLogger
 */
class LIBP6CORESHARED_EXPORT LogRecordItemModel
    : public SharedUiItemsTableModel {
  Q_OBJECT
  Q_DISABLE_COPY(LogRecordItemModel)

public:
  /** Create a model that collects log records with severity >= min_severity. */
  LogRecordItemModel(
      QObject *parent, Severity min_severity, int maxrows = 100,
      const Utf8String &prefix_filter = {});
  /** Create a model that collects log records with severity >= min_severity. */
  explicit LogRecordItemModel(Severity min_severity, int maxrows = 100,
                    Utf8String prefixFilter = {})
    : LogRecordItemModel(0, min_severity, maxrows, prefixFilter) { }
  /** Create a model that collects log records with severity >= min_severity. */
  LogRecordItemModel(Severity min_severity, Utf8String prefixFilter)
    : LogRecordItemModel(0, min_severity, 100, prefixFilter) { }
  /** Create a model that collects log records with severity >= min_severity. */
  LogRecordItemModel(Severity min_severity, const char *prefixFilter)
    : LogRecordItemModel(0, min_severity, 100, prefixFilter) { }
  /** Create a model that collects log records with severity >= min_severity. */
  LogRecordItemModel(QObject *parent, Severity min_severity,
                       const char *prefixFilter)
    : LogRecordItemModel(parent, min_severity, 100, prefixFilter) { }
};

} // ns p6::log

#endif // LOGRECORDITEMMODEL_H
