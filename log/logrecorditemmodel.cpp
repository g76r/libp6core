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
#include "logrecorditemmodel.h"
#include "logrecorditemlogger.h"

namespace p6::log {

LogRecordItemModel::LogRecordItemModel(
    QObject *parent, Severity min_severity, int maxrows,
    const Utf8String &prefix_filter)
  : SharedUiItemsTableModel(parent) {
  setMaxrows(maxrows);
  setDefaultInsertionPoint(SharedUiItemsTableModel::FirstItem);
  setHeaderDataFromTemplate(LogRecordItem(Record{}));
  auto logger = new LogRecordItemLogger(min_severity, prefix_filter);
  connect(logger, &LogRecordItemLogger::item_changed,
          this, &SharedUiItemsModel::changeItem);
  connect(this, &QObject::destroyed,
          logger, &QObject::deleteLater);
}

} // ns p6::log
