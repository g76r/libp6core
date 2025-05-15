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
#ifndef NUMBERUTILS_H
#define NUMBERUTILS_H

#include "libp6core_global.h"
#include <cfloat>

namespace p6 {

/** mixed sign integer comparison */
[[nodiscard]] static inline std::strong_ordering compare(uint64_t a, int64_t b) {
  if (b < 0)
    return std::strong_ordering::greater;
  return a <=> (uint64_t)b;
}

/** mixed sign integer comparison */
[[nodiscard]] static inline std::strong_ordering compare(int64_t a, uint64_t b) {
  if (a < 0)
    return std::strong_ordering::less;
  return (uint64_t)a <=> b;
}

/** tells if a double fits in a given integral type.
 *
 *  both the integer and double bit counts are tested, i.e. a double >= 2**53
 *  can't be converted to a 64 bit integer because it's not an integer since
 *  it's mantissa is only 53 bits (assuming double is IEEE754 double precision).
 *
 *  double_fits_in_integral_type<int>(42.0) -> true
 *  double_fits_in_integral_type<short>(66e3) -> false
 *  double_fits_in_integral_type<int64_t>(66e3) -> true
 *  double_fits_in_integral_type<int64_t>(9e15) -> true
 *  double_fits_in_integral_type<int64_t>(1e16) -> false (more than 53 bits)
 */
template<typename I>
[[nodiscard]] static inline bool double_fits_in_integral_type(double d) {
  if constexpr (std::numeric_limits<I>::digits >= DBL_MANT_DIG) {
    // integral types with more digits than double's mantissa:
    // use double value only if it's within DBL_MANT_DIG bits integer range
    // (if double is IEE754 double precision, DBL_MANT_DIG == 53)
    // in other words we now every digits of 9e15 but 1e16 lacks some on the
    // right
    if constexpr (std::numeric_limits<I>::is_signed)
      return d >= -(1LL<<DBL_MANT_DIG) && d <= (1LL<<DBL_MANT_DIG);
    return d >= 0 && d <= (1LL<<DBL_MANT_DIG);
  }
  // integral types with less digits than double's mantissa:
  // use double value only if it fits in the integer type
  return d >= std::numeric_limits<I>::min() &&
      d <= std::numeric_limits<I>::max();
}

/** tells if an integral type value fits in the double mantissa without loosing
 *  precision.
 */
template<typename I>
[[nodiscard]] static inline bool integral_type_fits_in_double(I i) {
  // inegral types with less digits than double's mantissa always fit
  if constexpr (std::numeric_limits<I>::digits < DBL_MANT_DIG)
    return true;
  // otherwise it must not excess the mantissa bits number
  // (if double is IEE754 double precision, DBL_MANT_DIG == 53)
  if constexpr (std::numeric_limits<I>::is_signed)
    return i >= -(1LL<<DBL_MANT_DIG) && i <= (1LL<<DBL_MANT_DIG);
  return i >= 0 && i <= (1LL<<DBL_MANT_DIG);
}


} // p6 namespace

#endif // NUMBERUTILS_H
