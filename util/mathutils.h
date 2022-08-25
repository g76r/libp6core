/* Copyright 2022 Gregoire Barbier and others.
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
#ifndef MATHUTILS_H
#define MATHUTILS_H

#include "libp6core_global.h"
#include <QVariant>

class LIBP6CORESHARED_EXPORT MathUtils {
  MathUtils() = delete;

public:
  static bool promoteToBestNumericType(QVariant *a);
  /** Try to convert to largest matching compatible numeric types.
   * If both are any signed integer types both become qlonglong
   * If both are any unsigned integer types both become qulonglong
   * If one is any floating point type both become double
   * String types are first converted to qlonglong if possible, or double if
   * possible, then above rules apply as if they were qlonglong or double.
   * @return true on success false on failure
   */
  static bool promoteToBestNumericType(QVariant *a, QVariant *b);
  /** same as QVariant::compare() but can compare string representations of
   *  numbers, and can compare signed and unsigned in most cases (provided
   *  the unsigned one is lower than signed long long positive max).
   */
  static QPartialOrdering compareQVariantAsNumber(QVariant a, QVariant b);
  static QVariant addQVariantAsNumber(QVariant a, QVariant b);
  static QVariant subQVariantAsNumber(QVariant a, QVariant b);
  static QVariant mulQVariantAsNumber(QVariant a, QVariant b);
  static QVariant divQVariantAsNumber(QVariant a, QVariant b);
  static QVariant modQVariantAsNumber(QVariant a, QVariant b);
};

#endif // MATHUTILS_H
