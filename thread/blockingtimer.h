/* Copyright 2012 Hallowyn and others.
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
#ifndef BLOCKINGTIMER_H
#define BLOCKINGTIMER_H

#include <QtGlobal>
#include "libqtssu_global.h"

/** Blocking timer which calls QCoreApplication::processEvents when waiting
  * and does not drift as a simple sleep() or usleep() would.
  */
class LIBQTSSUSHARED_EXPORT BlockingTimer {
private:
  quint64 _lasttick;
  quint32 _intervalMsec, _processEventsIntervalMsec;
  bool _shouldStop;

public:
  /** @param processEventsIntervalMsec is bounded to 1 hour (any longer value
    * will lead to precessEvents() being called every hour).
    */
  BlockingTimer(quint32 intervalMsec, quint32 processEventsIntervalMsec = 200);
  /** Wait for the next trigger time.
    * To avoid drifting, this method does not wait intervalMsec but waits until
    * intervalMsec + last trigger time. If wait has been last called for longer
    * than intervalMsec, it returns immediatly.
    * This method calls QCoreApplication::processEvents() every
    * processEventsIntervalMsec milliseconds (and calls it once before returning
    * immediatly in case it has been called for longer than intervalMsec).
    * If processEventsIntervalMsec <= 0, processEvents is never called.
    */
  void wait();
  /** Stop waiting as soon as possible (i.e. within least of intervalMsec,
    * processEventsIntervalMsec and 1 hour).
    * This method can be called from another thread than the one currently
    * waiting and can also be called from within a queued slot in the waiting
    * thread (since wait calls QCoreApplication::processEvents()).
    * If wait() is not running, then this method has no effect.
    */
  inline void stopWaiting() { _shouldStop = true; }
  /** Return current date in millisecondes since 1970-01-01 00:00:00.
    * With Qt >= 4.7 this is the same as QDateTime::currentMSecsSinceEpoch().
    */
  static quint64 msecSince1970();
};

#endif // BLOCKINGTIMER_H
