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

/** Arrange items in [first,end) to move dependencies before their dependants.
 *
 *  This is a stable sort algorithm using a directed graph neighborhood
 *  (given by DependsOn predicate) as a partial order.
 *  The graph does not need to be connected (DependsOn may even always return
 *  false).
 *
 *  Dependencies are recursively searched.
 *  If there are circular dependencies the sort will no longer be stable (some
 *  dependencies will be arbitrarily choosen and used to sort items) and
 *  obviously the result won't be a fully satisfying topological sort since no
 *  one exists.
 *
 *  Time complexity: best case O(n²), worst case O(n³).
 *  Space complexity: O(n).
 *
 *  Parameters assume_acyclic and assume_injective provides some optimizations.
 *  If it is known that there are no cyclic dependencies (the dependency graph
 *  is a DAG), assume_acyclic can be set to true. However it can leads to
 *  infinite recursion if there are unexpected cycles.
 *  If it is known that no item has more than 1 dependency, assume_injective can
 *  be set to true. However it can lead to not fully sorted sets if an item has
 *  unexpected additionnal dependencies.
 *  If it is known that the dependency graph is a tree, both flag can be set to
 *  true.
 *
 *  @param first iterator on first item
 *  @param end sentinel (upper bound) beyond last item
 *  @param depends_on(a,b) must return true iff a depends on b.
 *  @param cyclic_dependency_found target is set if != null and !assume_acyclic
 *  @param assume_acyclic don't look for cyclic dependencies
 *  @param assume_injective don't look for more than 1 dependency per item
 *  @see https://en.wikipedia.org/wiki/Topological_sorting
 */
#ifdef __cpp_concepts
template<std::forward_iterator Iterator,
         std::indirect_binary_predicate<Iterator,Iterator> DependsOn>
#else
template<typename Iterator, typename DependsOn>
#endif
inline void stable_topological_sort(
    const Iterator first, const Iterator end, const DependsOn depends_on,
    bool *cyclic_dependency_found = 0, const bool assume_acyclic = false,
    const bool assume_injective = false) {
  /** The algorithm searches for direct dependencies (A->B) in reverse order as
   *  compared to current container order, that is, items in (first,end) on
   *  which *first depends on, and reorders items according to this.
   *
   *  There are 2 iteration branches types, called high branch and low branch:
   *
   *  Both branches search for items on which first item depends on, to move
   *  it "before" first (actually to move its value on first element and shift
   *  every other values to the right).
   *  An item directly depends on another when depends_on(*item, *another) is
   *  true. Indirect dependencies are searched by the algorithm (depends_on
   *  function returns false if the dependency is indirect).
   *  When a move occurs both branches create a high branch iteration (which is
   *  implemented as a recursive call).
   *
   *  Low branch:
   *  After searching an item to move, the low branch iterates (which is also
   *  implemented as a recursive call) on [next,end), with next being the next
   *  item after first (if the set was already sorted for this iteration) or
   *  after last moved item.
   *  It's the main branch and even the only one if the container is already
   *  toatally sorted at the beginning (best case).
   *
   *  High branch:
   *  As seen above, high branch is only called when an item has been moved.
   *  After having moved an item, a high branch iteration is created on
   *  [current0,end), with *current having been replaced with the moved item
   *  value, although low branch iterations are created on [next,end).
   *  This is the way we look for indirect dependencies (A->B->C) because
   *  indirect dependecies are dependencies of [current0,next) items
   *  which are the B's of A->B->C.
   *  Before moving the item, it searches for cycles and will neither move the
   *  item (stable sort) nor do the recursive call if a cycle is found (avoiding
   *  infinite recursion).
   *  Cycles are detected by looking for dependencies from already moved items
   *  in [current0,after_last_moved) to remaining items in (current,end).
   *  Note: `current0` is the current item iterator as it is at the begining of
   *  a given iteration, whereas `current` is incremented everytime an item is
   *  moved before it.
   *  High branch iterations don't create low branch iterations.
   *
   *  Example:
   *  initial (low branch) iteration [0,9)     {0,1,2,3,4,5,6,7,8}
   *   | moving 2 because 0 depends on 2       {2,0,1,3,4,5,6,7,8}
   *   +--high branch iteration [2,9)
   *   |  \ nothing depends on 2
   *   | we know now that nothing else depends on 2 or 0
   *   | low branch iteration [1,9)                {1,3,4,5,6,7,8}
   *   | moving 4 and 5 b/c 1 depends on them      {4,5,1,3,6,7,8}
   *   +--high branch iteration [4,9)
   *      \ moving 5 and 3 b/c 4 depends on them   {5,3,4,1,6,7,8}
   *       \ high branch iteration [5,9)
   *        \ nothing depends on 5
   *   | we know now that nothing else depends on 5..1
   *   | low branch iteration [6,9)                        {6,7,8}
   *   | nothing depends on 6
   *   | low branch iteration [7,9)                          {7,8}
   *   | nothing depends on 7
   *   | low branch iteration [8,9)                            {8}
   *   | nothing can depend on set with less than 2 items
   *   .
   *  final sorted state:                      {2,0,5,3,4,1,6,7,8}
   *
   *  @param current begin iterator for iteration set [current,end)
   *  @param after_last_moved interator on element after last moved item
   *  @param depends_on depends_on(a,b) == true iff *a depends on *b
   *  @param in_high_branch true iff current iteration is part of a high branch
   *  @return count of items that can be skipped in next iterations b/c their
   *    dependencies have already been searched for by recursive calls
   */
  std::function<size_t(Iterator, Iterator, const bool)> do_sort
      = [&do_sort,&first,&end,&depends_on,cyclic_dependency_found,assume_acyclic,assume_injective](
        Iterator current, Iterator after_last_moved, const bool in_high_branch) -> size_t {
    const auto current0 = current; // current at begining of this iteration
    if (current == end) [[unlikely]]
      return 0; // empty set is already sorted
    auto next = std::next(current); // will be current in next low branch iteration
    if (next == end) [[unlikely]]
      return 0; // 1 item set is already sorted
    size_t skippable_items_count = 0; // counting how many times we ++after_last_moved
    auto after_first_moved = current; // first interval value for cycles detection
    bool i_is_beyond_last_moved = (after_last_moved == current);
    // search for current dependencies within i items beyond current
    for (auto i = next; i != end; ++i) [[likely]] {
      if (i == after_last_moved) [[unlikely]]
        i_is_beyond_last_moved = true;
      if (depends_on(*current, *i) && i != current) [[unlikely]] {
        if (!assume_acyclic && in_high_branch) {
          // search for circular dependencies: j items on which i depends
          // with j in [after_first_moved,after_last_moved)
          for (auto j = after_first_moved; j != after_last_moved; ++j) {
            if (depends_on(*i, *j)) [[unlikely]] {
              // cycle detected, ignore i
              if (!assume_acyclic && cyclic_dependency_found)
                *cyclic_dependency_found = true;
              goto cycle_detected;
            }
          }
        }
        // *i = *current and shift [current,i) values to the right
        std::rotate(current, i, std::next(i));
        if (i_is_beyond_last_moved) {
          ++after_last_moved;
          ++after_first_moved;
          ++skippable_items_count;
        }
        ++current;
        if (assume_injective)
          break;
      }
cycle_detected:;
    }
    if (current != current0) {
      // one or more items where moved, create high branch iteration
      auto recursive_skippable_items_count =
          do_sort(current0, after_last_moved, true);
      skippable_items_count += recursive_skippable_items_count;
      // skip items that have already been moved because their dependencies have
      // already been looked for in this or previous low or high iterations
      for (auto i = skippable_items_count; i > 0; --i)
        ++next;
    }
    if (!in_high_branch) {
      // sort remaining items by carrying on low branch recursion
      do_sort(next, next, false);
    }
    return skippable_items_count;
  };
  do_sort(first, first, false);
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

template<typename C, typename T>
concept readable_set = requires(const C &c, const T &t) {
  { c.contains(t) } -> std::same_as<bool>;
};

template<typename T>
class single_set {
  T t;
public:
  inline single_set(const T&t) : t(t) {};
  inline single_set(const T&&t) : t(std::move(t)) {};
  inline bool contains(const T&t) const { return t == this->t; }
};

} // namespace p6

namespace ContainerUtils {

// backward compatibility with former ContainerUtils class used as a ns
using p6::reversed_hash;
using p6::reversed_map;
using p6::index;

} // namespace ContainerUtils

#endif // CONTAINERUTILS_H
