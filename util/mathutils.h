/* Copyright 2022-2025 Gregoire Barbier and others.
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
//#include <QPartialOrdering>
#include <QVariant>

class LIBP6CORESHARED_EXPORT MathUtils {
  MathUtils() = delete;

private:
  /** Try to convert to largest compatible arithmetic type:
   *  - double if floating type
   *  - otherwise qlonglong / std::int64_t if possible (small enough integer)
   *  - otherwise qulonglong / std::uint64_t if possible (small enough and > 0)
   *  String types (QString,QByteArray,Utf8String) are converted to numbers as
   *  above if possible, "true" to 1, "false" to 0.
   *  QDateTime,QDate,QTime are converted to msecs since 1970.
   *  @return true on success false on failure
   */
  static bool promoteToBestArithmeticType(QVariant *a);
  /** Try to convert to largest matching compatible numeric types.
   * If both are any signed integer types both become qlonglong
   * If both are any unsigned integer types both become qulonglong
   * If one is any floating point type both become double
   * String types are first converted to qlonglong if possible, or double if
   * possible, then above rules apply as if they were qlonglong or double.
   * QDateTime,QDate,QTime are converted to msecs since 1970.
   * @return true on success false on failure
   */
  static bool promoteToBestArithmeticType(QVariant *a, QVariant *b);
public:
  /** Add numbers after promoting them to best arithmetic type.
   *  For integers, use __builtin_{s,u}addll_overflow instead of + if supported
   *  by compiler */
  static QVariant addQVariantAsNumber(QVariant a, QVariant b);
  /** Substract numbers after promoting them to best arithmetic type.
   *  For integers, use __builtin_{s,u}subll_overflow instead of + if supported
   *  by compiler */
  static QVariant subQVariantAsNumber(QVariant a, QVariant b);
  /** Multiply numbers after promoting them to best arithmetic type.
   *  For integers, use __builtin_{s,u}mulll_overflow instead of + if supported
   *  by compiler */
  static QVariant mulQVariantAsNumber(QVariant a, QVariant b);
  /** Divide numbers after promoting them to best arithmetic type. */
  static QVariant divQVariantAsNumber(QVariant a, QVariant b);
  /** Take reminder of division after promoting numbers to best arithmetic
   *  type.
   *  For doubles, use std::fmod(). */
  static QVariant modQVariantAsNumber(QVariant a, QVariant b);
  /** Boolean and after best arithmethic type conversion then bool
   *  conversion. */
  static QVariant boolAndQVariantAsNumber(QVariant a, QVariant b);
  /** Boolean exclusive or after best arithmethic type conversion then bool
   *  conversion. */
  static QVariant boolXorQVariantAsNumber(QVariant a, QVariant b);
  /** Boolean inclusive or after best arithmethic type conversion then bool
   *  conversion. */
  static QVariant boolOrQVariantAsNumber(QVariant a, QVariant b);
private:
  /** convert QVariant to QString */
  static bool convertToUtf16(QVariant *a);
};

#endif // MATHUTILS_H
