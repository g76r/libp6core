/* Copyright 2024 Hallowyn, Gregoire Barbier and others.
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
#ifndef OBJECTUTILS_H
#define OBJECTUTILS_H

#include "libp6core_global.h"
#include <QObject>
#include <functional>

/** QObject utilities. */
namespace object_utils {

/** Apply method to every child */
inline void foreach_child(
    const QObject *object, std::function<void(QObject*)> f) {
  for (auto c: object->children())
    f(c);
}

/** Apply method to every descendant */
inline void foreach_descendant(
    const QObject *object, std::function<void(QObject*)> f) {
  for (auto c: object->children()) {
    f(c);
    foreach_descendant(c, f);
  }
}

} // namespace

#endif // OBJECTUTILS_H


