/* Copyright 2015 Hallowyn and others.
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
#ifndef ATOMICVALUE_H
#define ATOMICVALUE_H

#include <QMutex>
#include <QMutexLocker>

/** Class that protect access to a value object with a mutex providing the same
 * kind of protection than e.g. a QAtomicInteger despite using a less scalable
 * mean (QMutex + copying the value object for every access).
 *
 * Usable with cheap copyable objets that are not thread-safe themselves, like
 * 64 bits integers on 32 bits platforms or implicitly shared objects (e.g.
 * QString, QDateTime).
 *
 * Usage example:
 * AtomicValue<QString> threadSafeString;
 * ...
 * QString string = threadSafeString; // a mutex protects the (shallow) copy
 * string.replace("foo", "bar");
 * threadSafeString = string; // the same mutex protects the (deep) copy
 *
 * @see QAtomicInteger
 * @see QMutex
 */
template <class T>
class AtomicValue {
  T _data;
  mutable QMutex _mutex;

public:
  AtomicValue() {}
  explicit AtomicValue(T data) : _data(data) { }
  explicit AtomicValue(const AtomicValue<T> &other) : _data(other.data()) { }
  /** Get (take a copy of) holded data.
   * This method is thread-safe. */
  T data() const {
    QMutexLocker ml(&_mutex);
    return _data;
  }
  /** Convenience operator for data() */
  T operator*() const { return this->data(); }
  /** Convenience operator for data() */
  operator T() const { return this->data(); }
  /** Set (overwrite) holded data.
   * This method is thread-safe. */
  void setData(T other) {
    QMutexLocker ml(&_mutex);
    _data = other;
  }
  /** Convenience operator for setData() */
  AtomicValue<T> &operator=(T other) { setData(other); return *this; }
  /** Convenience method for setData(other.data()) */
  void setData(const AtomicValue<T> &other) { setData(other.data()); }
  /** Convenience operator for setData() */
  AtomicValue<T> &operator=(const AtomicValue<T> &other) {
    setData(other);
    return *this;
  }
  /** Lock and get (take a copy of) holded data, which disable any read and
   * write access until unlocked with unlockData().
   * Use with caution, since accessing through data() enable achieving shorter
   * lock durations. */
  T &lockData() {
    _mutex.lock();
    return _data;
  }
  /** Lock and get (take a copy of) holded data, which disable any read and
   * write access until unlocked with unlockData().
   * Use with caution, since accessing through data() enable achieving shorter
   * lock durations.
   * Never try to access data through returned reference on failure.
   * @param success *success is set to true on succes */
  T &tryLockData(bool *success, int timeout = 0) {
    if (success) {
      *success = _mutex.tryLock(timeout);
      if (*success)
        return _data;
    }
    return *(T*)0;
  }
  /** Unlock when previously locked by lockData() or tryLockData(). */
  void unlockData() {
    _mutex.unlock();
  }
};

#endif // ATOMICVALUE_H
