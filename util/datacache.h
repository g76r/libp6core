/* Copyright 2024 Gregoire Barbier and others.
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
#ifndef DATACACHE_H
#define DATACACHE_H

#include "libp6core_global.h"
#include <QCache>
#include <QMutexLocker>
#include <functional>

/** Non-thread-safe helper for QCache holding implicitly shared data objects.
 *  usage:
 *  DataCache<int,MyData> cache;
 *  auto data = cache.get_or_create(key, [&](){ return MyData(whatever); });
 *
 *  For thread-safe cache, use either MultiThreadDataCache or DataCache with
 *  thread_local storage class which is lock-free but takes more memory and/or
 *  more compilation time depending of the kind of cached data types.
 */
template <typename K, typename T>
class LIBP6CORESHARED_EXPORT DataCache {
  QCache<K,T> _cache;

public:
  inline DataCache(qsizetype maxCost = 100) : _cache(maxCost) {}
  T get_or_create(K key, std::function<T()> creator) {
    auto ptr = _cache[key];
    if (ptr) {
      //qDebug() << "cache HIT:" << key << _cache.size();
      return T(*ptr);
    }
    //qDebug() << "cache miss:" << key << _cache.size();
    T value = creator();
    _cache.insert(key, new T(value));
    return value;
  }
};

/** Thread-safe helper for QCache holding implicitly shared data objects,
 *  globally across all threads.
 *  Uses QMutex (hence it's not lock free).
 *  usage:
 *  MultiThreadDataCache<int,MyData> cache;
 *  auto data = cache.get_or_create(key, [&](){ return MyData(whatever); });
 */
template <typename K, typename T>
class LIBP6CORESHARED_EXPORT MultiThreadDataCache {
  QCache<K,T> _cache;
  QMutex _mutex;

public:
  inline MultiThreadDataCache(qsizetype maxCost = 100) : _cache(maxCost) {}
  T get_or_create(K key, std::function<T()> creator) {
    QMutexLocker locker(&_mutex);
    auto ptr = _cache[key];
    if (ptr) {
      //qDebug() << "cache HIT:" << key << _cache.size();
      return T(*ptr);
    }
    //qDebug() << "cache miss:" << key << _cache.size();
    locker.unlock();
    T value = creator();
    locker.relock();
    _cache.insert(key, new T(value));
    return value;
  }
};

#endif // DATACACHE_H
