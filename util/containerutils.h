/* Copyright 2016-2023 Hallowyn, Gregoire Barbier and others.
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
class LIBP6CORESHARED_EXPORT ContainerUtils {
  ContainerUtils() = delete;

public:
  /** Build inverse mapping QHash.
   * If original container had duplicate values, only one of its keys will be
   * associated as value in the reverted container. No mean to know which one.
   */
  template<class K,class T>
  static QHash<K,T> reversed_hash(QHash<T,K> source) {
    QHash<K,T> dest;
    for (auto [k,v]: source.asKeyValueRange())
      dest.insert(v, k);
    return dest;
  }

  /** Build inverse mapping QMap.
   * If original container had duplicate values, only one of its keys will be
   * associated as value in the reverted container. No mean to know which one.
   */
  template<class K,class T>
  static QMap<K,T> reversed_map(QHash<T,K> source) {
    QMap<K,T> dest;
    for (auto [k,v]: source.asKeyValueRange())
      dest.insert(v, k);
    return dest;
  }

  /** Build inverse mapping QMap.
   * If original container had duplicate values, only one of its keys will be
   * associated as value in the reverted container. The last one in the original
   * keys order.
   */
  template<class K,class T>
  static QMap<K,T> reversed_map(QMap<T,K> source) {
    QMap<K,T> dest;
    for (auto [k,v]: source.asKeyValueRange())
      dest.insert(v, k);
    return dest;
  }

  /** Build inverse mapping QHash.
   * If original container had duplicate values, only one of its keys will be
   * associated as value in the reverted container. The last one in the original
   * keys order.
   */
  template<class K,class T>
  static QHash<K,T> reversed_hash(QMap<T,K> source) {
    QHash<K,T> dest;
    for (auto [k,v]: source.asKeyValueRange())
      dest.insert(v, k);
    return dest;
  }

  /** Build index of QList, i.e. maps every item to its index in the list.
   *  If there are duplicated items, the last index will prevail.
   */
  template<class T>
  static QMap<T,int> index(QList<T> source) {
    QMap<T,int> dest;
    qsizetype n = source.size();
    for (qsizetype i = 0; i < n; ++i)
      dest.insert(source[i], i);
    return dest;
  }
};

//template<typename T>
//class kv_range {
//  T data;
//public:
//  kv_range(T data) : data{data} { }
//  //kv_range(T &&data) : data{data} { }
//  auto begin() { return data.keyValueBegin(); }
//  auto end() { return data.keyValueEnd(); }
//};

#endif // CONTAINERUTILS_H
