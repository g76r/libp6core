/* Copyright 2023 Gregoire Barbier and others.
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

#include "utf8string.h"

template<typename C,typename T>
static inline Utf8String join(const C &container, const T &separator) {
  Utf8String joined;
  bool first = true;
  for (auto s: container) {
    if (first)
      first = false;
    else
      joined += separator;
    joined += s;
  }
  return joined;
}

Utf8String Utf8StringList::join(const Utf8String &separator) {
  return ::join(*this, separator);
}

Utf8String Utf8StringList::join(const char separator) {
  return ::join(*this, separator);
}

Utf8String Utf8StringSet::join(const Utf8String &separator) {
  return ::join(*this, separator);
}

Utf8String Utf8StringSet::join(const char separator) {
  return ::join(*this, separator);
}
