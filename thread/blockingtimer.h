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
#ifndef BLOCKINGTIMER_H
#define BLOCKINGTIMER_H

#include <QtGlobal>
#include "libqtssu_global.h"
#include <functional>

/** Blocking timer which calls QCoreApplication::processEvents when waiting
  * and does not drift as a simple sleep() or usleep() would.
  */
class LIBQTSSUSHARED_EXPORT BlockingTimer {
public:
  using ShouldStopFunction = std::function<bool()>;

private:
  quint64 _lasttick;
  quint32 _intervalMsec, _subintervalMsec;
  ShouldStopFunction _shouldStopFunction;
  bool _shouldCallProcessEvents;

public:
  /** @param subintervalMsec is bounded to 1 hour (any longer value
    * will lead to precessEvents() being called every hour).
    * @param intervalMsec time to wait between every call to wait()
    * @param subIntervalMsec time to wait between every call to processEvents()
    *   and/or to shouldStopFunction
    */
  BlockingTimer(quint32 intervalMsec, quint32 subntervalMsec = 200,
                ShouldStopFunction shouldStopFunction = 0,
                bool shouldCallProcessEvents = true);
  BlockingTimer(quint32 intervalMsec, ShouldStopFunction shouldStopFunction,
                bool shouldCallProcessEvents = true)
    : BlockingTimer(intervalMsec, 200, shouldStopFunction,
                    shouldCallProcessEvents) { }
  BlockingTimer(quint32 intervalMsec, bool shouldCallProcessEvents)
    : BlockingTimer(intervalMsec, 200, 0, shouldCallProcessEvents) { }
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
};

#endif // BLOCKINGTIMER_H
