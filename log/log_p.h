/* Copyright 2025 Gregoire Barbier and others.
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
#ifndef LOG_P_H
#define LOG_P_H

#include "log/log.h"

namespace p6::log {

void LIBP6CORESHARED_EXPORT stderr_direct_log(const Record &record);

#if LOG_LOCATION_ENABLED
inline void stderr_direct_log(
    const Utf8String &message, Severity severity, const Utf8String &taskid = {},
    const Utf8String &execid = {},
    source_location location = source_location::current()) {
  stderr_direct_log(Record{severity, taskid, execid, location}.set_message(message));
}
#else
inline void stderr_direct_log(
    const Utf8String &message, Severity severity, const Utf8String &taskid = {},
    const Utf8String &execid = {}) {
  stderr_direct_log(Record{severity, taskid, execid}.set_message(message));
}
#endif // LOG_LOCATION_ENABLED

}

#endif // LOG_P_H
