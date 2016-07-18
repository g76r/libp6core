/* Copyright 2012-2016 Hallowyn and others.
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

BlockingTimer::BlockingTimer(quint32 intervalMsec, quint32 subntervalMsec,
    ShouldStopFunction shouldStopFunction, bool shouldCallProcessEvents)
  : _lasttick(0), _intervalMsec(intervalMsec),
    _subintervalMsec(subntervalMsec),
    _shouldStopFunction(shouldStopFunction),
    _shouldCallProcessEvents(shouldCallProcessEvents) {
}

void BlockingTimer::wait() {
  if (_lasttick == 0)
    _lasttick = QDateTime::currentMSecsSinceEpoch();
  quint64 now = _lasttick, nexttick = _lasttick+_intervalMsec;
  if (_shouldCallProcessEvents)
    QCoreApplication::processEvents();
  while ((!_shouldStopFunction || !_shouldStopFunction())
         && (now = QDateTime::currentMSecsSinceEpoch()) < nexttick) {
    // Bounding the wait time to 1 hour to avoid overflow when converting to
    // microseconds (microseconds in 22 bits would lead to bugs for intervals
    // longer than about 2 hours 1/2).
    // Cast to quint32 is obviously safe thanks to the bounds.
    // nexttick-now is always >= 0 since _lasttick is always <= now
    quint32 timeToWait = (quint32)qBound<quint64>(0, nexttick-now, 3600000);
    if (_subintervalMsec > 0)
      timeToWait = qMin(timeToWait, _subintervalMsec);
    usleep(1000*timeToWait);
    if (_shouldCallProcessEvents)
      QCoreApplication::processEvents();
  }
  _lasttick = now;
}
