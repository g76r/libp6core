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
#include "typedvalue.h"
#include "eg/entity.h"
#include "log/log.h"
#include "util/containerutils.h"
#include "util/utf8utils.h"
#include <QPointF>
#include <QDateTime>
#include <QVariant>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <QLineF>
#include <QRegularExpression>

namespace p6 {

using Type = TypedValue::Type;

const int TypedValue::_entity8vector_mtid = qMetaTypeId<QList<Entity>>();
const int TypedValue::_fvector_mtid = qMetaTypeId<QList<double>>();
const int TypedValue::_pointfvector_mtid = qMetaTypeId<QList<QPointF>>();

// Value base class ///////////////////////////////////////////////////////////

const std::vector<Entity> TypedValue::Value::_empty_entityvector;
const std::vector<double> TypedValue::Value::_empty_fvector;
const std::vector<QPointF> TypedValue::Value::_empty_pointfvector;
const TypedValue::NullValue TypedValue::NullValue::_nullvalue;

TypedValue::Value::~Value() {
}

Type TypedValue::Value::type() const {
  return Null;
}

bool TypedValue::Value::operator!() const {
  return true;
}

std::partial_ordering TypedValue::Value::operator<=>(const Value &) const {
  return std::partial_ordering::unordered;
}

bool TypedValue::Value::operator==(const Value &) const {
  return false;
}

uint64_t TypedValue::Value::unsigned8() const {
  return 0;
}

int64_t TypedValue::Value::signed8() const {
  return 0;
}

double TypedValue::Value::float8() const {
  return std::numeric_limits<double>::quiet_NaN();
}

bool TypedValue::Value::bool1() const {
  return false;
}

Utf8String TypedValue::Value::utf8() const {
  return {};
}

QByteArray TypedValue::Value::bytes() const {
  return {};
}

Entity TypedValue::Value::entity8() const {
  return {};
}

const std::vector<Entity> &TypedValue::Value::entityvector() const {
  return _empty_entityvector;
}

const std::vector<double> &TypedValue::Value::fvector() const {
  return _empty_fvector;
}

const std::vector<QPointF> &TypedValue::Value::pointfvector() const {
  return _empty_pointfvector;
}

QPointF TypedValue::Value::pointf() const {
  return {};
}

QSizeF TypedValue::Value::sizef() const {
  return {};
}

QRectF TypedValue::Value::rectf() const {
  return {};
}

QLineF TypedValue::Value::linef() const {
  return {};
}

QDateTime TypedValue::Value::timestamp8() const {
  return {};
}

QRegularExpression TypedValue::Value::regexp() const {
  return {};
}

QVariant TypedValue::Value::qvariant() const {
  return {};
}

uint64_t TypedValue::Value::as_unsigned8(uint64_t def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

int64_t TypedValue::Value::as_signed8(int64_t def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

double TypedValue::Value::as_float8(double def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

bool TypedValue::Value::as_bool1(bool def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

Utf8String TypedValue::Value::as_utf8(const Utf8String &def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

QDateTime TypedValue::Value::as_timestamp8(const QDateTime &def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

QRegularExpression TypedValue::Value::as_regexp(const QRegularExpression &def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

const std::vector<Entity> TypedValue::Value::as_entityvector(const std::vector<Entity> &def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

const std::vector<double> TypedValue::Value::as_fvector(const std::vector<double> &def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

const std::vector<QPointF> TypedValue::Value::as_pointfvector(const std::vector<QPointF> &def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

QPointF TypedValue::Value::as_pointf(const QPointF &def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

QSizeF TypedValue::Value::as_sizef(const QSizeF &def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

QRectF TypedValue::Value::as_rectf(const QRectF &def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

QLineF TypedValue::Value::as_linef(const QLineF &def, bool *ok) const {
  if (ok) *ok = false;
  return def;
}

Type TypedValue::NullValue::type() const {
  return Null;
}

// arithmetic and comparison operators ////////////////////////////////////////

std::partial_ordering TypedValue::compare_assuming_one_float_or_both_integral(
    const TypedValue &a, const TypedValue &b, bool pretend_null_or_nan_is_empty,
    const Type ta, const Type tb) {
  Q_ASSERT((is_integral(ta) && is_integral(tb)) || ta == Float8 || tb == Float8);
  // C++23: maybe replace __attribute__ with front attribute [[gnu::always_inline]] (between capture list and function args)
  auto float8_cmp = [pretend_null_or_nan_is_empty](double fa, double fb) __attribute__((always_inline)) -> std::partial_ordering {
    if (pretend_null_or_nan_is_empty) {
      auto na = std::isnan(fa), nb = std::isnan(fb);
      if (na && nb)
        return std::partial_ordering::equivalent;
      if (na || nb)
        return std::partial_ordering::unordered;
    }
    return fa <=> fb;
  };
  // try float8 first because we don't want to loose the fractionnal part by
  // wrongly converting a float8 to an integral type
  if (ta == Float8) {
    if (tb == Float8)
      return float8_cmp(a.direct_float8(), b.direct_float8());
    // float promotion for integral types and conversion for others (incl. utf8)
    bool ok;
    auto fb = b.as_float8(&ok);
    if (ok)
      return float8_cmp(a.direct_float8(), fb);
    if (pretend_null_or_nan_is_empty && std::isnan(a.direct_float8()))
      return ""_u8 <=> b.as_utf8();
    return std::partial_ordering::unordered;
  }
  if (tb == Float8) {
    // float promotion for integral types and conversion for others (incl. utf8)
    bool ok;
    auto fa = a.as_float8(&ok);
    if (ok)
      return float8_cmp(fa, b.direct_float8());
    if (pretend_null_or_nan_is_empty && std::isnan(b.direct_float8()))
      return a.as_utf8() <=> ""_u8;
    return std::partial_ordering::unordered;
  }
  // at this point both must be integral, since the function assumes being
  // called with either at less one float operand or both integral ones
  // handle mixed sign comparison
  // process bool1 as if it was unsigned8, like in C++: 42==true is false and
  // 1==true is true
  if (ta == Signed8) {
    if (tb == Signed8)
      return a.direct_signed8() <=> b.direct_signed8();
    if (a.direct_signed8() < 0)
      return std::partial_ordering::less;
  } else if (tb == Signed8) {
    if (b.direct_signed8() < 0)
      return std::partial_ordering::greater;
  }
  return a.direct_unsigned8() <=> b.direct_unsigned8();
}

std::partial_ordering TypedValue::compare_as_number_otherwise_string(
    const TypedValue &a, const TypedValue &b, bool pretend_null_or_nan_is_empty) {
  auto ta = a.type(), tb = b.type();
  auto aa = is_arithmetic(ta), ab = is_arithmetic(tb);
  //qDebug() << "comparing" << a << b << ta << tb << aa << ab;
  // short path when no conversion is needed
  if ((aa && ab) || ta == Float8 || tb == Float8)
    return compare_assuming_one_float_or_both_integral(
          a, b, pretend_null_or_nan_is_empty, ta, tb);
  // at this point at less one operand is not an arithmetic number, but maybe
  // one (or both) can be casted to from non arithmetic type (incl. utf8)
  Utf8String sa, sb;
  if (!aa) {
    // a needs conversion
    sa = a.as_utf8();
    auto ca = best_number_type(sa);
    ta = ca.type();
    if (ta != Null) {
      if (ab)
        return compare_assuming_one_float_or_both_integral(
              ca, b, pretend_null_or_nan_is_empty, ta, tb);
      // b also needs conversion
      sb = b.as_utf8();
      auto cb = best_number_type(sb);
      tb = cb.type();
      if (tb != Null)
        return compare_assuming_one_float_or_both_integral(
              ca, cb, pretend_null_or_nan_is_empty, ta, tb);
      // at this point b cannot be converted to an arithmetic type, but a was
      // if ta is nan, must process as if it were null
      if (ta == Float8 && std::isnan(ca.direct_float8())) {
        // qDebug() << "a is converted to nan but not b" << ca << b
        //          << (""_u8 <=> sb);
        if (pretend_null_or_nan_is_empty)
          return ""_u8 <=> sb;
        return std::partial_ordering::unordered;
      }
      goto b_converted_to_utf8_but_cannot_be_converted_to_number;
    } else {
      // at this point a cannot be converted to an arithmetic type, but b is one
      // if ba is nan, must process as if it were null
      if (tb == Float8 && std::isnan(b.direct_float8())) {
        // qDebug() << "b is nan" << a << b
        //          << (sa <=> ""_u8);
        if (pretend_null_or_nan_is_empty)
          return sa <=> ""_u8;
        return std::partial_ordering::unordered;
      }
      // note: we can't rely on a further string comparison, because
      // TypedValue(NAN).as_utf8() returns "nan", not ""
    }
    // at this point a cannot be converted to an arithmetic type
  }
  if (!ab) {
    // b needs conversion
    sb = b.as_utf8(); // no duplicate conversion b/c skipped by goto
    auto cb = best_number_type(sb);
    tb = cb.type();
    if (tb != Null)
      return compare_assuming_one_float_or_both_integral(
            a, cb, pretend_null_or_nan_is_empty, ta, tb);
    // at this point b cannot be converted to an arithmetic type
    // if ta is nan, must process as if it were null
    if (ta == Float8 && std::isnan(a.direct_float8())) {
      // qDebug() << "a is nan" << a << b
      //          << (""_u8 <=> b.as_utf8());
      if (pretend_null_or_nan_is_empty)
        return ""_u8 <=> b.as_utf8();
      return std::partial_ordering::unordered;
    }
  }
  // at this point at less one operand can't be converted to an arithmetic type
  // and none is nan, only string comparison is still possible
  if (aa)
    sa = a.as_utf8();
  if (ab)
    sb = b.as_utf8();
b_converted_to_utf8_but_cannot_be_converted_to_number:
  //qDebug() << "no conversion matched" << a << b << sa << sb;
  if (!pretend_null_or_nan_is_empty && (!sa || !sb))
    return std::partial_ordering::unordered;
  return sa <=> sb; // Utf8String::operator<=>() processes null as empty
}

TypedValue TypedValue::best_number_type(
    const Utf8String &utf8,
    bool use_integral_types_despite_floating_notation_if_possible) {
  auto s = utf8.constData(), begin = s, end = s+utf8.length();
  bool ok;
  if (!s)
    return {};
  bool ignore_e = false;
  for (; s < end; ++s)
    switch (*s) {
      case 't':
      case 's':
        // t and s are allowed only in "true" or "false" booleans
        // and they are before the 'e' which avoids mistaking it for a float
        // note: can't use f because of femto suffix and inf litteral, can't
        // use a because of nan litteral
        if (strcmp(begin, "true") == 0)
          return true;
        if (strcmp(begin, "false") == 0)
          return false;
        return {};
      case 'n':
      case 'N':
        // check for nan or inf or -inf, case insensitive (may also be nano)
        if (s == begin && end == begin+3 && (s[1] == 'a' || s[1] == 'A')
            && (s[2] == 'n' || s[2] == 'N'))
          return std::numeric_limits<double>::quiet_NaN();
        if (s == begin+1 && end == begin+3 && (s[-1] == 'i' || s[-1] == 'I')
            && (s[1] == 'f' || s[1] == 'F'))
          return std::numeric_limits<double>::infinity();
        if (s == begin+2 && end == begin+4 && s[-2] == '-' &&
            (s[-1] == 'i' || s[-1] == 'I') && (s[1] == 'f' || s[1] == 'F'))
          return -std::numeric_limits<double>::infinity();
        break;
      case 'x':
        ignore_e = true;
        break;
      case 'e':
      case 'E':
        if (ignore_e)
          break;
        [[fallthrough]];
      case '.': {
          // . and e are not allowed in an integer (excepted hexadecimal), so it
          // must be a float
          auto d = utf8.toDouble(&ok);
          if (!ok)
            return {};
          if (use_integral_types_despite_floating_notation_if_possible) {
            double i;
            std::modf(d, &i);
            if (d != i)
              return d;
            if (double_fits_in_integral_type<uint64_t>(d))
              return (uint64_t)d;
            if (double_fits_in_integral_type<int64_t>(d))
              return (int64_t)d;
          }
          return d;
        }
    }
  if (*begin == '-') {
    auto i = utf8.toLongLong(&ok);
    return ok ? i : TypedValue{};
  }
  auto u = utf8.toULongLong(&ok);
  return ok ? u : TypedValue{};
}


TypedValue TypedValue::arithmetic_binary_operation_assuming_one_float_or_both_integral(
    const TypedValue &a, const TypedValue &b,
    ArithmeticBinaryOperator<double> float8_op,
    ArithmeticBinaryOperator<uint64_t> unsigned8_op,
    ArithmeticBinaryOperator<int64_t> signed8_op,
    ArithmeticBinaryOperator<bool> bool1_op, Type ta, Type tb) {
  Q_ASSERT((is_integral(ta) && is_integral(tb)) || ta == Float8 || tb == Float8);
  // try float8 first because we don't want to loose the fractionnal part by
  // wrongly converting a float8 to an integral type
  if (ta == Float8) {
    if (tb == Float8)
      return float8_op(a.direct_float8(), b.direct_float8());
    // float promotion for integral types and conversion for others (incl. utf8)
    bool ok;
    auto fb = b.as_float8(&ok);
    if (!ok)
      return {};
    return float8_op(a.direct_float8(), fb);
  }
  if (tb == Float8) {
    // float promotion for integral types and conversion for others (incl. utf8)
    bool ok;
    auto fa = a.as_float8(&ok);
    if (!ok)
      return {};
    return float8_op(fa, b.direct_float8());
  }
  // at this point both must be integral, since the function assumes being
  // called with either at less one float operand or both integral ones
  // handle bool1 apart from unsigned8 only when both operands are booleans
  if (ta == Bool1 && tb == Bool1) // at less + and - are not same as Unsigned8
    return bool1_op(a.direct_unsigned8(), b.direct_unsigned8());
  // handle mixed sign operation when possible
  if (ta == Signed8) {
    if (tb == Signed8)
      return signed8_op(a.direct_signed8(), b.direct_signed8());
    if (a.direct_signed8() < 0
        && b.direct_unsigned8() > std::numeric_limits<int64_t>::max())
      return {}; // operation between negative signed and large unsigned
  } else if (tb == Signed8) {
    if (b.direct_signed8() < 0
        && a.direct_unsigned8() > std::numeric_limits<int64_t>::max())
      return {}; // operation between negative signed and large unsigned
  }
  // process operation as unsigned, being it a cast from bool or positive signed
  // or true natural unsigned operation
  return unsigned8_op(a.direct_unsigned8(), b.direct_unsigned8());
}

TypedValue TypedValue::arithmetic_binary_operation_with_best_type(
    const TypedValue &a, const TypedValue &b,
    ArithmeticBinaryOperator<double> float8_op,
    ArithmeticBinaryOperator<uint64_t> unsigned8_op,
    ArithmeticBinaryOperator<int64_t> signed8_op,
    ArithmeticBinaryOperator<bool> bool1_op) {
  auto ta = a.type(), tb = b.type();
  auto aa = is_arithmetic(ta), ab = is_arithmetic(tb);
  // short path when no conversion is needed (not including float promotion)
  if ((aa && ab) || ta == Float8 || tb == Float8)
    return arithmetic_binary_operation_assuming_one_float_or_both_integral(
          a, b, float8_op, unsigned8_op, signed8_op, bool1_op, ta, tb);
  // at this point either ta or tb or both are not an arithmetic type
  if (!aa) {
    auto ca = best_number_type(a.as_utf8());
    ta = ca.type();
    if (ta == Null)
      return {};
    if (!ab) {
      auto cb = best_number_type(b.as_utf8());
      tb = cb.type();
      if (tb == Null)
        return {};
      return arithmetic_binary_operation_assuming_one_float_or_both_integral(
            ca, cb, float8_op, unsigned8_op, signed8_op, bool1_op, ta, tb);
    }
    return arithmetic_binary_operation_assuming_one_float_or_both_integral(
          ca, b, float8_op, unsigned8_op, signed8_op, bool1_op, ta, tb);
  }
  auto cb = best_number_type(b.as_utf8());
  tb = cb.type();
  if (tb == Null)
    return {};
  return arithmetic_binary_operation_assuming_one_float_or_both_integral(
        a, cb, float8_op, unsigned8_op, signed8_op, bool1_op, ta, tb);
}

TypedValue TypedValue::add(const TypedValue &a, const TypedValue &b) {
  return arithmetic_binary_operation_with_best_type(
        a, b,
        [](double fa, double fb) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    return fa + fb;
  }, [](uint64_t ua, uint64_t ub) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    // C++26: use ckd_add
#if __has_builtin(__builtin_add_overflow)
    unsigned long long u;
    return __builtin_uaddll_overflow(ua, ub, &u) ? TypedValue{} : u;
#else
    return ua + ub;
#endif
  }, [](int64_t ia, int64_t ib) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    // C++26: use ckd_add
#if __has_builtin(__builtin_add_overflow)
    signed long long i;
    return __builtin_saddll_overflow(ia, ib, &i) ? TypedValue{} : i;
#else
    return ia + ib;
#endif
  }, [](bool ba, bool bb) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    return ba || bb; // addition is disjonction
  });
}

TypedValue TypedValue::sub(const TypedValue &a, const TypedValue &b) {
  return arithmetic_binary_operation_with_best_type(
        a, b,
        [](double fa, double fb) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    return fa - fb;
  }, [](uint64_t ua, uint64_t ub) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    if (ub > ua) {
      // the result is < 0, try to make it fit in a signed8
      if (ub < std::numeric_limits<int64_t>::max()
          && ua < std::numeric_limits<int64_t>::max())
        return (int64_t)ua - (int64_t)ub;
      return {};
    }
    // C++26: use ckd_sub
#if __has_builtin(__builtin_sub_overflow)
    unsigned long long u;
    return __builtin_usubll_overflow(ua, ub, &u) ? TypedValue{} : u;
#else
    return ua - ub;
#endif
  }, [](int64_t ia, int64_t ib) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    // C++26: use ckd_sub
#if __has_builtin(__builtin_sub_overflow)
    signed long long i;
    return __builtin_ssubll_overflow(ia, ib, &i) ? TypedValue{} : i;
#else
    return ia - ib;
#endif
  }, [](bool ba, bool bb) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    return ba != bb; // substraction is xor
  });
}

TypedValue TypedValue::mul(const TypedValue &a, const TypedValue &b) {
  return arithmetic_binary_operation_with_best_type(
        a, b,
        [](double fa, double fb) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    return fa * fb;
  }, [](uint64_t ua, uint64_t ub) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    // C++26: use ckd_mul
#if __has_builtin(__builtin_mul_overflow)
    unsigned long long u;
    return __builtin_umulll_overflow(ua, ub, &u) ? TypedValue{} : u;
#else
#warning TypedValue::{multiply,add,substract,divide,modulo} w/o checked overflow
    return ua * ub;
#endif
  }, [](int64_t ia, int64_t ib) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    // C++26: use ckd_mul
#if __has_builtin(__builtin_mul_overflow)
    signed long long i;
    return __builtin_smulll_overflow(ia, ib, &i) ? TypedValue{} : i;
#else
    return ia * ib;
#endif
  }, [](bool ba, bool bb) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    return ba && bb; // multiplication is conjonction
  });
}

TypedValue TypedValue::div(const TypedValue &a, const TypedValue &b) {
  return arithmetic_binary_operation_with_best_type(
        a, b,
        [](double fa, double fb) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    return fa / fb;
  }, [](uint64_t ua, uint64_t ub) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    if (ub == 0)
      return {};
    return ua / ub;
  }, [](int64_t ia, int64_t ib) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    if (ib == 0)
      return {};
    return ia / ib;
  }, [](bool ba, bool bb) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    if (!bb)
      return {};
    return ba; // division is identity, provided the divisor is non null
  });
}

TypedValue TypedValue::mod(const TypedValue &a, const TypedValue &b) {
  return arithmetic_binary_operation_with_best_type(
        a, b,
        [](double fa, double fb) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    return std::fmod(fa, fb);
  }, [](uint64_t ua, uint64_t ub) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    if (ub == 0)
      return {};
    return ua % ub;
  }, [](int64_t ia, int64_t ib) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    if (ib == 0)
      return {};
    return ia % ib;
  }, [](bool ba, bool bb) __attribute__((always_inline)) STATIC_LAMBDA -> TypedValue {
    if (!bb)
      return {};
    return ba;  // division is identity, provided the divisor is non null
  });
}

// Unsigned8Value /////////////////////////////////////////////////////////////

Type TypedValue::Unsigned8Value::type() const {
  return Unsigned8;
}

bool TypedValue::Unsigned8Value::operator!() const {
  return false;
}

std::partial_ordering TypedValue::Unsigned8Value::operator<=>(
    const Value &value) const {
  const auto &other = static_cast<const Unsigned8Value&>(value);
  return u <=> other.u;
}

bool TypedValue::Unsigned8Value::operator==(const Value &value) const {
  const auto &other = static_cast<const Unsigned8Value&>(value);
  return u == other.u;
}

uint64_t TypedValue::Unsigned8Value::unsigned8() const {
  return u;
}

uint64_t TypedValue::Unsigned8Value::as_unsigned8(uint64_t, bool *ok) const {
  if (ok) *ok = true;
  return u;
}

int64_t TypedValue::Unsigned8Value::as_signed8(int64_t def, bool *ok) const {
  if (u <= std::numeric_limits<int64_t>::max()) {
    if (ok) *ok = true;
    return u;
  }
  if (ok) *ok = false;
  return def;
}

double TypedValue::Unsigned8Value::as_float8(double, bool *ok) const {
  if (ok) *ok = true;
  return u;
}

bool TypedValue::Unsigned8Value::as_bool1(bool, bool *ok) const {
  if (ok) *ok = true;
  return u;
}

Utf8String TypedValue::Unsigned8Value::as_utf8(
    const Utf8String &, bool *ok) const {
  if (ok) *ok = true;
  return Utf8String::number(u);
}

QDateTime TypedValue::Unsigned8Value::as_timestamp8(const QDateTime &def, bool *ok) const {
  if (u <= std::numeric_limits<int64_t>::max()) {
    auto ts = QDateTime::fromMSecsSinceEpoch(u);
    if (ts.isValid()) {
      if (ok) *ok = true;
      return ts;
    }
  }
  if (ok) *ok = false;
  return def;
}

// EntityValue ///////////////////////////////////////////////////////////////

Type TypedValue::EntityValue::type() const {
  return Entity8;
}

bool TypedValue::EntityValue::operator!() const {
  return !u;
}

Entity TypedValue::EntityValue::entity8() const {
  return u;
}

Utf8String TypedValue::EntityValue::as_utf8(
    const Utf8String &, bool *ok) const {
  if (ok) *ok = true;
  return "0x"_u8+Utf8String::number(u, 16);
}

// Bool1Value ////////////////////////////////////////////////////////////////

Type TypedValue::Bool1Value::type() const {
  return Bool1;
}

std::partial_ordering TypedValue::Bool1Value::operator<=>(
    const Value &value) const {
  const auto &other = static_cast<const Bool1Value&>(value);
  return !!u <=> !!other.u;
}

bool TypedValue::Bool1Value::operator==(const Value &value) const {
  const auto &other = static_cast<const Bool1Value&>(value);
  return !!u == !!other.u;
}


bool TypedValue::Bool1Value::bool1() const {
  return u;
}

Utf8String TypedValue::Bool1Value::as_utf8(const Utf8String &, bool *ok) const {
  if (ok) *ok = true;
  return Utf8String::number((bool)u);
}

// Signed8Value //////////////////////////////////////////////////////////////

Type TypedValue::Signed8Value::type() const {
  return Signed8;
}

bool TypedValue::Signed8Value::operator!() const {
  return false;
}

std::partial_ordering TypedValue::Signed8Value::operator<=>(
    const Value &value) const {
  const auto &other = static_cast<const Signed8Value&>(value);
  return i <=> other.i;
}

bool TypedValue::Signed8Value::operator==(const Value &value) const {
  const auto &other = static_cast<const Signed8Value&>(value);
  return i == other.i;
}

int64_t TypedValue::Signed8Value::signed8() const {
  return i;
}

int64_t TypedValue::Signed8Value::as_signed8(int64_t, bool *ok) const {
  if (ok) *ok = true;
  return i;
}

uint64_t TypedValue::Signed8Value::as_unsigned8(uint64_t def, bool *ok) const {
  if (i > 0) {
    if (ok) *ok = true;
    return i;
  }
  if (ok) *ok = false;
  return def;
}

double TypedValue::Signed8Value::as_float8(double, bool *ok) const {
  if (ok) *ok = true;
  return i;
}

bool TypedValue::Signed8Value::as_bool1(bool, bool *ok) const {
  if (ok) *ok = true;
  return i;
}

Utf8String TypedValue::Signed8Value::as_utf8(
    const Utf8String &, bool *ok) const {
  if (ok) *ok = true;
  return Utf8String::number(i);
}

QDateTime TypedValue::Signed8Value::as_timestamp8(
    const QDateTime &def, bool *ok) const {
  auto ts = QDateTime::fromMSecsSinceEpoch(i);
  if (ts.isValid()) {
    if (ok) *ok = true;
    return ts;
  }
  if (ok) *ok = false;
  return def;

}

// Float8Value ////////////////////////////////////////////////////////////////

Type TypedValue::Float8Value::type() const {
  return Float8;
}

bool TypedValue::Float8Value::operator!() const {
  return std::isnan(f);
}

std::partial_ordering TypedValue::Float8Value::operator<=>(
    const Value &value) const {
  const auto &other = static_cast<const Float8Value&>(value);
  return f <=> other.f;
}

bool TypedValue::Float8Value::operator==(const Value &value) const {
  const auto &other = static_cast<const Float8Value&>(value);
  return f == other.f;
}

double TypedValue::Float8Value::float8() const {
  return f;
}

uint64_t TypedValue::Float8Value::as_unsigned8(uint64_t def, bool *ok) const {
  if (p6::double_fits_in_integral_type<uint64_t>(f)) {
    if (ok) *ok = true;
    return f;
  }
  if (ok) *ok = false;
  return def;
}

int64_t TypedValue::Float8Value::as_signed8(int64_t def, bool *ok) const {
  if (p6::double_fits_in_integral_type<int64_t>(f)) {
    if (ok) *ok = true;
    return f;
  }
  if (ok) *ok = false;
  return def;
}

bool TypedValue::Float8Value::as_bool1(bool, bool *ok) const {
  if (ok) *ok = true;
  return f;
}

double TypedValue::Float8Value::as_float8(double, bool *ok) const {
  if (ok) *ok = true;
  return f;
}

Utf8String TypedValue::Float8Value::as_utf8(
    const Utf8String &, bool *ok) const {
  if (ok) *ok = true;
  if (f == 0 && std::signbit(f))
    return "-0.0"_u8; // QByteArray::number() doesn't support -0.0
  return Utf8String::number(f, 'g', QLocale::FloatingPointShortest);
}

// BytesValue and Utf8Value ///////////////////////////////////////////////////

Type TypedValue::BytesValue::type() const {
  return Bytes;
}

bool TypedValue::BytesValue::operator!() const {
  return !s;
}

std::partial_ordering TypedValue::BytesValue::operator<=>(
    const Value &value) const {
  const auto &other = static_cast<const BytesValue&>(value);
  return s <=> other.s;
}

bool TypedValue::BytesValue::operator==(const Value &value) const {
  const auto &other = static_cast<const BytesValue&>(value);
  return s == other.s;
}

QByteArray TypedValue::BytesValue::bytes() const {
  return s;
}

uint64_t TypedValue::BytesValue::as_unsigned8(uint64_t def, bool *ok) const {
  return s.toNumber<uint64_t>(ok, def);
}

int64_t TypedValue::BytesValue::as_signed8(int64_t def, bool *ok) const {
  return s.toNumber<int64_t>(ok, def);
}

double TypedValue::BytesValue::as_float8(double def, bool *ok) const {
  return s.toNumber<double>(ok, def);
}

bool TypedValue::BytesValue::as_bool1(bool def, bool *ok) const {
  return s.toBool(ok, def);
}

Utf8String TypedValue::BytesValue::as_utf8(const Utf8String &, bool *ok) const {
  if (ok) *ok = true;
  return s;
}

QDateTime TypedValue::BytesValue::as_timestamp8(const QDateTime &def, bool *ok) const {
  auto ts = QDateTime::fromString(s, Qt::ISODateWithMs);
  if (ts.isValid()) {
    if (ok) *ok = true;
    return ts;
  }
  if (ok) *ok = false;
  return def;
}

QRegularExpression TypedValue::BytesValue::as_regexp(const QRegularExpression &def, bool *ok) const {
  auto re = QRegularExpression(s);
  if (re.isValid()) {
    if (ok) *ok = true;
    return re;
  }
  if (ok) *ok = false;
  return def;
}

static inline std::vector<Entity> utf8_to_evector(
    const Utf8String s, const std::vector<Entity> &def = {}, bool *ok = 0) {
  bool unsigned_ok;
  auto uv = utf8_to_uvector(s, {}, &unsigned_ok);
  if (unsigned_ok) {
    std::vector<Entity> ev;
    for (auto u: uv)
      ev.push_back(Entity{u});
    if (ok) *ok = true;
    return ev;
  }
  if (ok) *ok = false;
  return def;
}

const std::vector<Entity> TypedValue::BytesValue::as_entityvector(
    const std::vector<Entity> &def, bool *ok) const {
  return utf8_to_evector(s, def, ok);
}

const std::vector<double> TypedValue::BytesValue::as_fvector(
    const std::vector<double> &def, bool *ok) const {
  return utf8_to_fvector(s, def, ok);
}

static inline std::vector<QPointF> utf8_to_pointfvector(
    const Utf8String &s, const std::vector<QPointF> &def = {}, bool *ok = 0) {
  bool matrix_ok;
  auto fm = utf8_to_number2dmatrix<double>(
              s.constData(), s.size(), {}, &matrix_ok);
  if (matrix_ok) {
    std::vector<QPointF> pv;
    for (const auto &fv: fm) {
      if (fv.size() != 2)
        goto failure;
      pv.push_back(QPointF(fv[0], fv[1]));
    }
    if (ok) *ok = true;
    return pv;
  }
failure:
  if (ok) *ok = false;
  return def;
}

const std::vector<QPointF> TypedValue::BytesValue::as_pointfvector(
    const std::vector<QPointF> &def, bool *ok) const {
  return utf8_to_pointfvector(s, def, ok);
}

QPointF TypedValue::BytesValue::as_pointf(const QPointF &def, bool *ok) const {
  auto v = utf8_to_fvector(s);
  if (v.size() == 2) {
    if (ok) *ok = true;
    return QPointF{v[0], v[1]};
  }
  if (ok) *ok = false;
  return def;
}

QSizeF TypedValue::BytesValue::as_sizef(const QSizeF &def, bool *ok) const {
  auto v = utf8_to_fvector(s);
  if (v.size() == 2) {
    if (ok) *ok = true;
    return QSizeF{v[0], v[1]};
  }
  if (ok) *ok = false;
  return def;
}

QRectF TypedValue::BytesValue::as_rectf(const QRectF &def, bool *ok) const {
  auto v = utf8_to_fvector(s);
  if (v.size() == 4) {
    if (ok) *ok = true;
    return QRectF{v[0], v[1], v[2], v[3]};
  }
  if (ok) *ok = false;
  return def;
}

QLineF TypedValue::BytesValue::as_linef(const QLineF &def, bool *ok) const {
  auto v = utf8_to_fvector(s);
  if (v.size() == 4) {
    if (ok) *ok = true;
    return QLineF{v[0], v[1], v[2], v[3]};
  }
  if (ok) *ok = false;
  return def;
}

Type TypedValue::Utf8Value::type() const {
  return Utf8;
}

Utf8String TypedValue::Utf8Value::utf8() const {
  return s;
}

Utf8String TypedValue::Utf8Value::as_utf8(const Utf8String &, bool *ok) const {
  if (ok) *ok = true;
  return s;
}

// EntityVectorValue //////////////////////////////////////////////////////////

Type TypedValue::EntityVectorValue::type() const {
  return Entity8Vector;
}

bool TypedValue::EntityVectorValue::operator!() const {
  return false;
}

std::partial_ordering TypedValue::EntityVectorValue::operator<=>(const Value &value) const {
  const auto &other = static_cast<const EntityVectorValue&>(value);
  return v <=> other.v;
}

bool TypedValue::EntityVectorValue::operator==(const Value &value) const {
  const auto &other = static_cast<const EntityVectorValue&>(value);
  return v == other.v;
}

const std::vector<Entity> &TypedValue::EntityVectorValue::entityvector() const {
  return v;
}

Utf8String TypedValue::EntityVectorValue::as_utf8(
    const Utf8String &, bool *ok) const {
  Utf8String s;
  for (bool begin = true; const auto &e: v) {
    if (begin)
      begin = false;
    else
      s += ' ';
    s += e.n3();
  }
  if (ok) *ok = true;
  return s;
}

const std::vector<Entity> TypedValue::EntityVectorValue::as_entityvector(
    const std::vector<Entity> &, bool *ok) const {
  if (ok) *ok = true;
  return v;
}

// FVectorValue ///////////////////////////////////////////////////////////////

Type TypedValue::FVectorValue::type() const {
  return FVector;
}

bool TypedValue::FVectorValue::operator!() const {
  return false;
}

std::partial_ordering TypedValue::FVectorValue::operator<=>(
    const Value &value) const {
  const auto &other = static_cast<const FVectorValue&>(value);
  return v <=> other.v;
}

bool TypedValue::FVectorValue::operator==(const Value &value) const {
  const auto &other = static_cast<const FVectorValue&>(value);
  return v == other.v;
}

const std::vector<double> &TypedValue::FVectorValue::fvector() const {
  return v;
}

Utf8String TypedValue::FVectorValue::as_utf8(const Utf8String &, bool *ok) const {
  Utf8String s;
  for (bool begin = true; const auto &f: v) {
    if (begin)
      begin = false;
    else
      s += ',';
    s += Utf8String::number(f, 'g', QLocale::FloatingPointShortest);
  }
  if (ok) *ok = true;
  return s;
}

const std::vector<double> TypedValue::FVectorValue::as_fvector(
    const std::vector<double> &, bool *ok) const {
  if (ok) *ok = true;
  return v;
}

QPointF TypedValue::FVectorValue::as_pointf(const QPointF &def, bool *ok) const {
  if (v.size() >= 2) {
    if (ok) *ok = true;
    return QPointF{v[0], v[1]};
  }
  if (ok) *ok = false;
  return def;
}

QSizeF TypedValue::FVectorValue::as_sizef(const QSizeF &def, bool *ok) const {
  if (v.size() >= 2) {
    if (ok) *ok = true;
    return QSizeF{v[0], v[1]};
  }
  if (ok) *ok = false;
  return def;
}

QRectF TypedValue::FVectorValue::as_rectf(const QRectF &def, bool *ok) const {
  auto s = v.size();
  if (s >= 4 && s%2 == 0) {
    auto dim = s/2;
    if (ok) *ok = true;
    return QRectF{v[0], v[1], v[dim+0], v[dim+1]};
  }
  if (ok) *ok = false;
  return def;
}

// PointFValue ////////////////////////////////////////////////////////////////

TypedValue::PointFValue::PointFValue(const QPointF &p)
  : FVectorValue(std::vector<double>{p.x(), p.y()}) {
}

Type TypedValue::PointFValue::type() const {
  return PointF;
}

QPointF TypedValue::PointFValue::pointf() const {
  return as_pointf({}, 0);
}

const std::vector<QPointF> TypedValue::PointFValue::as_pointfvector(const std::vector<QPointF> &, bool *ok) const {
  if (ok) *ok = true;
  return std::vector<QPointF>{pointf()};
}

QPointF TypedValue::pointf() const {
  return value().pointf();
}

// SizeFValue ////////////////////////////////////////////////////////////////

TypedValue::SizeFValue::SizeFValue(const QSizeF &p)
  : FVectorValue(std::vector<double>{p.width(), p.height()}) {
}

Type TypedValue::SizeFValue::type() const {
  return SizeF;
}

QSizeF TypedValue::SizeFValue::sizef() const {
  return as_sizef({}, 0);
}

QSizeF TypedValue::sizef() const {
  return value().sizef();
}

// RectFValue ////////////////////////////////////////////////////////////////

TypedValue::RectFValue::RectFValue(const QRectF &p)
  : FVectorValue(std::vector<double>{p.x(), p.y(), p.width(), p.height()}) {
}

Type TypedValue::RectFValue::type() const {
  return RectF;
}

QRectF TypedValue::RectFValue::rectf() const {
  return as_rectf({}, 0);
}

QRectF TypedValue::rectf() const {
  return value().rectf();
}

// LineFValue ////////////////////////////////////////////////////////////////

TypedValue::LineFValue::LineFValue(const QLineF &p)
  : FVectorValue(std::vector<double>{p.x1(), p.y1(), p.x2(), p.y2()}) {
}

Type TypedValue::LineFValue::type() const {
  return LineF;
}

QLineF TypedValue::LineFValue::linef() const {
  return as_linef({}, 0);
}

QLineF TypedValue::linef() const {
  return value().linef();
}

// PointFVectorValue //////////////////////////////////////////////////////////

TypedValue::PointFVectorValue::PointFVectorValue(const std::vector<QPointF> &v)
  : v(v) {
}

TypedValue::PointFVectorValue::PointFVectorValue(std::vector<QPointF> &&v)
  : v(v) {
}

Type TypedValue::PointFVectorValue::type() const {
  return PointFVector;
}

bool TypedValue::PointFVectorValue::operator!() const {
  return false;
}

bool TypedValue::PointFVectorValue::operator==(const Value &value) const {
  const auto &other = static_cast<const PointFVectorValue&>(value);
  return v == other.v;
}

const std::vector<QPointF> &TypedValue::PointFVectorValue::pointfvector() const {
  return v;
}

Utf8String TypedValue::PointFVectorValue::as_utf8(const Utf8String &, bool *ok) const {
  Utf8String s;
  for (bool begin = true; const auto &p: v) {
    if (begin)
      begin = false;
    else
      s += ' ';
    s += Utf8String::number(p.x(), 'g', QLocale::FloatingPointShortest);
    s += ',';
    s += Utf8String::number(p.y(), 'g', QLocale::FloatingPointShortest);
  }
  if (ok) *ok = true;
  return s;
}

const std::vector<QPointF> TypedValue::PointFVectorValue::as_pointfvector(
    const std::vector<QPointF> &, bool *ok) const {
  if (ok) *ok = true;
  return v;
}

QPointF TypedValue::PointFVectorValue::as_pointf(const QPointF &def, bool *ok) const {
  if (v.size() == 1) {
    if (ok) *ok = true;
    return v[0];
  }
  if (ok) *ok = false;
  return def;
}

const std::vector<QPointF> TypedValue::as_pointfvector(
    const std::vector<QPointF> &def, bool *ok) const {
  return value().as_pointfvector(def, ok);
}

// Timestamp8Value ///////////////////////////////////////////////////////////

TypedValue::Timestamp8Value::Timestamp8Value(const QDateTime &ts)
  : ts(new QDateTime(ts)) {
}

TypedValue::Timestamp8Value::~Timestamp8Value() {
  delete ts;
}

Type TypedValue::Timestamp8Value::type() const {
  return Timestamp8;
}

bool TypedValue::Timestamp8Value::operator!() const {
  return !ts->isValid();
}

std::partial_ordering TypedValue::Timestamp8Value::operator<=>(
    const Value &value) const {
  const auto &other = static_cast<const Timestamp8Value&>(value);
  return *ts <=> *other.ts;
}

bool TypedValue::Timestamp8Value::operator==(const Value &value) const {
  const auto &other = static_cast<const Timestamp8Value&>(value);
  return *ts == *other.ts;
}

QDateTime TypedValue::Timestamp8Value::timestamp8() const {
  return *ts;
}

uint64_t TypedValue::Timestamp8Value::as_unsigned8(
    uint64_t def, bool *ok) const {
  if (ts->isValid()) {
    int64_t ms = ts->toMSecsSinceEpoch();
    if (ms > 0) {
      if (ok) *ok = true;
      return ms;
    }
  }
  if (ok) *ok = false;
  return def;
}

int64_t TypedValue::Timestamp8Value::as_signed8(int64_t def, bool *ok) const {
  if (ts->isValid()) {
    if (ok) *ok = true;
    return ts->toMSecsSinceEpoch();
  }
  if (ok) *ok = false;
  return def;
}

double TypedValue::Timestamp8Value::as_float8(double def, bool *ok) const {
  if (ts->isValid()) {
    if (ok) *ok = true;
    return ts->toMSecsSinceEpoch();
  }
  if (ok) *ok = false;
  return def;
}

Utf8String TypedValue::Timestamp8Value::as_utf8(
    const Utf8String &def, bool *ok) const {
  if (ts->isValid()) {
    if (ok) *ok = true;
    return ts->toString(Qt::ISODateWithMs);
  }
  if (ok) *ok = false;
  return def;
}

QDateTime TypedValue::timestamp8() const {
  return value().timestamp8();
}

// RegexpValue ////////////////////////////////////////////////////////////////

TypedValue::RegexpValue::RegexpValue(const QRegularExpression &re)
  : re(new QRegularExpression(re)) {
}

TypedValue::RegexpValue::~RegexpValue() {
  delete re;
}

Type TypedValue::RegexpValue::type() const {
  return Regexp;
}

bool TypedValue::RegexpValue::operator!() const {
  return !re->isValid();
}

bool TypedValue::RegexpValue::operator==(const Value &value) const {
  const auto &other = static_cast<const RegexpValue&>(value);
  return *re == *other.re;
}

QRegularExpression TypedValue::RegexpValue::regexp() const {
  return *re;
}

QRegularExpression TypedValue::RegexpValue::as_regexp(const QRegularExpression &def, bool *ok) const {
  if (re->isValid()) {
    if (ok) *ok = true;
    return *re;
  }
  if (ok) *ok = false;
  return def;
}

Utf8String TypedValue::RegexpValue::as_utf8(
    const Utf8String &def, bool *ok) const {
  if (re->isValid()) {
    if (ok) *ok = true;
    return re->pattern();
  }
  if (ok) *ok = false;
  return def;
}

QRegularExpression TypedValue::regexp() const {
  return value().regexp();
}

// EmbeddedQVariantValue //////////////////////////////////////////////////////

Type TypedValue::EmbeddedQVariantValue::type() const {
  return EmbeddedQVariant;
}

bool TypedValue::EmbeddedQVariantValue::operator!() const {
  return !v.isValid();
}

bool TypedValue::EmbeddedQVariantValue::operator==(const Value &value) const {
  const auto &other = static_cast<const EmbeddedQVariantValue&>(value);
  return v == other.v;
}

QVariant TypedValue::EmbeddedQVariantValue::qvariant() const {
  return v;
}

Utf8String TypedValue::EmbeddedQVariantValue::as_utf8(
    const Utf8String &def, bool *ok) const {
  Utf8String s{v};
  if (!!s) {
    if (ok) *ok = true;
    return s;
  }
  if (ok) *ok = false;
  return def;
}

// QVariant and QString conversion ////////////////////////////////////////////

TypedValue::TypedValue(const QString &utf16) : TypedValue(Utf8String{utf16}) {
}

QString TypedValue::as_utf16() const {
  return utf8();
}

QString TypedValue::toString() const {
  return utf8();
}

TypedValue::TypedValue(const QVariant &v) : d(std::move(from_qvariant(v).d)) {
}

TypedValue TypedValue::from_qvariant(const QVariant &v) {
  auto mtid = v.metaType().id();
  switch (mtid) {
    case QMetaType::UnknownType:
    case QMetaType::Void:
      return {};
    case QMetaType::Bool:
      return v.toBool();
    case QMetaType::Short:
    case QMetaType::Int:
    case QMetaType::Long:
    case QMetaType::LongLong:
      return v.toLongLong();
    case QMetaType::UShort:
    case QMetaType::UInt:
    case QMetaType::ULong:
    case QMetaType::ULongLong:
      return v.toULongLong();
    case QMetaType::Float:
    case QMetaType::Double:
      return v.toDouble();
    case QMetaType::QString:
    case QMetaType::QTime:
    case QMetaType::QDate:
      return v.value<Utf8String>();
    case QMetaType::QUuid:
    case QMetaType::QByteArray:
      return v.toByteArray();
    case QMetaType::QChar:
      return Utf8String{v.value<QChar>().unicode()};
    case QMetaType::Char:
      return Utf8String{v.value<char>()};
    case QMetaType::Char16:
      return Utf8String{v.value<char16_t>()};
    case QMetaType::Char32:
      return Utf8String{v.value<char32_t>()};
    case QMetaType::SChar:
      return Utf8String{v.value<signed char>()};
    case QMetaType::UChar:
      return Utf8String{v.value<unsigned char>()};
    case QMetaType::QPoint:
      return QPointF(v.value<QPoint>());
    case QMetaType::QPointF:
      return v.value<QPointF>();
    case QMetaType::QSize:
      return QSizeF(v.value<QSize>());
    case QMetaType::QSizeF:
      return v.value<QSizeF>();
    case QMetaType::QRect:
      return QRectF(v.value<QRect>());
    case QMetaType::QRectF:
      return v.value<QRectF>();
    case QMetaType::QLine:
      return QLineF(v.value<QLine>());
    case QMetaType::QLineF:
      return v.value<QLineF>();
    case QMetaType::QDateTime:
        return v.value<QDateTime>();
  }
  if (mtid == Utf8String::MetaTypeId)
    return v.value<Utf8String>();
  if (mtid == Entity::MetaTypeId)
    return v.value<Entity>();
  // embed QVariant in an opaque manner in order to make ti possible to give it
  // back untouched with as_qvariant()
  return TypedValue(new EmbeddedQVariantValue(v));
  // maybe needed: EntityList (or even QList<Entity>): not sure, this is only
  // used within EntityMimeData where only QVariant is used
  // maybe needed: QList<QPointF>: probably not, Utf8String provides it through
  // plain C++ methods but does not provide registred QMetaType converters
  // for some classes (custom classes, gui classes like QColor, QFont...) we
}

QVariant TypedValue::as_qvariant() const {
  switch (type()) {
    case Unsigned8:
      return QVariant::fromValue(direct_unsigned8());
    case Entity8:
      return Entity{direct_unsigned8()};
    case Bool1:
      return QVariant::fromValue(!!direct_unsigned8());
    case Signed8:
      return QVariant::fromValue(direct_signed8());
    case Float8:
      return QVariant::fromValue(direct_float8());
    case Bytes:
      return QByteArray(direct_utf8());
    case Utf8:
      return direct_utf8();
    case Entity8Vector: {
        const auto &v = d->entityvector();
        return QVariant::fromValue(EntityList(QList<Entity>(v.begin(), v.end())));
      }
    case FVector: {
        const auto &v = d->fvector();
        return QVariant::fromValue(QList<double>(v.begin(), v.end()));
      }
    case PointF:
      return d->pointf();
    case SizeF:
      return d->sizef();
    case RectF:
      return d->rectf();
    case LineF:
      return d->linef();
    case PointFVector: {
        const auto &v = d->pointfvector();
        return QVariant::fromValue(QList<QPointF>(v.begin(), v.end()));
      }
    case Timestamp8:
      return d->timestamp8();
    case Regexp:
      return d->regexp();
    case EmbeddedQVariant:
      return d->qvariant();
    case Null:
      ;
  }
  return {};
}

// External Typed Value format ////////////////////////////////////////////////

const static QMap<TypedValue::Type,Utf8String> _typecodes {
  { TypedValue::Null, "null" },
  { TypedValue::Unsigned8, "u8" },
  { TypedValue::Entity8, "e" },
  { TypedValue::Bool1, "b" },
  { TypedValue::Signed8, "i8" },
  { TypedValue::Float8, "f8" },
  { TypedValue::Bytes, "bytes" },
  { TypedValue::Utf8, "utf8" },
  { TypedValue::FVector, "e[]" },
  { TypedValue::FVector, "f8[]" },
  { TypedValue::PointF, "point" },
  { TypedValue::SizeF, "size" },
  { TypedValue::RectF, "rect" },
  { TypedValue::PointFVector, "point[]" },
  { TypedValue::Timestamp8, "ts" },
  { TypedValue::Regexp, "re" },
};

const static auto _from_typecodes = p6::reversed_map(_typecodes);

Utf8String TypedValue::typecode(Type type) {
  return _typecodes.value(type, "bytes"_u8);
}

Type TypedValue::from_typecode(const Utf8String &typecode) {
  return _from_typecodes.value(typecode, Null);
}

Utf8String TypedValue::as_etv() const {
  auto t = type();
  switch (t) {
    case Utf8:
      return '"'+direct_utf8().cEscaped()+'"';
    case Bytes:
      return "bytes{"_u8+direct_utf8().toHex()+"}"_u8;
    case Null:
      return "null{}"_u8;
    default:
      ;
  }
  auto code = _typecodes.value(t) | "null"_u8;
  auto unquoted_etv = as_utf8().cEscaped().replace('}', "\\}"_u8);
  return code+"{"_u8+unquoted_etv+"}"_u8;
}

TypedValue TypedValue::from_etv(const Utf8String &etv) {
  auto size = etv.size(), i = etv.indexOf('{');
  if (i == -1)
    return {};
  auto type = _from_typecodes.value(etv.first(i), Null);
  if (type == Null || etv[size-1] != '}')
    return {};
  return from_etv(type, etv.sliced(i+1, size-i-2));
}

TypedValue TypedValue::from_etv(Type type, const Utf8String &unquoted_etv) {
  switch (type) {
    case Unsigned8:
      return unquoted_etv.toNumber<uint64_t>();
    case Entity8:
      return Entity{unquoted_etv.toNumber<uint64_t>()};
    case Bool1:
      return unquoted_etv.toNumber<bool>();
    case Signed8:
      return unquoted_etv.toNumber<int64_t>();
    case Float8:
      if (auto d = unquoted_etv.toNumber<double>();
          d == 0 && unquoted_etv.value(0) == '-')
        return -0.0; // QByteArray::toDouble() doesn't support -0.0
      else
        return d;
    case Bytes:
      return QByteArray::fromHex(unquoted_etv);
    case Utf8:
      return unquoted_etv;
    case Entity8Vector:
      return utf8_to_evector(unquoted_etv);
    case FVector:
      return utf8_to_fvector(unquoted_etv);
    case PointF:
      return unquoted_etv.toPointF();
    case SizeF:
      return unquoted_etv.toSizeF();
    case RectF:
      return unquoted_etv.toRectF();
    case LineF:
      return unquoted_etv.toLineF();
    case PointFVector:
       return utf8_to_pointfvector(unquoted_etv);
    case Timestamp8:
      return QDateTime::fromString(unquoted_etv, Qt::ISODateWithMs);
    case Regexp:
      return QRegularExpression(unquoted_etv);
    case EmbeddedQVariant:
    case Null:
      qWarning() << "TypedValue::from_etv called with unsupported type"
                 << type;
      ;
  }
  return {};
}

// misc ///////////////////////////////////////////////////////////////////////

QDebug operator<<(QDebug dbg, TypedValue o) {
  return dbg.noquote() << o.as_etv();
}

log::LogHelper operator<<(log::LogHelper lh, TypedValue o) {
  return lh << o.as_etv();
}

} // p6
