/* Copyright 2015 Hallowyn and others.
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
#ifndef ATOMICVALUE_H
#define ATOMICVALUE_H

#include <QMutex>
#include <QMutexLocker>

/** Class that protect access to a value object with a mutex providing the same
 * kind of protection than e.g. a QAtomicInteger despite using a less scalable
 * mean (QMutex + copying the value object for every access).
 * Usable with cheap copyable objets that are not thread-safe themselves, like
 * 64 bits integers on 32 bits platforms or implicitly shared objects (e.g.
 * QString, QDateTime).
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
  T data() const {
      QMutexLocker ml(&_mutex);
      return _data;
  }
  /** Convenience operator for value() */
  T operator*() const { return this->data(); }
  /** Convenience operator for value() */
  operator T() const { return this->data(); }
  void setData(T other) {
      QMutexLocker ml(&_mutex);
      _data = other;
  }
  /** Convenience operator for setValue() */
  AtomicValue<T> &operator=(T other) { setData(other); return *this; }
  void setData(const AtomicValue<T> &other) {
      QMutexLocker ml(&_mutex);
      _data = other.data();
  }
  /** Convenience operator for setValue() */
  AtomicValue<T> &operator=(const AtomicValue<T> &other) {
      setData(other);
      return *this;
  }
};

#endif // ATOMICVALUE_H

