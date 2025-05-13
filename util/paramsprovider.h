/* Copyright 2013-2025 Hallowyn, Gregoire Barbier and others.
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
class Utf8StringList;

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
  /** Core method giving access to param values.
   *
   *  MUST be reimplemented and provide param values according to keys in a
   *  manner consistent with paramKeys() result: a key returned by paramKeys()
   *  SHOULD always provide a value through paramRawValue() and any regular
   *  key associated to a value SHOULD be listed by paramKeys(), with the
   *  exception of keys behaving like a function (if for instance the
   *  implementation supports a !calc:2+2 key function, every possible math
   *  formula can't be returned by paramKeys())
   *
   *  MUST check that its scope is compatible with context scope, for
   *  instance by starting the method with this code:
   *    if(!context.hasScopeOrNone(paramScope()))
   *      return {};
   *
   *  Default: always return def. */
  [[nodiscard]] virtual TypedValue paramRawValue(
      const Utf8String &key, const TypedValue &def = {},
      const EvalContext &context = {}) const = 0;
  /** Convenience method. */
  [[nodiscard]] inline TypedValue paramRawValue(
      const Utf8String &key, const EvalContext &context) const {
    return paramRawValue(key, {}, context); }
  /** Convenience method */
  [[nodiscard]] inline Utf8String paramRawUtf8(
      const Utf8String &key, const Utf8String &def = {},
      const EvalContext &context = {}) const {
    return Utf8String(paramRawValue(key, def, context)); }
  /** Convenience method. */
  [[nodiscard]] inline Utf8String paramRawUtf8(
      const Utf8String &key, const EvalContext &context) const {
    return paramRawUtf8(key, {}, context); }
  /** Convenience method */
  [[nodiscard]] inline QString paramRawUtf16(
      const Utf8String &key, const QString &def = {},
      const EvalContext &context = {}) const {
    auto v = paramRawValue(key, {}, context);
    return !!v ? v.as_utf16() : def; }
  /** Convenience method. */
  [[nodiscard]] inline QString paramRawUtf16(
      const Utf8String &key, const EvalContext &context) const {
    return paramRawUtf8(key, {}, context); }
  /** Return list of param keys. Maybe expensive depending on implementation,
   *  call only if/when needed. Called by paramSnapshot().
   *
   *  MUST be reimplemented in a manner to be consistent with paramRawValue(),
   *  see above. Any param which key is not returned by paramKeys() will be
   *  hidden to any autodiscover/gui code and absent of snapshots.
   *
   *  Default: return {} which, among other things, makes snapshots useless.
   *  @see paramRawValue()
   *  @see paramSnaphsot() */
  [[nodiscard]] virtual Utf8StringSet paramKeys(
      const EvalContext &context = {}) const = 0;
  /** Return true if key is set.
   *  CAN be overriden for performance.
   *  Default: call paramRawValue(key).isValid() */
  [[nodiscard]] virtual bool paramContains(
      const Utf8String &key, const EvalContext &context = {}) const;

  // %-evaluated values
  /** Return a parameter value.
   *  @param key key of parameter to take value of, if it contains a [scope]
   *         prefix, use it instead of the one in context
   *  @param def default value
   *  @param context %-evaluation context, params providers in the context
   *         will be prepended with this before evaluation */
  [[nodiscard]] TypedValue paramValue(
      const Utf8String &key, const TypedValue &def = {},
      const EvalContext &context = {}) const;
  /** Convenience method */
  [[nodiscard]] inline TypedValue paramValue(
      const Utf8String &key, const ParamsProvider *context) const {
    return paramValue(key, {}, context); }

  // utf8 %-evaluated values
  /** Convenience method */
  [[nodiscard]] inline Utf8String paramUtf8(
      const Utf8String &key, const Utf8String &def = {},
      const EvalContext &context = {}) const {
    return Utf8String(paramValue(key, def, context)); }
  /** Convenience method */
  [[nodiscard]] inline Utf8String paramUtf8(
      const Utf8String &key, const EvalContext &context) const {
    return paramUtf8(key, {}, context); }

  // utf16 %-evaluated values
  /** Convenience method */
  [[nodiscard]] inline QString paramUtf16(
      const Utf8String &key, const QString &def = {},
      const EvalContext &context = {}) const {
    auto v = paramValue(key, {}, context);
    return !!v ? v.as_utf16() : def; }
  /** Convenience method */
  [[nodiscard]] inline QString paramUtf16(
      const Utf8String &key, const EvalContext &context) const {
    return paramUtf16(key, {}, context); }

  // number %-evaluated values
  /** Evaluate and then convert result to a number type (i.e. floating, integer
   *  or bool).
   *
   *  If the raw value is a TypedValue with a number type (e.g. Float8)
   *  it will be passed through as is, otherwise Utf8String::toNumber<> is used
   *  to do the conversion (so base autodection (0x for hexa...), metric
   *  suffixes (kMPpu...) and casual suffixes (kbm...) are supported).
   *
   *  Most users want rather to call one of the convenience method without
   *  alreadyEvaluated param.
   *
   *  @see Utf8String::toNumber<>
   *  @param context is an evaluation context, uses this if null
   *  @param alreadyEvaluated used for loop detections, must not be null */
  template <p6::arithmetic T>
  [[nodiscard]] T paramNumber(
      const Utf8String &key, const T &def = {},
      const EvalContext &context = {}) const {
    return paramValue(key, {}, context).as_number<T>(def); }
  /** Convenience methods */
  template <p6::arithmetic T>
  [[nodiscard]] T paramNumber(
      const Utf8String &key, const EvalContext &context) const {
    return paramValue(key, {}, context).as_number<T>(); }
  /** Convenience method: call paramNumber<bool> */
  [[nodiscard]] bool paramBool(
      const Utf8String &key, bool def = false,
      const EvalContext &context = {}) const {
    return paramValue(key, {}, context).as_number<bool>(def); }
  /** Convenience method: call paramNumber<bool> */
  [[nodiscard]] bool paramBool(
      const Utf8String &key, const EvalContext &context) const {
    return paramValue(key, {}, context).as_number<bool>(); }

  // scope
  /** Return default scope, that is more or less a name or type for this
   *  ParamsProvider. e.g. "env", "customer:Customer123", "root", etc.
   *  Can be null or empty (which is the same).  */
  [[nodiscard]] virtual Utf8String paramScope() const;

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
  [[nodiscard]] virtual ParamSet paramSnapshot() const;

#if PARAMSET_SUPPORTS_DONTINHERIT
  // temporary partial backward compatibility with old API
  // it's partial because for some method it's broken about inherit
  [[deprecated("use paramValue() or paramUtf8() instead")]]
  Utf8String evaluate(
      const Utf8String &key, const ParamsProvider *context = 0,
      Utf8StringSet *ae = 0) const;
  [[deprecated("use paramValue() or paramUtf8() instead")]]
  Utf8String evaluate(const Utf8String &key, bool inherit,
      const ParamsProvider *context = 0, Utf8StringSet *ae = 0) const;
  [[deprecated("use paramUtf8List() instead")]]
  Utf8StringList splitAndEvaluate(
      const Utf8String &key, const Utf8String &separators, bool fake_inherit,
      const ParamsProvider *context = 0, Utf8StringSet *ae = 0) const;
#endif
};

/** Very simple ParamsProvider implementation, based on a
 *  Utf8String -> TypedValue map. */

class LIBP6CORESHARED_EXPORT SimpleParamsProvider : public ParamsProvider {
  QMap<Utf8String,TypedValue> _params;
  Utf8String _scope;

public:
  [[deprecated("use ParamSet instead")]]
  SimpleParamsProvider(
      const QMap<Utf8String,TypedValue> &params = {},
      const Utf8String &scope = {})
    : _params(params), _scope(scope) { }
  [[deprecated("use ParamSet instead")]]
  SimpleParamsProvider(
      std::initializer_list<std::pair<Utf8String,TypedValue>> list,
      const Utf8String &scope = {}) : _scope(scope) {
    for (auto &&p : list)
      _params.insert(p.first, p.second);
  }
  [[nodiscard]] TypedValue paramRawValue(
      const Utf8String &key, const TypedValue &def = {},
      const EvalContext &context = {}) const override;
  [[nodiscard]] Utf8StringSet paramKeys(
      const EvalContext &context = {}) const override;
  [[nodiscard]] bool paramContains(
      const Utf8String &key, const EvalContext &context = {}) const override;
  [[nodiscard]] Utf8String paramScope() const override;
  SimpleParamsProvider &setScope(const Utf8String &scope) {
    _scope = scope; return *this; }
  [[nodiscard]] const QMap<Utf8String,TypedValue> toMap() const {
    return _params; }
};

#endif // PARAMSPROVIDER_H
