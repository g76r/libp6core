/* Copyright 2015-2023 Hallowyn, Gregoire Barbier and others.
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
#ifndef ATOMICVALUE_H
#define ATOMICVALUE_H

#include "libp6core_global.h"

/** Class protecting access to a value object with a mutex providing the same
 * kind of protection than e.g. a QAtomicInteger despite using a less scalable
 * mean (QMutex + copying the value object for every access).
 *
 * Usable with cheap copyable objets that are not thread-safe themselves, like
 * 64 bits integers on 32 bits platforms or implicitly shared objects (e.g.
 * QString, QDateTime).
 *
 * Warning: Qt implicit sharing mechanisms perform shallow copy, which is ok
 * only provided every copy is used read-only in every thread. That is: nobody
 * ever modify an object received through AtomicValue::data(), the only "write"
 * operation allowed is replacing the data using AtomicValue::setData() or its
 * convenience operator =. This is obviously also true for containers (QList...)
 * for which a shallow copy means that the contained objects are not copied at
 * all. More or less this is ok if holded data is immutable or used as such.
 *
 * Qt implictly shared types (e.g. QString, QDateTime and containers e.g. QList,
 * QHash) can also be holded and copied fully thread-safe if calling
 * detachedData() instead of data(), which will perform the deep copy within the
 * mutex critic section, provided the type have a detach() method.
 *
 * The alternative, more explicit and with longer critical sections way to use
 * AtomicValue, either with lockData() and unlockData() methods or using the
 * RAII helper returned by lockedData(), works with any kind of holded data,
 * being it a simple type, an implicitly shared ones or anything. Provided locks
 * and unlocks are done without programmatic errors.
 *
 * Usage examples:
 * AtomicValue<quint64> threadSafeLongLong; // safe 64 bits on 32 bits platforms
 * ...
 * quint64 ll = threadSafeLongLong.data(); // mutex-protected (deep) copy
 * ++ll;  // safe since main data is not accessed
 * threadSafeLongLong = ll; // mutex-protected (deep) copy
 *
 * AtomicValue<QString> threadSafeString;
 * ...
 * QString string = threadSafeString.detachedData();// mutex-protected deep copy
 * string.replace("foo", "bar"); // safe since main object is not accessed
 * // but another thread can read or set threadSafeString meanwhile
 * threadSafeString = string; // mutex-protected (deep) copy
 * ...
 * QString &ref = threadSafeString.lockData(); // explicit lock
 * ref.replace("foo", "bar"); // safe since main object is locked
 * // no other thread can read or set threadSafeString meanwhile
 * // the lock is holded longer and the syntax is more explicit
 * threadSafeString.unlockData(); // explicit unlock
 *
 * Same with the RAII helper:
 * auto string = threadSafeString.lockedData(); // explicit lock
 * string.replace("foo", "bar"); // safe since main object is locked
 * string.unlock(); // or wait for string destructor to unlock the mutex
 *
 * If an implictly-shared type is used only as read-only after being set in the
 * AtomicValue<> data() is enough and detach() method is not needed. AtamicValue
 * will still ensure that setting and getting data from it is thread-safe and
 * (hence the name) atomic. Example:
 *
 * AtomicValue<QString> threadSafeStringHodler;
 * ...
 * threadSafeStringHodler = "any text";
 * ...
 * // in another thread
 * QString string = threadSafeStringHodler;
 * qDebug() << string; // or any other read-only usage
 *
 * @see QAtomicInteger
 * @see QMutex
 */
template <class T>
class LIBP6CORESHARED_EXPORT AtomicValue {
  T _data;
  mutable QMutex _mutex;

public:
  AtomicValue() {}
  explicit AtomicValue(T data) : _data(data) { }
  explicit AtomicValue(const AtomicValue<T> &other) : _data(other.data()) { }
  /** Take a copy of holded data.
   * Warning: if holded data needs a deep copy, rather use detachedData() or
   * lockedData().
   * @see detachedData()
   * @see lockedData()
   * This method is thread-safe. */
  T data() const {
    QMutexLocker ml(&_mutex);
    return _data;
  }
  /** Take a deep copy of holded data, calling T.detach().
   * Works with Qt implicitly shared types that have a detach() method.
   * This method is thread-safe. */
  T detachedData() const {
    QMutexLocker ml(&_mutex);
    T data = _data; // shallow copy
    data.detach(); // deep copy
    return data;
  }
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
    setData(other.data());
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
  class LockedData {
    friend class AtomicValue<T>;
    AtomicValue<T> *_v;
    QMutexLocker<QMutex> _ml;
    LockedData(AtomicValue<T> *v) : _v(v), _ml(&v->_mutex) { }
  public:
    T& operator*() { return _v->_data; }
    T* operator->() { return &_v->_data; }
    operator T*() { return &_v->_data; }
    void unlock() { _ml.unlock(); }
    void relock() { _ml.relock(); }
  };
  /** Lock and keep locked until LockedData is destroyed, same pattern
   * than QMutexLocker (RAII) and QPointer (smart pointer) combined */
  LockedData lockedData() {
    return LockedData(this);
  }
};

#endif // ATOMICVALUE_H
