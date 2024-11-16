/* Copyright 2014-2024 Hallowyn, Gregoire Barbier and others.
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
#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include "libp6core_global.h"
#include <QWaitCondition>
#include <QMutexLocker>
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
 * Can hold any data with operator=():
 * - fundamental types: int, char_8t...
 * - stucts and pocos with operator=() (implicit or explicit)
 * - Qt's implicitly shared data classes exactly as then can be sent through
 *   a queued signal/slot connection or a queued QMetaObject::invokeMethod()
 *   call.
 *
 * T is not required to be thread-safe, even its operator=().
 */
template <class T>
class LIBP6CORESHARED_EXPORT CircularBuffer {
public:
  const size_t _sizeMinusOne;
  size_t _putCounter, _getCounter, _free, _used;
  QMutex _mutex;
  QWaitCondition _notEmpty, _notFull;
  T *_buffer;

public:
  /** @param sizePowerOf2 size of buffer (e.g. 10 means 1024 slots) */
  explicit inline CircularBuffer(unsigned sizePowerOf2)
    : _sizeMinusOne((1 << sizePowerOf2) - 1), _putCounter(0), _getCounter(0),
      _free(_sizeMinusOne+1), _used(0), _buffer(new T[_sizeMinusOne+1]) {
    if (sizePowerOf2 >= sizeof(_sizeMinusOne)*8)
      qWarning() << "CircularBuffer cannot hold buffer as large as 2 ^ "
                 << sizePowerOf2;
  }
  CircularBuffer(const CircularBuffer&) = delete;
  CircularBuffer &operator=(const CircularBuffer &) = delete;
  inline ~CircularBuffer() {
    delete[] _buffer;
  }
  /** Put data. If needed, wait until there are enough room in the buffer. */
  inline void put(T data) {
    do_put<true>(data, QDeadlineTimer(QDeadlineTimer::Forever));
  }
  /** Put data only if there are enough room for it.
   * @return true on success */
  inline bool tryPut(T data) {
    return do_put<false>(data, {});
  }
  /** Put data only if there are enough room for it within timeout milliseconds.
   * @return true on success */
  inline bool tryPut(T data, const QDeadlineTimer &deadline) {
    return do_put<true>(data, deadline);
  }
  /** Get data. If needed, wait until it become available. */
  inline T get() {
    T t;
    return do_get<true>(&t, QDeadlineTimer(QDeadlineTimer::Forever)) ? t : T();
  }
  /** Get data only if it is available.
   * @return true on success */
  inline bool tryGet(T *data) {
    return do_get<false>(data, {});
  }
  /** Get data only if it is available within timeout milliseconds.
   * @return true on success */
  inline bool tryGet(T *data, const QDeadlineTimer &deadline) {
    return do_get<true>(data, deadline);
  }
  /** Discard all data. If needed, wait until it become available. */
  void clear() {
    _mutex.lock();
    _getCounter = _putCounter;
    _used = 0;
    _free = _sizeMinusOne+1;
    _mutex.unlock();
    _notFull.wakeOne();
  }
  /** Total size of buffer. */
  inline size_t size() const { return _sizeMinusOne+1; }
  /** Currently free size of buffer.
   * Beware that this value is not consistent from thread to thread. */
  inline size_t free() const { return _free; }
  /** Currently used size of buffer.
   * Beware that this value is not consistent from thread to thread. */
  inline size_t used() const { return _used; }
  /** Number of successful put so far.
   * This method is only usefull for testing or benchmarking this class. */
  inline size_t putCounter() const { return _putCounter; }
  /** Number of successful get so far.
   * This method is only usefull for testing or benchmarking this class. */
  inline size_t getCounter() const { return _getCounter; }

private:
  template<bool SHOULD_WAIT>
  inline bool do_put(T data, const QDeadlineTimer &deadline) {
    _mutex.lock();
    while (_free == 0 ) {
      if (!SHOULD_WAIT || !_notFull.wait(&_mutex, deadline)) {
        _mutex.unlock();
        return false;
      }
    }
    // since size is a power of 2, % size === &(size-1)
    _buffer[_putCounter & (_sizeMinusOne)] = data;
    ++_putCounter;
    --_free;
    ++_used;
    _mutex.unlock();
    _notEmpty.wakeOne();
    return true;
  }
  template<bool SHOULD_WAIT>
  inline bool do_get(T *data, const QDeadlineTimer &deadline) {
    if (!data)
      return false;
    _mutex.lock();
    while (_used == 0) {
      if (!SHOULD_WAIT || !_notEmpty.wait(&_mutex, deadline)) {
        _mutex.unlock();
        return false;
      }
    }
    // since size is a power of 2, % size === &(size-1)
    *data = _buffer[_getCounter & (_sizeMinusOne)];
    _buffer[_getCounter & (_sizeMinusOne)] = T();
    ++_getCounter;
    --_used;
    ++_free;
    _mutex.unlock();
    _notFull.wakeOne();
    return true;
  }
};

#endif // CIRCULARBUFFER_H
