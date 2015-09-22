/* Copyright 2012-2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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
#include "blockingtimer.h"
#include <QDateTime>
#include <unistd.h>
#include <QCoreApplication>

#if QT_VERSION < 0x040700
static QDateTime epoch(QDate(1970, 1, 1), QTime(0, 0));
#endif

static inline quint64 msecSince1970() {
#if QT_VERSION >= 0x040700
  return QDateTime::currentMSecsSinceEpoch();
#else
  return epoch.secsTo(QDateTime::currentDateTime())*1000;
#endif
}

BlockingTimer::BlockingTimer(quint32 intervalMsec,
                             quint32 processEventsIntervalMsec)
  : _lasttick(0), _intervalMsec(intervalMsec),
    _processEventsIntervalMsec(processEventsIntervalMsec), _shouldStop(false) {
}

void BlockingTimer::wait() {
  if (_lasttick == 0)
    _lasttick = msecSince1970();
  quint64 now = _lasttick, nexttick = _lasttick+_intervalMsec;
  _shouldStop = false;
  if (_processEventsIntervalMsec > 0)
    QCoreApplication::processEvents();
  while (!_shouldStop && (now = msecSince1970()) < nexttick) {
    // Bounding the wait time to 1 hour to avoid overflow when converting to
    // microseconds (microseconds in 22 bits would lead to bugs for intervals
    // longer than about 2 hours 1/2).
    // Cast to quint32 is obviously safe thanks to the bounds.
    // nexttick-now is always >= 0 since _lasttick is always <= now
    quint32 timeToWait = (quint32)qBound<quint64>(0, nexttick-now, 3600000);
    if (_processEventsIntervalMsec)
      timeToWait = qMin(timeToWait, _processEventsIntervalMsec);
    usleep(1000*timeToWait);
    if (_processEventsIntervalMsec)
      QCoreApplication::processEvents();
  }
  _lasttick = now;
}
