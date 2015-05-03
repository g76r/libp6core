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

#include "libqtssu_global.h"
#include <QSemaphore>
#include <QMutex>

/** Circular buffer which stays thread-safe regardless the number of producer
 * or consummer threads. */
template <class T>
class LIBQTSSUSHARED_EXPORT ThreadSafeCircularBuffer {
  long _sizeMinusOne, _putCounter, _getCounter;
  QSemaphore _free, _used;
  QMutex _putMutex, _getMutex;
  T *_buffer;

public:
  inline ThreadSafeCircularBuffer(int sizePowerOf2)
    : _sizeMinusOne((1 << sizePowerOf2) - 1), _putCounter(0), _getCounter(0),
      _free(_sizeMinusOne+1), _used(0), _buffer(new T[_sizeMinusOne+1]) {
  }
  inline ~ThreadSafeCircularBuffer() {
    delete[] _buffer;
  }
  inline void put(T data) {
    QMutexLocker locker(&_putMutex);
    _free.acquire();
    // since size is a power of 2, % size === &(size-1)
    _buffer[_putCounter++ & (_sizeMinusOne)] = data;
    _used.release();
  }
  inline T get() {
    QMutexLocker locker(&_getMutex);
    _used.acquire();
    // since size is a power of 2, % size === &(size-1)
    T t = _buffer[_getCounter++ & (_sizeMinusOne)];
    _free.release();
    return t;
  }
  inline bool tryPut(T data) {
    QMutexLocker locker(&_putMutex);
    if (_free.tryAcquire()) {
      // since size is a power of 2, % size === &(size-1)
      _buffer[_putCounter++ & (_sizeMinusOne)] = data;
      _used.release();
      return true;
    }
    return false;
  }
  /** @return T() if there is no available data */
  inline T tryGet() {
    QMutexLocker locker(&_getMutex);
    if (_used.tryAcquire()) {
      // since size is a power of 2, % size === &(size-1)
      T t = _buffer[_getCounter++ & (_sizeMinusOne)];
      _free.release();
      return t;
    }
    return T();
  }
  // LATER add method for puting or geting several data items at a time
  inline long size() const { return _sizeMinusOne+1; }
  inline long free() const { return _free.available(); }
  inline long used() const { return _used.available(); }
};

#endif // THREADSAFECIRCULARBUFFER_H
