/* Copyright 2016-2025 Hallowyn, Gregoire Barbier and others.
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
#include <algorithm>
#include <QHash>
#include <QMap>

namespace p6 {

/** Arrange items in a container to move dependencies before dependants.
 *  Dependencies are recursively searched.
 *  This is a stable sorting algorithm using a directed graph neighborhood
 *  (given by DependsOn predicate) as a partial order.
 *  The graph does not need to be connected (DependsOn may even always return
 *  false).
 *  If there are circular dependencies the sort will no longer be stable (some
 *  "forward" dependencies will be arbitrarily choosen and used to sort items)
 *  and obviously the result won't be a fully satisfying topological sort since
 *  no one exists.
 *  Time complexity: best case O(n²), worst case O(n³).
 *  Space complexity: O(n).
 *  @param depends_on(a,b) must return true iff a depends on b.
 *  @see https://en.wikipedia.org/wiki/Topological_sorting
 */
template<typename RandomAccessIterator, typename DependsOn>
inline void stable_topological_sort(
    RandomAccessIterator first, RandomAccessIterator last, DependsOn depends_on) {
  //using T = std::iterator_traits<RandomAccessIterator>::value_type;
  // recursive function, with cycle detection parameter
  std::function<void(RandomAccessIterator, RandomAccessIterator, DependsOn, bool)> do_sort
      = [&do_sort](RandomAccessIterator first, RandomAccessIterator last, DependsOn depends_on, bool high_branch) {
    if (first == last)
      return;
    auto current = first;
    auto next = std::next(current);
    if (next == last)
      return;
    //qDebug() << "stsrc:" << *first << *current << *std::prev(last);
    for (auto i = next; i != last; ++i) {
      if (depends_on(*current, *i)) {
        //qDebug() << "stsdo:" << *current << *i << *std::prev(i);
        if (high_branch) {
          for (auto j = std::prev(i); ; --j) {
            if (depends_on(*i, *j)) {
              // cycle detected, ignore i
              //qDebug() << "sts: cycle detected" << *i << *j;
              goto cycle_detected;
            }
            //qDebug() << "stsnc:" << *i << *j << depends_on(*i, *j) << *current << *i;
            if (j == first)
              break;
          }
        }
        // move *i before *current and shift everything else to the right
        //qDebug() << "stsbm:" << *current << *i;
        std::rotate(current, i, std::next(i));
        //qDebug() << "stsam:" << *current << *i;
        ++current;
        //qDebug() << "stsai:" << *current;
      }
cycle_detected:;
    }
    if (first != current) {
      //qDebug() << "stswr:" << *first << *std::prev(last);
      // sort direct dependencies, letting them recursively take other following
      // items excepted those which have already been sorted
      do_sort(first, last, depends_on, true);
      next = std::next(current);
    }
    if (!high_branch) {
      // sort remaining items
      //qDebug() << "stsrr:" << *next << *std::prev(last) << *first;
      do_sort(next, last, depends_on, false);
    }
  };
  do_sort(first, last, depends_on, false);
  // MAYDO provide a linked list implementation that rather uses splice() or
  // insert() instead of rotate()
  // it will be less generic (we need the container instead of 2 iterators)
  // and will invalid iterators on the container /!\ including our recursive
  // call iterators
  // but the move part of the sort will be O(1) instead of O(n) and won't do
  // any copy of the underlying data
  // the whole algorithm will stay O(n²) anyway, but with almost the same k for
  // best and worst case when the underlying data copy is expensive
  // see e.g. https://www.reddit.com/r/cpp_questions/comments/ecrbg4/stdrotate_on_lists_still_on/
}

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

/** Object hiding a QList behind a range loop expression.
 *  Usefull as an API return value waiting for C++23 and temporary
 *  std::ranges::view actually working (not UB, not crashing).
 *  Note that returning this object as a temporary for use as a range loop
 *  expression is still UB anyway in C++20, but it's supported by GCC.
 */
template<typename T>
class QListRange {
  QList<T> _list;
public:
  QListRange(QList<T> list) : _list(list) {}
  auto begin() const { return _list.constBegin(); }
  auto end() const { return _list.constEnd(); }
};

} // namespace p6

namespace ContainerUtils {

// backward compatibility with former ContainerUtils class used as a ns
using p6::reversed_hash;
using p6::reversed_map;
using p6::index;

} // namespace ContainerUtils

#endif // CONTAINERUTILS_H
