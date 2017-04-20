/* Copyright 2016-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef CONTAINERUTILS_H
#define CONTAINERUTILS_H

#include "libp6core_global.h"
#include <QHash>
#include <QMap>

/** Container utilities.
 */
class LIBPUMPKINSHARED_EXPORT ContainerUtils {
  ContainerUtils() = delete;

public:
  /** Build inverse mapping QHash.
   * If original container had duplicate values, only one of its keys will be
   * associated as value in the reverted container. No mean to know which one.
   */
  template<class K,class T>
  static QHash<K,T> reversed(QHash<T,K> source) {
    QHash<K,T> dest;
    foreach (const T &key, source.keys()) {
      dest.insert(source.value(key), key);
    }
    return dest;
  }

  /** Build inverse mapping QHash.
   * If original container had duplicate values, only one of its keys will be
   * associated as value in the reverted container. The last one in the original
   * keys order.
   */
  template<class K,class T>
  static QMap<K,T> reversed(QMap<T,K> source) {
    QMap<K,T> dest;
    foreach (const T &key, source.keys()) {
      dest.insert(source.value(key), key);
    }
    return dest;
  }
};


#endif // CONTAINERUTILS_H
