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
#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include "libqtssu_global.h"

/** Circular buffer interface.
 * @see TwoThreadsCircularBuffer
 * @see ThreadSafeCircularBuffer
 */
template <class T>
class LIBQTSSUSHARED_EXPORT CircularBuffer {
public:
  virtual ~CircularBuffer() { }
  virtual void put(T data) = 0;
  virtual T get() = 0;
  virtual bool tryPut(T data) = 0;
  /** @return T() if there is no available data */
  virtual T tryGet() = 0;
  // TODO virtual bool tryGet(T&) = 0;
  virtual long size() const = 0;
  virtual long free() const = 0;
  virtual long used() const = 0;
  // MAYDO add method for puting or geting several data items at a time
};

#endif // CIRCULARBUFFER_H
