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

public:
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
  /** same as QVariant::compare() but:
   *  - first try to promote numbers to best arithmetic type
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
  static std::partial_ordering compareQVariantAsNumberOrString(
      QVariant a, QVariant b, bool pretends_invalid_is_empty = false);
  /** Same as QVariant::compare() but:
   *  - first check if types are strictly the sames, otherwise return unordered
   *    which mean that 2L and 4LL are unordered (with QVariant::compare() we
   *    would have 2L < 4LL)
   *  - NaN <=> NaN is equivalent, not unordered (with regular C++ standard,
   *    which QVariant::compare() follows, NaN != NaN and two NaNs are unordered
   *    even if they are both the same kind of NaN)
   *
   *  The intend is to use QVariant as a data holder where different types (like
   *  unsigned long and signed char) must be treated as different values and
   *  where NaN should be kept as is (as a kind of null double or whatever)
   */
  static inline std::partial_ordering compare_qvariant_as_data_holder(
      const QVariant &a, const QVariant &b) {
    auto ta = a.metaType(), tb = b.metaType();
    qDebug() << "***     compare" << a << b << (ta == tb)
             //<< std::isnan(a.value<double>()) << std::isnan(b.value<double>())
             << (QVariant::compare(a, b) == 0);
    if (ta != tb)
      return std::partial_ordering::unordered;
    if (ta.id() == QMetaType::Double && tb.id() == QMetaType::Double
        && std::isnan(a.value<double>()) && std::isnan(b.value<double>()))
        return std::partial_ordering::equivalent;
    return QVariant::compare(a, b);
  }
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
  /** Bitwise and integers after promoting them to best integral type. */
  static QVariant bitwiseAndQVariantAsIntegral(QVariant a, QVariant b);
  /** Bitwise xor integers after promoting them to best integral type. */
  static QVariant bitwiseXorQVariantAsIntegral(QVariant a, QVariant b);
  /** Bitwise or integers after promoting them to best integral type. */
  static QVariant bitwiseOrQVariantAsIntegral(QVariant a, QVariant b);
  /** convert QVariant to QString */
  static bool convertToUtf16(QVariant *a);
};

#endif // MATHUTILS_H
