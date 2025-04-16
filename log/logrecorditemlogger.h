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
#ifndef LOGRECORDITEMLOGGER_H
#define LOGRECORDITEMLOGGER_H

#include "logger.h"
#include "modelview/shareduiitem.h"

namespace p6::log {

class LogRecordItemData;

class LogRecordItem : public SharedUiItem {
public:
  LogRecordItem() noexcept {}
  LogRecordItem(const Record &record);
  LogRecordItem(const LogRecordItem &other) noexcept : SharedUiItem(other) {}
  LogRecordItem(LogRecordItem &&other) noexcept
    : SharedUiItem(std::move(other)) {}
  LogRecordItem &operator=(const LogRecordItem &other) {
    SharedUiItem::operator=(other); return *this; }
  LogRecordItem &operator=(LogRecordItem &&other) {
    SharedUiItem::operator=(std::move(other)); return *this; }

private:
  inline const LogRecordItemData *data() const;
};

/** Logger producing a SharedUiItem for each log record.
 *  Used internaly by LogRecordItemModel, but could theorically be used for
 *  anything else.
 *  Adds itself to loggers (as a non removable logger) in constructor and
 *  removes itself in destructor.
 *  It's a DirectCall logger (it does not start a thread) and will be
 *  automaticaly moved to root logger's thread (and as any logger must not have
 *  a parent).
 * @see LogRecordItemModel */
class LIBP6CORESHARED_EXPORT LogRecordItemLogger : public Logger {
  friend class LogRecordItemModel;
  Q_OBJECT
  Q_DISABLE_COPY(LogRecordItemLogger)
  Utf8String _prefix_filter;

  // only LogModel can create a LogRecordItemLogger object
  LogRecordItemLogger(Severity min_severity, const Utf8String &prefix_filter);

public:
  ~LogRecordItemLogger();

signals:
  /** emited on every non-filtered log record, with a LogRecordItem new item. */
  void item_changed(const SharedUiItem &new_item, const SharedUiItem &old_item,
                    const Utf8String &qualifier);

protected:
  void do_log(const Record &record) override;
};

} // ns p6::log

Q_DECLARE_METATYPE(p6::log::LogRecordItem);
Q_DECLARE_TYPEINFO(p6::log::LogRecordItem, Q_RELOCATABLE_TYPE);

#endif // LOGRECORDITEMLOGGER_H
