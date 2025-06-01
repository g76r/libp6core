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
#ifndef TYPEDVALUE_H
#define TYPEDVALUE_H

#include "util/utf8string.h"
#include "eg/entity.h"
#include <QSharedDataPointer>

class QVariant;
class QString;
class QDateTime;
class QRegularExpression;
class QPointF;
class QSizeF;
class QRectF;
class QLineF;

namespace p6 {

namespace log {
class LogHelper;
}

struct LIBP6CORESHARED_EXPORT TypedValue {
public:
  enum Type {
    Null = 0,
    Unsigned8 = 0x40, Entity8 = 0x1, Bool1 = 0x41,
    Signed8 = 0x42,
    Float8 = 0x80,
    Bytes = 0x2, Utf8,
    //UVector, SVector,
    Entity8Vector,
    FVector, PointF, SizeF, RectF, LineF,
    //FMatrix2,
    PointFVector,
    Timestamp8, // TimestampWithTZ,
    Regexp,
    EmbeddedQVariant,
  };
  static inline bool is_integral(Type t) { return t & 0x40; }
  inline bool is_integral() const { return is_integral(type()); }
  static inline bool is_arithmetic(Type t) { return t & (0x80|0x40); }
  inline bool is_arithmetic() const { return is_arithmetic(type()); }

private:
  struct LIBP6CORESHARED_EXPORT Value : QSharedData {
    const static std::vector<Entity> _empty_entityvector;
    const static std::vector<double> _empty_fvector;
    const static std::vector<QPointF> _empty_pointfvector;
    virtual ~Value();
    virtual Type type() const = 0;
    virtual bool operator!() const;
    virtual std::partial_ordering operator<=>(const Value &) const;
    virtual bool operator==(const Value &) const;
    virtual uint64_t unsigned8() const;
    virtual int64_t signed8() const;
    virtual double float8() const;
    virtual bool bool1() const;
    virtual Utf8String utf8() const;
    virtual QByteArray bytes() const;
    virtual Entity entity8() const;
    virtual const std::vector<Entity> &entityvector() const;
    virtual const std::vector<double> &fvector() const;
    virtual const std::vector<QPointF> &pointfvector() const;
    virtual QPointF pointf() const;
    virtual QSizeF sizef() const;
    virtual QRectF rectf() const;
    virtual QLineF linef() const;
    virtual QDateTime timestamp8() const;
    virtual QRegularExpression regexp() const;
    virtual QVariant qvariant() const;
    virtual uint64_t as_unsigned8(uint64_t def, bool *ok) const;
    virtual int64_t as_signed8(int64_t def, bool *ok) const;
    virtual double as_float8(double def, bool *ok) const;
    virtual bool as_bool1(bool def, bool *ok) const;
    virtual Utf8String as_utf8(const Utf8String &def, bool *ok) const;
    virtual QDateTime as_timestamp8(const QDateTime &def, bool *ok) const;
    virtual QRegularExpression as_regexp(const QRegularExpression &def, bool *ok) const;
    virtual const std::vector<Entity> as_entityvector(const std::vector<Entity> &def, bool *ok) const;
    virtual const std::vector<double> as_fvector(const std::vector<double> &def, bool *ok) const;
    virtual const std::vector<QPointF> as_pointfvector(const std::vector<QPointF> &def, bool *ok) const;
    virtual QPointF as_pointf(const QPointF &def, bool *ok) const;
    virtual QSizeF as_sizef(const QSizeF &def, bool *ok) const;
    virtual QRectF as_rectf(const QRectF &def, bool *ok) const;
    virtual QLineF as_linef(const QLineF &def, bool *ok) const;
  };
  static_assert(sizeof(Value)==16);
  struct LIBP6CORESHARED_EXPORT NullValue : Value {
    const static NullValue _nullvalue;
    Type type() const override;
  };
  struct LIBP6CORESHARED_EXPORT Unsigned8Value : Value {
    uint64_t u = 0;
    Unsigned8Value(uint64_t u) : u(u) {}
    Type type() const override;
    bool operator!() const override;
    std::partial_ordering operator<=>(const Value &value) const override;
    bool operator==(const Value &value) const override;
    uint64_t unsigned8() const override;
    uint64_t as_unsigned8(uint64_t, bool *ok) const override;
    int64_t as_signed8(int64_t def, bool *ok) const override;
    double as_float8(double, bool *ok) const override;
    bool as_bool1(bool, bool *ok) const override;
    Utf8String as_utf8(const Utf8String &, bool *ok) const override;
    QDateTime as_timestamp8(const QDateTime &def, bool *ok) const override;
  };
  struct LIBP6CORESHARED_EXPORT EntityValue : Unsigned8Value {
    EntityValue(Entity e) : Unsigned8Value(e) { }
    Type type() const override;
    bool operator!() const override;
    using Value::unsigned8;
    Entity entity8() const override;
    using Value::as_unsigned8;
    using Value::as_signed8;
    using Value::as_float8;
    using Value::as_bool1;
    Utf8String as_utf8(const Utf8String &, bool *ok) const override;
    using Value::as_timestamp8;
  };
  struct LIBP6CORESHARED_EXPORT Bool1Value : Unsigned8Value {
    Bool1Value(bool b) : Unsigned8Value(b) {}
    Type type() const override;
    std::partial_ordering operator<=>(const Value &value) const override;
    virtual bool operator==(const Value &value) const override;
    using Value::unsigned8;
    bool bool1() const override;
    Utf8String as_utf8(const Utf8String &, bool *ok) const override;
    using Value::as_timestamp8;
  };
  struct LIBP6CORESHARED_EXPORT Signed8Value : Value {
    int64_t i = 0;
    Signed8Value(int64_t i) : i(i) {}
    Type type() const override;
    bool operator!() const override;
    std::partial_ordering operator<=>(const Value &value) const override;
    bool operator==(const Value &value) const override;
    int64_t signed8() const override;
    int64_t as_signed8(int64_t, bool *ok) const override;
    uint64_t as_unsigned8(uint64_t def, bool *ok) const override;
    double as_float8(double, bool *ok) const override;
    bool as_bool1(bool, bool *ok) const override;
    Utf8String as_utf8(const Utf8String &, bool *ok) const override;
    QDateTime as_timestamp8(const QDateTime &def, bool *ok) const override;
  };
  struct LIBP6CORESHARED_EXPORT Float8Value : Value {
    double f = 0;
    Float8Value(double f) : f(f) {}
    Type type() const override;
    bool operator!() const override;
    std::partial_ordering operator<=>(const Value &value) const override;
    bool operator==(const Value &value) const override;
    double float8() const override;
    uint64_t as_unsigned8(uint64_t def, bool *ok) const override;
    int64_t as_signed8(int64_t, bool *ok) const override;
    double as_float8(double def, bool *ok) const override;
    bool as_bool1(bool, bool *ok) const override;
    Utf8String as_utf8(const Utf8String &, bool *ok) const override;
  };
  struct LIBP6CORESHARED_EXPORT BytesValue : Value {
    Utf8String s;
    BytesValue(const QByteArray &bytes) : s(bytes) {}
    Type type() const override;
    bool operator!() const override;
    std::partial_ordering operator<=>(const Value &value) const override;
    virtual bool operator==(const Value &value) const override;
    QByteArray bytes() const override;
    uint64_t as_unsigned8(uint64_t def, bool *ok) const override;
    int64_t as_signed8(int64_t def, bool *ok) const override;
    double as_float8(double def, bool *ok) const override;
    bool as_bool1(bool def, bool *ok) const override;
    Utf8String as_utf8(const Utf8String &, bool *ok) const override;
    QDateTime as_timestamp8(const QDateTime &def, bool *ok) const override;
    QRegularExpression as_regexp(const QRegularExpression &def, bool *ok) const override;
    const std::vector<Entity> as_entityvector(const std::vector<Entity> &def, bool *ok) const override;
    const std::vector<double> as_fvector(const std::vector<double> &def, bool *ok) const override;
    const std::vector<QPointF> as_pointfvector(const std::vector<QPointF> &def, bool *ok) const override;
    QPointF as_pointf(const QPointF &def, bool *ok) const override;
    QSizeF as_sizef(const QSizeF &def, bool *ok) const override;
    QRectF as_rectf(const QRectF &def, bool *ok) const override;
    QLineF as_linef(const QLineF &def, bool *ok) const override;
  };
  struct LIBP6CORESHARED_EXPORT Utf8Value : BytesValue {
    Utf8Value(const Utf8String &utf8) : BytesValue(utf8) {}
    Type type() const override;
    using Value::bytes;
    Utf8String utf8() const override;
    Utf8String as_utf8(const Utf8String &, bool *ok) const override;
  };
  struct LIBP6CORESHARED_EXPORT EntityVectorValue : Value {
    std::vector<Entity> v;
    EntityVectorValue(const std::vector<Entity> &v) : v(v) {}
    EntityVectorValue(std::vector<Entity> &&v) : v(std::move(v)) {}
    Type type() const override;
    bool operator!() const override;
    std::partial_ordering operator<=>(const Value &value) const override;
    bool operator==(const Value &value) const override;
    const std::vector<Entity> &entityvector() const override;
    Utf8String as_utf8(const Utf8String &def, bool *ok) const override;
    const std::vector<Entity> as_entityvector(
        const std::vector<Entity> &def, bool *ok) const override;
  };
  struct LIBP6CORESHARED_EXPORT FVectorValue : Value {
    std::vector<double> v;
    FVectorValue(const std::vector<double> &v) : v(v) {}
    FVectorValue(std::vector<double> &&v) : v(std::move(v)) {}
    Type type() const override;
    bool operator!() const override;
    std::partial_ordering operator<=>(const Value &value) const override;
    bool operator==(const Value &value) const override;
    const std::vector<double> &fvector() const override;
    Utf8String as_utf8(const Utf8String &def, bool *ok) const override;
    const std::vector<double> as_fvector(const std::vector<double> &def, bool *ok) const override;
    QPointF as_pointf(const QPointF &def, bool *ok) const override;
    QSizeF as_sizef(const QSizeF &def, bool *ok) const override;
    QRectF as_rectf(const QRectF &def, bool *ok) const override;
  };
  struct LIBP6CORESHARED_EXPORT PointFValue : FVectorValue {
    PointFValue(const QPointF &p);
    Type type() const override;
    QPointF pointf() const override;
    const std::vector<QPointF> as_pointfvector(const std::vector<QPointF> &def, bool *ok) const override;
  };
  struct LIBP6CORESHARED_EXPORT SizeFValue : FVectorValue {
    SizeFValue(const QSizeF &p);
    Type type() const override;
    QSizeF sizef() const override;
  };
  struct LIBP6CORESHARED_EXPORT RectFValue : FVectorValue {
    RectFValue(const QRectF &p);
    Type type() const override;
    QRectF rectf() const override;
  };
  struct LIBP6CORESHARED_EXPORT LineFValue : FVectorValue {
    LineFValue(const QLineF &p);
    Type type() const override;
    QLineF linef() const override;
  };
  struct LIBP6CORESHARED_EXPORT PointFVectorValue : Value {
    std::vector<QPointF> v;
    PointFVectorValue(const std::vector<QPointF> &v);
    PointFVectorValue(std::vector<QPointF> &&v);
    Type type() const override;
    bool operator!() const override;
    virtual bool operator==(const Value &value) const override;
    const std::vector<QPointF> &pointfvector() const override;
    Utf8String as_utf8(const Utf8String &def, bool *ok) const override;
    const std::vector<QPointF> as_pointfvector(const std::vector<QPointF> &def, bool *ok) const override;
    QPointF as_pointf(const QPointF &def, bool *ok) const override;
  };
  struct LIBP6CORESHARED_EXPORT Timestamp8Value : Value {
    QDateTime *ts = 0;
    Timestamp8Value(const QDateTime &ts);
    ~Timestamp8Value();
    Type type() const override;
    bool operator!() const override;
    std::partial_ordering operator<=>(const Value &value) const override;
    virtual bool operator==(const Value &value) const override;
    QDateTime timestamp8() const override;
    uint64_t as_unsigned8(uint64_t def, bool *ok) const override;
    int64_t as_signed8(int64_t def, bool *ok) const override;
    double as_float8(double def, bool *ok) const override;
    Utf8String as_utf8(const Utf8String &def, bool *ok) const override;
  };
  struct LIBP6CORESHARED_EXPORT RegexpValue : Value {
    QRegularExpression *re = 0;
    RegexpValue(const QRegularExpression &re);
    ~RegexpValue();
    Type type() const override;
    bool operator !() const override;
    bool operator ==(const Value &) const override;
    QRegularExpression regexp() const override;
    Utf8String as_utf8(const Utf8String &def, bool *ok) const override;
    QRegularExpression as_regexp(const QRegularExpression &def, bool *ok) const override;
  };
  struct LIBP6CORESHARED_EXPORT EmbeddedQVariantValue : Value {
    QVariant v;
    EmbeddedQVariantValue(const QVariant &v) : v(v) {}
    EmbeddedQVariantValue(QVariant &&v) : v(v) {}
    Type type() const override;
    bool operator !() const override;
    bool operator ==(const Value &) const override;
    QVariant qvariant() const override;
    Utf8String as_utf8(const Utf8String &def, bool *ok) const override;
  };

public:
  // common methods //////////////////////////////////////////////////////////
  inline TypedValue() noexcept {}
  inline TypedValue(const TypedValue &other) noexcept : d(other.d) {}
  inline TypedValue(TypedValue &&other) noexcept : d(std::move(other.d)) {}
  inline ~TypedValue() noexcept {}
  inline TypedValue &operator=(const TypedValue &other) noexcept {
    if (this != &other) d = other.d;
    return *this; }
  inline TypedValue &operator=(TypedValue &&other) noexcept {
    if (this != &other) d = std::move(other.d);
    return *this; }
  /** highly depends on contained type
   *  - TypedValues of different types are always unordered
   *  - TypedValues of same type may or may not be unordered, e.g. <=> provides
   *    a strong ordering for integers and provides almost nothing for QPointF
   *  - operator == does not rely on <=>
   */
  [[nodiscard]] inline std::partial_ordering operator<=>(
      const TypedValue &other) const {
    if (type() != other.type())
      return std::partial_ordering::unordered;
    return value() <=> other.value(); }
  /** compare two TypedValue as numbers if both are numbers or can be converted
   *  to numbers (incl. timestamps which are ms since 1970 UTC) and otherwise
   *  compare them as characters string.
   *  if pretend_null_or_nan_is_empty is false (the default) the result will be
   *  unordered if one at less of the operands is null or NaN or cannot be
   *  converted to a string otherwise null, NaN and unconvertible values will be
   *  processed as if they were "".
   *  compare_as_number_otherwise_string(42.0, 42) -> equivalent
   *  compare_as_number_otherwise_string(42.0, "42") -> equivalent
   *  compare_as_number_otherwise_string(
   *    42, std::vector<double>({42.0})) -> equivalent
   *  compare_as_number_otherwise_string(NaN, NaN, false) -> unordered
   *  compare_as_number_otherwise_string(NaN, NaN, true) -> equivalent
   *  compare_as_number_otherwise_string(NaN, "", false) -> unordered
   *  compare_as_number_otherwise_string(NaN, "", true) -> equivalent
   *    because NaN is assimilated to ""
   *  compare_as_number_otherwise_string(NaN, "nan", false) -> unordered
   *  compare_as_number_otherwise_string(NaN, "nan", true) -> equivalent
   *    because "nan" string is convertible to NaN double
   *    (so, yes, NaN is equivalent to both "" and "nan" (or "NaN") when
   *    pretend_null_or_nan_is_empty is true)
   *  compare_as_number_otherwise_string(
   *    42, QDateTime::from...("1970-01-01T00:00:00,042Z")) -> equivalent
   */
  [[nodiscard]] static std::partial_ordering compare_as_number_otherwise_string(
      const TypedValue &a, const TypedValue &b,
      bool pretend_null_or_nan_is_empty = false);
  /** highly depends on contained type
   *  - TypedValues of different types are always !=
   *  - TypedValues of same type rely on their type operator == (implies that
   *    for doubles NaN != NaN)
   *  - operator == does not rely on <=>
   */
  [[nodiscard]] inline bool operator==(const TypedValue &other) const {
    return !!*this && type() == other.type() && value() == other.value(); }
  [[nodiscard]] friend inline bool operator==(
      const TypedValue &tv, const Utf8String &o) {
    return tv.type() == Utf8 && tv.utf8() == o; }
  [[nodiscard]] friend inline bool operator==(
      const Utf8String &o, const TypedValue &tv) {
    return tv.type() == Utf8 && tv.utf8() == o; }
  [[nodiscard]] friend inline bool operator==(
      const TypedValue &tv, const Entity &o) {
    return tv.type() == Entity8 && tv.entity8() == o; }
  [[nodiscard]] friend inline bool operator==(
      const Entity &o, const TypedValue &tv) {
    return tv.type() == Entity8 && tv.entity8() == o; }

  // null values and types ////////////////////////////////////////////////////
  /** is null operator
   *  - a null TypedValue is null (a.k.a. TypedValue() or {})
   *  - a TypedValue containing a null value is null, which only applies for
   *    types that supports null semantics like QByteArray, Utf8String,
   *    QRegularExpression... or an embedded QVariant
   *    null semantics apply only when null can be distinguished from 0 or empty
   *    (e.g. an empty Utf8String a.k.a. "" is different from a null one a.k.a.
   *    {}), not when they cannot (e.g. QPointF().isNull() is true when x and y
   *    are both == 0.0 so QPointF does not complies with null semantics and
   *    a TypedValue containing a QPointF() is never null)
   *  - a TypedValue containing a NaN double is null, you can use
   *    std::isnan() to check for it (an overload for TypedValue is defined
   *    below so that std::isnan(TypedValue&) will be true only for a TypedValue
   *    containing a floating number and this "number" is a NaN value)
   *  - infinity is not null
   *  - in all other cases a TypedValue is not null (incl. e.g. empty vectors)
   */
  inline bool operator!() const { return !!d ? !*d : true; }
  [[deprecated]] inline bool isValid() const { return !operator!(); }
  [[deprecated]] inline bool isNull() const { return operator!(); }
  [[nodiscard]] inline Type type() const { return value().type(); }
  /** convert a type enum/int code (Signed8...) into an ETV code ("i8"...) */
  [[nodiscard]] static Utf8String typecode(Type type);
  [[nodiscard]] inline Utf8String typecode() const { return typecode(type()); }
  [[nodiscard]] static Type from_typecode(const Utf8String &typecode);

  // regular data access //////////////////////////////////////////////////////
  TypedValue(bool b) : d(new Bool1Value{b}) { }
  TypedValue(uint64_t u) : d(new Unsigned8Value{u}) { }
  TypedValue(int64_t i) : d(new Signed8Value{i}) { }
  TypedValue(p6::integral_or_enum auto i) {
    if constexpr (std::is_signed_v<decltype(i)>)
      d = new Signed8Value{i};
    else
      d = new Unsigned8Value{i};
  }
  TypedValue(double d) : d(new Float8Value{d}) { }
  TypedValue(std::floating_point auto f) : d(new Float8Value{f}) { }
  TypedValue(Entity e) : d(new EntityValue{e}) { }
  TypedValue(const QByteArray &bytes) : d(new BytesValue{bytes}) { }
  TypedValue(const Utf8String &utf8) : d(new Utf8Value{utf8}) { }
  TypedValue(const QDateTime &ts) : d(new Timestamp8Value{ts}) { }
  TypedValue(const QRegularExpression &re) : d(new RegexpValue{re}) { }
  TypedValue(const std::vector<Entity> &v) : d(new EntityVectorValue{v}) { }
  TypedValue(std::vector<Entity> &&v) : d(new EntityVectorValue{v}) { }
  template <typename T>
  requires std::ranges::input_range<T>
  && std::same_as<std::decay_t<std::ranges::range_reference_t<T>>,Entity>
  inline TypedValue(T values)
    : TypedValue(std::vector<Entity>(values.begin(), values.end())) { }
  TypedValue(const std::vector<double> &v) : d(new FVectorValue{v}) { }
  TypedValue(std::vector<double> &&v) : d(new FVectorValue{v}) { }
  template <typename T>
  requires std::ranges::input_range<T>
  && std::same_as<std::decay_t<std::ranges::range_reference_t<T>>,double>
  inline TypedValue(T values)
    : TypedValue(std::vector<double>(values.begin(), values.end())) { }
  TypedValue(const QPointF &p) : d(new PointFValue{p}) { }
  TypedValue(const QSizeF &p) : d(new SizeFValue{p}) { }
  TypedValue(const QRectF &p) : d(new RectFValue{p}) { }
  TypedValue(const QLineF &p) : d(new LineFValue{p}) { }
  TypedValue(const std::vector<QPointF> &v) : d(new PointFVectorValue{v}) { }
  TypedValue(std::vector<QPointF> &&v) : d(new PointFVectorValue{v}) { }
  template <typename T>
  requires std::ranges::input_range<T>
  && std::same_as<std::decay_t<std::ranges::range_reference_t<T>>,QPointF>
  inline TypedValue(T values)
    : TypedValue(std::vector<QPointF>(values.begin(), values.end())) { }
  /** return contained unsigned value if any, or 0 */
  [[nodiscard]] inline uint64_t unsigned8() const {
    return value().unsigned8(); }
  /** return contained Entity value if any, or {} */
  [[nodiscard]] inline Entity entity8() const { return value().entity8(); }
  /** return contained signed value if any, or 0 */
  [[nodiscard]] inline int64_t signed8() const { return value().signed8(); }
  /** return contained bool value if any, or false */
  [[nodiscard]] inline bool bool1() const { return value().bool1(); }
  /** return contained floating value if any, or 0.0 */
  [[nodiscard]] inline double float8() const { return value().float8(); }
  /** return contained binary value if any, or {} */
  [[nodiscard]] inline QByteArray bytes() const { return value().bytes(); }
  /** return contained text value if any, or {} */
  [[nodiscard]] inline Utf8String utf8() const { return value().utf8(); }
  [[nodiscard]] QDateTime timestamp8() const;
  [[nodiscard]] QRegularExpression regexp() const;
  [[nodiscard]] inline const std::vector<double> &fvector() const {
    return value().fvector(); }
  [[nodiscard]] QPointF pointf() const;
  [[nodiscard]] QSizeF sizef() const;
  [[nodiscard]] QRectF rectf() const;
  [[nodiscard]] QLineF linef() const;
  [[nodiscard]] inline const std::vector<QPointF> &pointfvector() const {
    return value().pointfvector(); }

  // conversions among regular data types /////////////////////////////////////
  /** return value converted to unsigned as far as possible */
  [[nodiscard]] inline uint64_t as_unsigned8(
      uint64_t def = 0, bool *ok = 0) const {
    return value().as_unsigned8(def, ok); }
  [[nodiscard]] inline uint64_t as_unsigned8(bool *ok) const {
    return value().as_unsigned8(0, ok); }
  /** return value converted to signed as far as possible */
  [[nodiscard]] inline int64_t as_signed8(
      int64_t def = 0, bool *ok = 0) const {
    return value().as_signed8(def, ok); }
  [[nodiscard]] inline int64_t as_signed8(bool *ok) const {
    return value().as_signed8(0, ok); }
  /** return value converted to floating as far as possible */
  [[nodiscard]] inline double as_float8(
      double def = 0.0, bool *ok = 0) const {
    return value().as_float8(def, ok); }
  [[nodiscard]] inline double as_float8(bool *ok) const {
    return value().as_float8(0.0, ok); }
  /** return value converted to bool as far as possible */
  [[nodiscard]] inline bool as_bool1(
      bool def = false, bool *ok = 0) const {
    return value().as_bool1(def, ok); }
  [[nodiscard]] inline bool as_bool1(bool *ok) const {
    return value().as_bool1(false, ok); }
  /** return value converted to text as far as possible */
  [[nodiscard]] inline Utf8String as_utf8(
      const Utf8String &def = {}, bool *ok = 0) const {
    return value().as_utf8(def, ok); }
  [[nodiscard]] inline Utf8String as_utf8(bool *ok) const {
    return value().as_utf8({}, ok); }
  [[nodiscard]] explicit operator Utf8String() const { return as_utf8(); }
  [[nodiscard]] QDateTime as_timestamp8(const QDateTime &def, bool *ok) const;
  [[nodiscard]] QRegularExpression as_regexp(
      const QRegularExpression &def, bool *ok) const;
  [[nodiscard]] inline const std::vector<Entity> as_entityvector(
      const std::vector<Entity> &def, bool *ok) const {
    return value().as_entityvector(def, ok); }
  [[nodiscard]] inline const std::vector<double> as_fvector(
      const std::vector<double> &def, bool *ok) const {
    return value().as_fvector(def, ok); }
  [[nodiscard]] const std::vector<QPointF> as_pointfvector(
      const std::vector<QPointF> &def, bool *ok) const;
  [[nodiscard]] QPointF as_pointf(const QPointF &def, bool *ok) const;
  [[nodiscard]] QSizeF as_sizef(const QSizeF &def, bool *ok) const;
  [[nodiscard]] QRectF as_rectf(const QRectF &def, bool *ok) const;
  [[nodiscard]] QLineF as_linef(const QLineF &def, bool *ok) const;

  // conversions to and from other data types /////////////////////////////////
  inline TypedValue(const void *ptr) : TypedValue((uint64_t)ptr) { }
  inline TypedValue(const char *utf8, qsizetype len = -1)
    : TypedValue(Utf8String{utf8, len}) { }
  inline TypedValue(const signed char *utf8, qsizetype len = -1)
    : TypedValue(Utf8String{utf8, len}) { }
  inline TypedValue(const unsigned char *utf8, qsizetype len = -1)
    : TypedValue(Utf8String{utf8, len}) { }
  inline TypedValue(const char8_t *utf8, qsizetype len = -1)
    : TypedValue(Utf8String{utf8, len}) { }
  explicit inline TypedValue(char c) : TypedValue(Utf8String{c}) { }
  explicit inline TypedValue(signed char c) : TypedValue(Utf8String{c}) { }
  explicit inline TypedValue(unsigned char c) : TypedValue(Utf8String{c}) { }
  explicit inline TypedValue(char8_t c) : TypedValue(Utf8String{c}) { }
  explicit inline TypedValue(char32_t c) : TypedValue(Utf8String{c}) { }
  TypedValue(const QString &utf16);
  explicit TypedValue(const QVariant &v);
  /** create a TypedValue with best matching number type from a text
   *  representation.
   *
   *  examples:
   *  "1.2" or "1.0" or "1e3" or "1.2k" or "-1.0" or "100000.0P" -> Float8
   *  "1" or "1k" or "0xe" -> Unsigned8
   *  "-1" or "-1G" -> Signed8
   *  "true" or "false" -> Bool1
   *  "" or "foo" or "100000P" -> Null
   *
   *  if use_integral_types_with_floating_notation_if_possible is true, then:
   *  "1.2" or "100000.0P" -> Float8
   *  "1" or "1k" or "0xe" or "1.0" or "1e3" or "1.2k"  -> Unsigned8
   *  "-1" or "-1G" or "-1.0" -> Signed8
   *  "true" or "false" -> Bool1
   *  "" or "foo" or "100000P" -> Null
   */
  static TypedValue best_number_type(
      const Utf8String &utf8,
      bool use_integral_types_despite_floating_notation_if_possible = false);
  /** conversion helper for QString */
  [[nodiscard]] QString as_utf16() const;
  [[deprecated]] QString toString() const;
  [[nodiscard]] explicit operator QString() const { return as_utf16(); }
  /** conversion for any number type, applicable domain is checked for
    * smaller datatypes (e.g. as_number<short>(TypedValue(4e9)) will return
    * def and set *ok to false because the result fits in an int64_t but not
    * in an int16_t) */
  template <p6::arithmetic T>
  [[nodiscard]] inline T as_number(const T &def = {}, bool *ok = nullptr) const{
    if constexpr (std::same_as<T, bool>) {
      return value().as_bool1(def, ok);
    } else if constexpr (std::is_floating_point_v<T>) {
      bool ok1;
      double d = value().as_float8(def, &ok1);
      if (ok1 && std::numeric_limits<T>::min() <= d
          && d <= std::numeric_limits<T>::max()) {
        if (ok) *ok = true;
        return d;
      }
      if (ok) *ok = false;
      return def;
    } else if constexpr (std::is_signed_v<T>) {
      bool ok1;
      int64_t i = value().as_signed8(def, &ok1);
      if (ok1 && std::numeric_limits<T>::min() <= i
          && i <= std::numeric_limits<T>::max()) {
        if (ok) *ok = true;
        return i;
      }
      if (ok) *ok = false;
      return def;
    } else {
      bool ok1;
      uint64_t u = value().as_unsigned8(def, &ok1);
      if (ok1 && std::numeric_limits<T>::min() <= u
          && u <= std::numeric_limits<T>::max()) {
        if (ok) *ok = true;
        return u;
      }
      if (ok) *ok = false;
      return def;
    }
  }
  template <p6::arithmetic T>
  [[nodiscard]] inline T as_number(bool *ok) const{
    return as_number(T{}, ok);
  }
  [[nodiscard]] static TypedValue from_qvariant(const QVariant &v);
  [[nodiscard]] QVariant as_qvariant() const;
  [[nodiscard]] explicit operator QVariant() const { return as_qvariant(); }

  // external typed value format //////////////////////////////////////////////
  /** Convert to External Typed Value format, e.g. "text" or i8{-42} */
  [[nodiscard]] Utf8String as_etv() const;
  /** Convert from External Typed TypedValue format, e.g. "text" or i8{-42} */
  /** create a TypedValue from e.g. "i8{-3.14}" or "utf8{foo}" */
  [[nodiscard]] static TypedValue from_etv(const Utf8String &etv);
  /** create a TypedValue from e.g. (Signed8, "-3.14") or (Utf8, "foo") */
  [[nodiscard]] static TypedValue from_etv(
      Type type, const Utf8String &unquoted_etv);

  // operations ///////////////////////////////////////////////////////////////
  /** return bitwise or between this and other, don't change this
   *  return {} if this or other can't be converted to unsigned8 and
   *  pretend_null_or_invalid_as_zero is false */
  template <bool pretend_null_or_invalid_as_zero = false>
  [[nodiscard]] TypedValue bitwise_or(const TypedValue &other) const {
    bool ok1, ok2;
    uint64_t x = as_unsigned8(0, &ok1), y = other.as_unsigned8(0, &ok2);
    if constexpr (pretend_null_or_invalid_as_zero)
      return x|y;
    if (ok1 && ok2)
      return x|y;
    return {};
  }
  /** return bitwise and between this and other, don't change this
   *  return {} if this or other can't be converted to unsigned8 and
   *  pretend_null_or_invalid_as_zero is false */
  template <bool pretend_null_or_invalid_as_zero = false>
  [[nodiscard]] TypedValue bitwise_and(const TypedValue &other) const {
    bool ok1, ok2;
    uint64_t x = as_unsigned8(0, &ok1), y = other.as_unsigned8(0, &ok2);
    if constexpr (pretend_null_or_invalid_as_zero)
      return x&y;
    if (ok1 && ok2)
      return x&y;
    return {};
  }
  /** return bitwise xor between this and other, don't change this
   *  return {} if this or other can't be converted to unsigned8 and
   *  pretend_null_or_invalid_as_zero is false */
  template <bool pretend_null_or_invalid_as_zero = false>
  [[nodiscard]] TypedValue bitwise_xor(const TypedValue &other) const {
    bool ok1, ok2;
    uint64_t x = as_unsigned8(0, &ok1), y = other.as_unsigned8(0, &ok2);
    if constexpr (pretend_null_or_invalid_as_zero)
      return x^y;
    if (ok1 && ok2)
      return x^y;
    return {};
  }
  /** return string concatenation between this and other, don't change this
   *  return {} if this or other can't be converted to utf8 and
   *  pretend_null_or_invalid_as_zero is false */
  template <bool pretend_null_or_invalid_as_empty = false>
  [[nodiscard]] TypedValue concat(const TypedValue &other) const {
    bool ok1, ok2;
    Utf8String x = as_utf8(&ok1), y = other.as_utf8(&ok2);
    if constexpr (pretend_null_or_invalid_as_empty)
      return x+y;
    if (ok1 && ok2)
      return x+y;
    return {};
  }
  /** return a+b using best suited arithmetic type, checking integer overflow
   *  return {} if an operand can't be converted to a number type
   *  return {} if both operands are integers and the operation overflows */
  [[nodiscard]] static TypedValue add(TypedValue a, TypedValue b);
  /** return a-b using best suited arithmetic type, checking integer overflow
   *  return {} if an operand can't be converted to a number type
   *  return {} if both operands are integers and the operation overflows */
  [[nodiscard]] static TypedValue sub(TypedValue a, TypedValue b);
  /** return a*b using best suited arithmetic type, checking integer overflow
   *  return {} if an operand can't be converted to a number type
   *  return {} if both operands are integers and the operation overflows */
  [[nodiscard]] static TypedValue mul(TypedValue a, TypedValue b);
  /** return a/b using best suited arithmetic type
   *  if both operands are integral types, use euclidean division and return
   *  integer quotient rather than floating point division
   *  return {} if an operand can't be converted to a number type or b == 0 */
  [[nodiscard]] static TypedValue div(TypedValue a, TypedValue b);
  /** return remainder between a and b using best suited arithmetic type
   *  return {} if an operand can't be converted to a number type or b == 0 */
  [[nodiscard]] static TypedValue mod(TypedValue a, TypedValue b);

private:
  QSharedDataPointer<Value> d;
  inline TypedValue(Value *value) : d(value) {}
  /** safe access to a Value object reference even if d == 0 */
  inline const Value &value() const { return !!d ? *d : NullValue::_nullvalue; }
};
static_assert(sizeof(TypedValue)==8);

/** Null coalesce operator */
inline const TypedValue &operator||(const TypedValue &x, const TypedValue &y) {
  return !!x ? x : y; }

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, TypedValue o);

log::LogHelper LIBP6CORESHARED_EXPORT operator<<(
    log::LogHelper lh, TypedValue o);

} // p6

namespace std {

/** always return false if tv is not a floating number */
inline bool isnan(const p6::TypedValue &tv) {
  return tv.type() == p6::TypedValue::Float8 && isnan(tv.float8());
}

/** always return false if tv is not a floating number */
inline bool isinf(const p6::TypedValue &tv) {
  return tv.type() == p6::TypedValue::Float8 && isinf(tv.float8());
}

/** always return false if tv is not a floating number */
inline bool isfinite(const p6::TypedValue &tv) {
  return tv.type() == p6::TypedValue::Float8 && isfinite(tv.float8());
}

} // std

#endif // TYPEDVALUE_H
