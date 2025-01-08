/* Copyright 2025 Gregoire Barbier and others.
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
#include "util/containerutils.h"
#include <list>
#include <set>
#include <QtDebug>
#include "util/utf8string.h"

inline QDebug operator<<(QDebug dbg, const std::list<int> &list) {
  Utf8String s = "{ ";
  for (auto i: list)
    s = s + Utf8String::number(i,16) + ' ';
  s += '}';
  return dbg.noquote() << s;
}

int main(void) {
  std::list<int> list1 = { 1, 2, 3, 4, 5, 6};
  std::set<std::pair<int,int>> deps1 = {{1,2},/*{1,4},*/{2,3},{5,6},{3,2}};
  qDebug() << list1; // { 1 2 3 4 5 6 }
  p6::stable_topological_sort(list1.begin(), list1.end(), [deps=deps1](int a, int b) {
    return deps.contains({a, b});
  });
  qDebug() << list1; // { 2 1 3 4 6 5 }
  list1 = { 1, 2, 3, 4, 5, 6};
  deps1 = {{2,3},{5,6},{3,2}};
  qDebug() << list1; // { 1 2 3 4 5 6 }
  p6::stable_topological_sort(list1.begin(), list1.end(), [deps=deps1](int a, int b) {
    return deps.contains({a, b});
  });
  qDebug() << list1; // { 1 2 3 4 6 5 }
  std::list<int> list2 = { 0x2001,0x2006,0x2004,0x2003,0x2002,0x2005,0x2000 };
  std::set<std::pair<int,int>> deps2 = {{0x2003,0x2000},{0x2002,0x2000},{0x2004,0x2000},{0x2001,0x2000},{0x2005,0x2001},{0x2006,0x2001}};
  qDebug() << list2; // { 2001 2006 2004 2003 2002 2005 2000 }
  p6::stable_topological_sort(list2.begin(), list2.end(), [deps=deps2](int a, int b) {
    return deps.contains({a, b});
  });
  qDebug() << list2; // { 2000 2001 2006 2004 2003 2002 2005  }
  std::list<int> list3 = { 0x2005,0x2001,0x2006,0x2004,0x2003,0x2002,0x2000 };
  qDebug() << list3; // { 2005 2001 2006 2004 2003 2002 2000 }
  p6::stable_topological_sort(list3.begin(), list3.end(), [deps=deps2](int a, int b) {
    return deps.contains({a, b});
  });
  qDebug() << list3; // { 2000 2001 2005 2006 2004 2003 2002 }
  list3 = {0x2000,0x2006,0x2007,0x2003,0x2005,0x2004,0x2001,0x2002};
  deps2 = {{0x2003,0x2000},{0x2002,0x2000},{0x2004,0x2000},{0x2001,0x2000},{0x2005,0x2000},{0x2006,0x2005},{0x2007,0x2005}};
  qDebug() << list3;
  p6::stable_topological_sort(list3.begin(), list3.end(), [deps=deps2](int a, int b) {
    return deps.contains({a, b});
  });
  qDebug() << list3;
}
