/* Copyright 2017 Hallowyn, Gregoire Barbier and others.
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

#include <QtDebug>
#include "util/radixtree.h"
#include <functional>
#include <string>

int main(int, char **) {
  RadixTree<int> rt1 { { "bar", 2 }, { "bar", 3 }, { "foo", 1 }, { "baz", 4 } };
  qDebug().noquote() << rt1.toDebugString();
  RadixTree<int> rt2 { { "foo", 1 }, { "bar", 2 }, { "", 6 } };
  qDebug().noquote() << rt2.toDebugString();
  RadixTree<int> rt3 { { "foo", 1 }, { "bar", 2 }, { "f", 3 }, { "fo", 4 } };
  qDebug().noquote() << rt3.toDebugString();
  RadixTree<std::function<int(const char*)>> rt4 {
  { "strlen", ::strlen },
  { "strsize", ::strlen },
  { "strcount", ::strlen },
  { "strs", :: strlen },
  };
  qDebug().noquote() << rt4.toDebugString();
  return 0;
}
