/* Copyright 2014-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef RELATIVEDATETIME_H
#define RELATIVEDATETIME_H

#include <QSharedDataPointer>
#include <QString>
#include <QDateTime>
#include "libp6core_global.h"

class RelativeDateTimeData;

/** Represents a point in time relative to another point. Like natural language
 * expression like "two days ago", "tomorrow" or "last monday at 1 a.m." do.
 * Or more formally "two days before", "the day after" or "previous monday at
 * 1 a.m." since the reference point may be different of "now".
 *
 * Support for a consise notation for relative (or even absolute) time
 * expression like those:
 * "2014-06-26T23:02:43,221"
 * "12:02:43" -> today at 12:02:43 p.m.
 * "-4d" -> 4 days ago
 * "00:00" -> today at midnight
 * "00:00+2h-4min" -> today at 02:04 a.m., same as "02:04"
 * "12:02-4days" -> 12:02 p.m. 4 days ago
 * "01-02" -> last january 2nd (yesterday on jan 3rd, almost 1 year ago on
 * jan 1st)
 * "01-2w+4d" -> 10 days before last january 1st
 * "monday+2h-4min"
 * "00:00-1d" -> yesterday at midnight.
 * "-3months" -> same time 93 days ago
 * "12:02:43-3months" -> 12:02:43 p.m. 93 days ago
 *
 * More formally the expression is a sum which first term can be either an
 * absolute timestamp or an incomplete (hence relative) timestamp or week
 * day in english (e.g. "monday"). Following terms are of the form
 * "([+-][0-9.]+[a-z]+)+" (e.g. "-1day+2hours").
 *
 * Supported timestamp are ISO8601-like. As compared to strict ISO8601, they
 * support space instead of T between date and time, they can be truncated to
 * partial forms in reverse order as compared to what ISO8601 permits, like
 * this: "06-26 12:02" "26T12:02" "12:02" "26", but not in the way ISO8601
 * states (e.g. "2014-01" is invalid), and colons cannot be omitted.
 * When only the time is specified, the date is assumed to be the reference day
 * (e.g. today).
 * When only the date is specified, the time is assumed to be midnight.
 * When the date is truncated (e.g. "06-26" or "26"), it represents last
 * occurence of such date, i.e. "06-26" means yesterday on june the 27th and
 * means almost one year ago on june the 25th.
 * When truncated to only one number (e.g. "26") it is assumed to be the day
 * of month, therefore specifiying 9 p.m. must be done with "21:00" not "21"
 * which means "last 21st at midnight".
 *
 * Supported time units in sum terms are: year (meaning 366 days),
 * month (meaning 31 days), week (meaning 7 days), day, hour, minute, second,
 * milliseconds.
 * They can be used with plural mark or abbreviated to y, mon, w, d, h, min, s,
 * mil, ms.
 *
 * Week day names can be abbreviated to their 3 first letters.
 * Non significant 0 in month or day cannot be omitted in timestamp (e.g.
 * "2038-1-1" or "1-1" or even "1" are invalid).
 * Case is not significant.
 * When no reference (being it a timestamp or week day) is specified, reference
 * date and time are used (e.g. "-1min" for one minute ago from the reference
 * date and time, e.g. now).
 */
class LIBPUMPKINSHARED_EXPORT RelativeDateTime {
  QSharedDataPointer<RelativeDateTimeData> d;

public:
  RelativeDateTime();
  RelativeDateTime(QString pattern);
  RelativeDateTime(const RelativeDateTime &);
  RelativeDateTime &operator=(const RelativeDateTime &);
  ~RelativeDateTime();
  bool isNull();
  /** Apply relative date pattern to 'reference'.
   * If RelativeDateTime is null, return 'reference' as is. */
  QDateTime apply(QDateTime reference = QDateTime::currentDateTime());
};

#endif // RELATIVEDATETIME_H
