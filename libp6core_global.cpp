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
#include "libp6core_global.h"
#include <QtDebug>

QDebug operator<<(QDebug dbg, std::partial_ordering po) {
  if (po == std::partial_ordering::equivalent)
    return dbg << "equivalent";
  if (po == std::partial_ordering::less)
    return dbg << "less";
  if (po == std::partial_ordering::greater)
    return dbg << "greater";
  return dbg << "unordered";
}
