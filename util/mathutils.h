/* Copyright 2022-2024 Gregoire Barbier and others.
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
#include <QPartialOrdering>

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
   * QDateTime,QDate,QTime are converted to msecs since 1970.
   * @return true on success false on failure
   */
  static bool promoteToBestNumericType(QVariant *a, QVariant *b);
  /** same as QVariant::compare() but:
   *  - first try to promote numbers to best integer or decimal representation
   *  and to convert strings if they are valid representations of numbers
   *  (such as: "1" "8e-10" "0x20" "true"), to convert dates and
   *  datetimes to msecs since 1970, times to msecs since midnight, booleans to
   *  1 or 0. try to compare signed and unsigned whenever possible
   *  (provided the unsigned one is lower than signed long long positive max).
   *  - then if pretends_invalid_is_empty is false (the default) :
   *  if a or b cannot be converted to a number but both can be converted as
   *  strings, compare them as strings (for instance: QDateTime
   *  compared to an ISO timestamp in a QString)
   *  - or, if pretends_invalid_is_empty is true: compare the string
   *  representation of variants, whatever they are. including invalid objects
   *  that will be processed as if they were an empty string (so using
   *  pretends_invalid_is_empty == true, QVariant() is equal to an QString("")
   *  and to QDateTime())
   */
  static QPartialOrdering compareQVariantAsNumberOrString(
      QVariant a, QVariant b, bool pretends_invalid_is_empty = false);
  static QVariant addQVariantAsNumber(QVariant a, QVariant b);
  static QVariant subQVariantAsNumber(QVariant a, QVariant b);
  static QVariant mulQVariantAsNumber(QVariant a, QVariant b);
  static QVariant divQVariantAsNumber(QVariant a, QVariant b);
  static QVariant modQVariantAsNumber(QVariant a, QVariant b);
  static QVariant andQVariantAsNumber(QVariant a, QVariant b);
  static QVariant xorQVariantAsNumber(QVariant a, QVariant b);
  static QVariant orQVariantAsNumber(QVariant a, QVariant b);
  /** convert QVariant to QString */
  static bool convertToUtf16(QVariant *a);
};

#endif // MATHUTILS_H
