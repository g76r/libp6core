/* Copyright 2013-2023 Hallowyn, Gregoire Barbier and others.
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
#ifndef PARAMSPROVIDER_H
#define PARAMSPROVIDER_H

#include "util/percentevaluator.h"

class ParamSet;

/** Base class for any class that may provide key/value parameters.
 *
 *  Implementations MUST override paramRawValue and paramKeys, they MAY also
 *  override some other virtual methods for performance reasons (e.g. paramValue
 *  and paramUtf8) or to provide added value (e.g. paramScope which give no
 *  scope information by default).
 *
 *  @see ParamSet
 *  @see ParamsProviderMerger
 *  @see percent_evaluation.md
 *  @see https://gitlab.com/g76r/libp6core/-/blob/master/util/percent_evaluation.md
 */
class LIBP6CORESHARED_EXPORT ParamsProvider {
  static ParamsProvider *_environment, *_empty;

public:
  using EvalContext = PercentEvaluator::EvalContext;

public:
  ParamsProvider() = default;
  virtual ~ParamsProvider() = default;

  // raw keys and values
  // FIXME comment
  /** Core method
   *  MUST be reimplemented.
   *  Default: always return def. */
  [[nodiscard]] virtual const QVariant paramRawValue(
      const Utf8String &key, const QVariant &def = {},
      const EvalContext &context = {}) const = 0;
  /** Convenience method. */
  [[nodiscard]] inline const QVariant paramRawValue(
      const Utf8String &key, const EvalContext &context) const {
    return paramRawValue(key, {}, context); }
  /** Same as paramRawValue with utf8.
   *  CAN be overriden for performance (for implementation dealing only with
   *  text, without QVariant overhead).
   *  Default: call paramRawValue() */
  [[nodiscard]] virtual const Utf8String paramRawUtf8(
      const Utf8String &key, const Utf8String &def = {},
      const EvalContext &context = {}) const;
  /** Convenience method. */
  [[nodiscard]] inline const Utf8String paramRawUtf8(
      const Utf8String &key, const EvalContext &context) const {
    return paramRawUtf8(key, {}, context); }
  /** Return list of param keys. Maybe expensive depending on implementation,
   *  call only if/when needed. Called by paramSnapshot().
   *  MUST be reimplemented.
   *  Default: return {} which, among other things, disables snapshots */
  [[nodiscard]] virtual const Utf8StringSet paramKeys(
      const EvalContext &context = {}) const = 0;
  /** Return true if key is set.
   *  CAN be overriden for performance.
   *  Default: call paramRawValue(key).isValid() */
  [[nodiscard]] virtual bool paramContains(
      const Utf8String &key, const EvalContext &context = {}) const;

  // %-evaluated values
  /** Return a parameter value.
   *  Most users want rather to call one of the convenience method without
   *  alreadyEvaluated param.
   *  @param context is an evaluation context, uses this if null
   *  @param alreadyEvaluated used for loop detections, must not be null */
  [[nodiscard]] const QVariant paramValue(
      const Utf8String &key, const QVariant &def = {},
      const EvalContext &context = {}) const;
  /** Convenience method */
  [[nodiscard]] inline const QVariant paramValue(
      const Utf8String &key, const EvalContext &context) const {
    return paramValue(key, {}, context); }

  // utf8 %-evaluated values
  /** Default implem: calls paramValue().
   *  Can be overloaded by ParamsProvider implementation that would have a
   *  more efficient way to get Utf8String than converting to and back from
   *  QVariant through paramValue(). */
  [[nodiscard]] virtual const Utf8String paramUtf8(
      const Utf8String &key, const Utf8String &def = {},
      const EvalContext &context = {}) const;
  /** Convenience method */
  [[nodiscard]] inline const Utf8String paramUtf8(
      const Utf8String &key, const EvalContext &context) const {
    return paramUtf8(key, {}, context); }

  // number %-evaluated values
  /** Evaluate and then convert result to a number type (i.e. floating, integer
   *  or bool).
   *
   *  If the raw value is a QVariant with a number type (e.g. QMetaType::Double)
   *  it will be passed through as is, otherwise Utf8String::toNumber<> is used
   *  to do the conversion (so base autodection and metric (kMPpu...) and casual
   *  (kbm...) suffixes are supported).
   *
   *  Most users want rather to call one of the convenience method without
   *  alreadyEvaluated param.
   *
   *  @see Utf8String::toNumber<>
   *  @param context is an evaluation context, uses this if null
   *  @param alreadyEvaluated used for loop detections, must not be null */
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  [[nodiscard]] const T paramNumber(
      const Utf8String &key, const T &def,
      const EvalContext &context = {}) const {
    auto v = paramRawValue(key);
    auto mtid = v.metaType().id();
    if (v.canConvert<T>() && mtid != qMetaTypeId<Utf8String>()
        && mtid != QMetaType::QString && mtid != QMetaType::QByteArray)
      return v.value<T>(); // passing QVariant trough
    auto expr = Utf8String(v);
    return PercentEvaluator::eval_number<T>(expr, def, context);
  }
  /** Convenience methods */
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  [[nodiscard]] const T paramNumber(
      const Utf8String &key, const EvalContext &context) const {
    return paramNumber<T>(key, {}, context); }

  // scope
  /** Return default scope, that is more or less a name or type for this
   *  ParamsProvider. e.g. "env", "customer:Customer123", "root", etc.
   *  Can be null or empty (which is the same).  */
  [[nodiscard]] virtual const Utf8String paramScope() const;

  // system-wide (singleton) params providers
  /** Singleton wrapper to environment variables */
  [[nodiscard]] static const ParamsProvider *environment() {
    return _environment; }
  /** Singleton empty ParamsProvider */
  [[nodiscard]] static const ParamsProvider *empty() { return _empty; }

  // snapshots
  /** Take an key-values snapshot that no longer depend on ParamsProvider* not
   *  being deleted nor on %-evaluation.
   *  This can be very expensive since it calls paramKeys() and then %-evaluate
   *  every key. */
  [[nodiscard]] virtual const ParamSet paramSnapshot() const;

  // temporary partial backward compatibility with old API
  // it's partial because for some method it's broken about inherit
  [[deprecated("evaluating with two sources (this,ctxt) is deprecated")]]
  const Utf8String evaluate(
      const Utf8String &key, const ParamsProvider *context = 0,
      Utf8StringSet *ae = 0) const;
  [[deprecated("evaluating with two sources (this,ctxt) is deprecated")]]
  const Utf8String evaluate(const Utf8String &key, bool inherit,
      const ParamsProvider *context = 0, Utf8StringSet *ae = 0) const;
  [[deprecated("split is a special case, do it elsewhere")]]
  Utf8StringList splitAndEvaluate(
      const Utf8String &key, const Utf8String &separators, bool fake_inherit,
      const ParamsProvider *context = 0, Utf8StringSet *ae = 0) const;
};

/** Very simple ParamsProvider implementation, based on Utf8String -> QVariant
 *  map. */
class LIBP6CORESHARED_EXPORT SimpleParamsProvider : public ParamsProvider {
  QMap<Utf8String,QVariant> _params;
  Utf8String _scope;

public:
  SimpleParamsProvider(
      const QMap<Utf8String,QVariant> &params = {},
      const Utf8String &scope = {})
    : _params(params), _scope(scope) { }
  SimpleParamsProvider(
      std::initializer_list<std::pair<Utf8String,QVariant>> list,
      const Utf8String &scope = {}) : _scope(scope) {
    for (auto &p : list)
      _params.insert(p.first, p.second);
  }
  [[nodiscard]] const QVariant paramRawValue(
      const Utf8String &key, const QVariant &def = {},
      const EvalContext &context = {}) const override;
  [[nodiscard]] const Utf8StringSet paramKeys(
      const EvalContext &context = {}) const override;
  [[nodiscard]] bool paramContains(
      const Utf8String &key, const EvalContext &context = {}) const override;
  [[nodiscard]] const Utf8String paramScope() const override;
  SimpleParamsProvider &setScope(const Utf8String &scope) {
    _scope = scope; return *this; }
  [[nodiscard]] const QMap<Utf8String,QVariant> toMap() const {
    return _params; }
};

#endif // PARAMSPROVIDER_H
