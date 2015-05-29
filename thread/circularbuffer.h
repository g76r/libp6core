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
#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include "libqtssu_global.h"
#include <QMutex>
#include <QWaitCondition>
#include <QtDebug>

// MAYDO add method for puting or geting several data items at a time
// QList<T> tryGetAll() : get all data currently available, or an empty list
// QList<T> tryGetAll(int timeout) : get all data as soon as there is at less 1 available within timeout ms
// QList<T> getAll() : get all data currently available, or wait until there is at less 1
// QList<T> waitAndGetAll(int interval) : get all data received within interval ms, maybe more than the buffer size, maybe an empty list
// bool tryPutAll(QList<T>) : put all data if there is enough room for the whole list
// bool tryPutAll(QList<T>, int timeout) : put all data as a whole if there is enough room within timeout ms
// void putAll(QList<T>) : put data for which there are enough room, then wait and do it again until all data has been put
// int putAsManyAsPossible(QList<T>) : put as many items as possible and return their count
// int putAsManyAsPossible(QList<T>, int timeout) : same but wait timeout ms if needed to put more data if possible

/** Thread-safe circular buffer.
 *
 * Usable as a multithreading queue communication mechanism.
 *
 * Can hold any data with operator=().
 *
 * The datatype is not required to be thread-safe, even its operator=().
 * Therefore Qt's implicitly shared data classes can be sent through a
 * CircularBuffer exactly as then can through an queued signal/slot connection
 * or a queued QMetaObject::invokeMethod() call.
 */
template <class T>
class LIBQTSSUSHARED_EXPORT CircularBuffer {
public:
  long _sizeMinusOne, _putCounter, _getCounter, _free, _used;
  QMutex _mutex;
  QWaitCondition _notEmpty, _notFull;
  T *_buffer;

public:
  /** @param sizePowerOf2 size of buffer (e.g. 10 means 1024 slots) */
  inline CircularBuffer(unsigned sizePowerOf2)
    : _sizeMinusOne((1 << sizePowerOf2) - 1), _putCounter(0), _getCounter(0),
      _free(_sizeMinusOne+1), _used(0), _buffer(new T[_sizeMinusOne+1]) {
    if (sizePowerOf2 >= sizeof(_sizeMinusOne)*8)
      qWarning() << "CircularBuffer cannot hold buffer as large as 2 ^ "
                 << sizePowerOf2;
  }
  inline ~CircularBuffer() {
    delete[] _buffer;
  }
  /** Put data. If needed, wait until there are enough room in the buffer. */
  inline void put(T data) {
    QMutexLocker locker(&_mutex);
    if (_free == 0)
      _notFull.wait(&_mutex);
    // since size is a power of 2, % size === &(size-1)
    _buffer[_putCounter & (_sizeMinusOne)] = data;
    ++_putCounter;
    --_free;
    ++_used;
    _notEmpty.wakeAll();
  }
  /** Put data only if there are enough room for it.
   * @return true on success */
  inline bool tryPut(T data) {
    QMutexLocker locker(&_mutex);
    if (_free == 0)
      return false;
    // since size is a power of 2, % size === &(size-1)
    _buffer[_putCounter & (_sizeMinusOne)] = data;
    ++_putCounter;
    --_free;
    ++_used;
    _notEmpty.wakeAll();
    return true;
  }
  /** Put data only if there are enough room for it within timeout milliseconds.
   * @return true on success */
  inline bool tryPut(T data, int timeout) {
    QMutexLocker locker(&_mutex);
    if (_free == 0 && !_notFull.wait(&_mutex, timeout))
      return false;
    // since size is a power of 2, % size === &(size-1)
    _buffer[_putCounter & (_sizeMinusOne)] = data;
    ++_putCounter;
    --_free;
    ++_used;
    _notEmpty.wakeAll();
    return true;
  }
  /** Get data. If needed, wait until it become available. */
  inline T get() {
    QMutexLocker locker(&_mutex);
    if (_used == 0)
      _notEmpty.wait(&_mutex);
    // since size is a power of 2, % size === &(size-1)
    T t = _buffer[_getCounter & (_sizeMinusOne)];
    _buffer[_getCounter & (_sizeMinusOne)] = T();
    ++_getCounter;
    --_used;
    ++_free;
    _notFull.wakeAll();
    return t;
  }
  /** Get data only if it is available.
   * @return true on success */
  inline bool tryGet(T *data) {
    QMutexLocker locker(&_mutex);
    if (!data || _used == 0)
      return false;
    // since size is a power of 2, % size === &(size-1)
    *data = _buffer[_getCounter & (_sizeMinusOne)];
    _buffer[_getCounter & (_sizeMinusOne)] = T();
    ++_getCounter;
    --_used;
    ++_free;
    _notFull.wakeAll();
    return true;
  }
  /** Get data only if it is available within timeout milliseconds.
   * @return true on success */
  inline bool tryGet(T *data, int timeout) {
    QMutexLocker locker(&_mutex);
    if (!data || (_used == 0 && !_notEmpty.wait(&_mutex, timeout)))
      return false;
    // since size is a power of 2, % size === &(size-1)
    *data = _buffer[_getCounter & (_sizeMinusOne)];
    _buffer[_getCounter & (_sizeMinusOne)] = T();
    ++_getCounter;
    --_used;
    ++_free;
    _notFull.wakeAll();
    return true;
  }
  /** Total size of buffer. */
  inline long size() const { return _sizeMinusOne+1; }
  /** Currently free size of buffer.
   * Beware that this value is not consistent from thread to thread. */
  inline long free() const { return _free; }
  /** Currently used size of buffer.
   * Beware that this value is not consistent from thread to thread. */
  inline long used() const { return _used; }
  /** Number of successful put so far.
   * This method is only usefull for testing or benchmarking this class. */
  inline long putCounter() const { return _putCounter; }
  /** Number of successful get so far.
   * This method is only usefull for testing or benchmarking this class. */
  inline long getCounter() const { return _getCounter; }
};

#endif // CIRCULARBUFFER_H
