/* Copyright 2013-2014 Hallowyn and others.
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
#ifndef TIMEFORMATS_H
#define TIMEFORMATS_H

#include "libqtssu_global.h"
#include <QString>
#include <QDateTime>

class TimeFormatsPrivate;

/** Utilites to handle date/time formats. */
class LIBQTSSUSHARED_EXPORT TimeFormats {
private:
  TimeFormatsPrivate *d;
  static inline TimeFormatsPrivate *instance();

public:
  /** Should never be called directly (only used for singleton init) */
  TimeFormats();
  // TODO switch to QDateTime::toString(Qt::RFC2822Date) now that it exists
  static QString toRfc2822DateTime(QDateTime dt);
  static QDateTime fromRfc2822DateTime(QString rfc2822DateTime,
                                       QString &errorString);
  static QDateTime fromRfc2822DateTime(QString rfc2822DateTime) {
    QString s;
    return fromRfc2822DateTime(rfc2822DateTime, s); }
  /** e.g. "1.250 seconds", "10 months and 3 days", "-10 months and 3 days"
   * @param absolute if false, add initial "-" if msec < 0 */
  static QString toCoarseHumanReadableTimeInterval(
      qint64 msecs, bool absolute = false);
  /** e.g. "1.250 seconds ago", "in 10 months and 3 days"
    * invalid QDateTime gives null QString */
  static QString toCoarseHumanReadableRelativeDate(
      QDateTime dt, QDateTime reference = QDateTime::currentDateTime());
};

#endif // TIMEFORMATS_H
