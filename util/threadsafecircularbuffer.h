/* Copyright 2014-2015 Hallowyn and others.
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
#ifndef THREADSAFECIRCULARBUFFER_H
#define THREADSAFECIRCULARBUFFER_H

#include "twothreadscircularbuffer.h"
#include <QMutex>

/** Circular buffer which stays thread-safe regardless the number of producer
 * or consummer threads. */
template <class T>
class LIBQTSSUSHARED_EXPORT ThreadSafeCircularBuffer
    : public TwoThreadsCircularBuffer<T> {
  QMutex _putMutex, _getMutex;

public:
  inline ThreadSafeCircularBuffer(int sizePowerOf2)
    : TwoThreadsCircularBuffer<T>(sizePowerOf2) {
  }
  void put(T data) {
    QMutexLocker locker(&_putMutex);
    TwoThreadsCircularBuffer<T>::put(data);
  }
  T get() {
    QMutexLocker locker(&_getMutex);
    return TwoThreadsCircularBuffer<T>::get();
  }
  bool tryPut(T data) {
    QMutexLocker locker(&_putMutex);
    return TwoThreadsCircularBuffer<T>::tryPut(data);
  }
  T tryGet() {
    QMutexLocker locker(&_getMutex);
    return TwoThreadsCircularBuffer<T>::tryGet();
  }
};

#endif // THREADSAFECIRCULARBUFFER_H
