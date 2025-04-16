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
#include "logrecorditemlogger.h"
#include "logrecorditemmodel.h"

namespace p6::log {

LogRecordItemLogger::LogRecordItemLogger(
    Severity min_severity, const Utf8String &prefix_filter)
  : Logger(min_severity, Logger::DirectCall), _prefix_filter(prefix_filter) {
  addLogger(this, false);
}

LogRecordItemLogger::~LogRecordItemLogger() {
  removeLogger(this);
}

void LogRecordItemLogger::do_log(const Record &record) {
  if (!_prefix_filter.isNull() && !record.message().startsWith(_prefix_filter))
    return;
  auto record_item = LogRecordItem(record);
  emit item_changed(record_item, {}, record_item.qualifier());
}

static Utf8StringList _uiHeaderNames {
  "Timestamp", // 0
  "Task id",
  "Execution id",
  "Location",
  "Severity",
  "Message" // 5
};

static Utf8StringList _uiSectionNames {
  "timestamp", // 0
  "taskid",
  "execid",
  "location",
  "severity",
  "message" // 5
};

static auto _uiSectionIndex = ContainerUtils::index(_uiSectionNames);

static QAtomicInt _sequence;

class LogRecordItemData : public SharedUiItemData, public Record {
public:
  Utf8String _id;
  LogRecordItemData(const Record &record)
    : Record(record),
      _id(QByteArray::number(_sequence.fetchAndAddOrdered(1))) { }
  QVariant uiData(int section, int role) const override;
  QVariant uiHeaderData(int section, int role) const override {
    switch (role) {
      case Qt::DisplayRole:
      case Qt::EditRole:
      case SharedUiItem::ExternalDataRole:
        return uiSectionName(section);
    }
    return {};
  }
  int uiSectionCount() const override { return _uiSectionNames.size(); }
  Utf8String uiSectionName(int section) const override {
    return _uiSectionNames.value(section); }
  int uiSectionByName(Utf8String sectionName) const override {
    return _uiSectionIndex.value(sectionName, -1); }
  Utf8String id() const override { return _id; }
  Utf8String qualifier() const override { return "logrecorditem"_u8; }
};

LogRecordItem::LogRecordItem(const Record &record)
  : SharedUiItem(new LogRecordItemData(record)) {
}

QVariant LogRecordItemData::uiData(
    int section, int role) const {
  switch(role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    switch(section) {
    case 0:
      return QDateTime::fromMSecsSinceEpoch(_timestamp)
          .toString(u"yyyy-MM-dd hh:mm:ss,zzz"_s);
    case 1:
      return _taskid;
    case 2:
      return _execid;
    case 3:
      return _location;
    case 4:
      return severity_as_text(_severity);
    case 5:
      return _message;
    }
    break;
  default:
    ;
  }
  return {};
}

const LogRecordItemData *LogRecordItem::data() const {
  return specializedData<LogRecordItemData>();
}

} // ns p6
