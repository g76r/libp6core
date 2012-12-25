/*
Copyright 2012 Hallowyn and others.
See the NOTICE file distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file to you under
the Apache License, Version 2.0 (the "License"); you may not use this
file except in compliance with the License. You may obtain a copy of the
License at http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations
under the License.
*/
#include "blockingtimer.h"
#include <QDateTime>
#include <unistd.h>
#include <QCoreApplication>

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

quint64 BlockingTimer::msecSince1970() {
#if QT_VERSION >= 0x040700
  return QDateTime::currentMSecsSinceEpoch();
#else
  static QDateTime epoch(QDate(1970, 1, 1), QTime(0, 0));
  return epoch.secsTo(QDateTime::currentDateTime())*1000;
#endif
}
