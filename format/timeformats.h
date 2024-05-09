/* Copyright 2013-2024 Hallowyn, Gregoire Barbier and others.
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
#ifndef TIMEFORMATS_H
#define TIMEFORMATS_H

#include "util/relativedatetime.h"
#include "util/paramset.h"
#include <QTimeZone>

/** Utilites to handle date/time formats. */
class LIBP6CORESHARED_EXPORT TimeFormats {

public:
  TimeFormats() = delete;
  // TODO switch to QDateTime::toString(Qt::RFC2822Date) now that it exists
  static QString toRfc2822DateTime(const QDateTime &dt);
  static QDateTime fromRfc2822DateTime(
      const QString &rfc2822DateTime, QString *errorString = 0);
  /** e.g. "1.250 seconds", "10 months and 3 days", "-10 months and 3 days"
   * @param absolute if false, add initial "-" if msec < 0 */
  static QString toCoarseHumanReadableTimeInterval(
      qint64 msecs, bool absolute = false);
  /** e.g. "1.250 seconds ago", "in 10 months and 3 days"
    * invalid QDateTime gives null QString */
  static QString toCoarseHumanReadableRelativeDate(
      const QDateTime &dt,
      const QDateTime &reference = QDateTime::currentDateTime());
  /** Format a given timestamp using given format and RelativeDateTime shift.
   *
   * Supported format strings are those supported by QDateTime::toString() plus:
   * - "s1970" : seconds since 1970-01-01 00:00:00
   * - "ms1970" : milliseconds since 1970-01-01 00:00:00
   * - empty string defaults to pseudo-iso8601: yyyy-MM-dd hh:mm:ss,zzz
   */
  static QString toCustomTimestamp(
      const QDateTime &dt, const QString &format = {},
      const RelativeDateTime &relativeDateTime = {},
      const QTimeZone &tz = {});
  /** Syntactic sugar over toCustomTimestamp with an multifieldSpecifiedFormat
   * parameter of the form !format!relativedatetime!timezone
   *
   * format defaults to pseudo-iso-8601 "yyyy-MM-dd hh:mm:ss,zzz"
   * relativedatetime defaults to plain QDateTime date
   * timezone defaults to the one holded by QDateTime
   *
   * Any other character than ! can be used (the same way that sed s command
   * accepts any character beside / as a regular expression separator).
   *
   * examples:
   * !yyyy-MM-dd
   * ::-2days
   * !!!UTC
   * !hh:mm:ss,zzz!01-01T20:02-2w+1d!GMT
   *
   * This method is used by ParamSet for its %=date function:
   * %=date!format!relativedatetime!timezone
   * and is usable by any other kind of timestamp formating within a
   * ParamsProvider.
   *
   * Internally it uses RelativeDateTime class, with a cache on relativedatetime
   * field value.
   *
   * @see RelativeDateTime
   */
  static const QString toMultifieldSpecifiedCustomTimestamp(
      const QDateTime &dt, const Utf8String &multifieldSpecifiedFormat,
      const ParamsProvider::EvalContext &context = {});
  /** Creates a QTimeZone from an ISO 8601 pattern.
   *  Returns UTC on "+00:00" "-00:00" and "Z".
   *  Returns UTC offset timezone on other "+-nn:nn" patterns.
   *  Returns defaultValue if empty or invalid.
   *  Trims before analyzing pattern.
   */
  static const QTimeZone tzFromIso8601(
      const QString &offset, const QTimeZone &defaultValue = {});
};

#endif // TIMEFORMATS_H
