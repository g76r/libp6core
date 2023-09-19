/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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
#ifndef PERCENTEVALUATOR_H
#define PERCENTEVALUATOR_H

#include "utf8string.h"
#include "utf8stringset.h"
#include <set>

class ParamsProvider;
class Utf8StringSet;
class QDebug;
class LogHelper;

/** Evaluate a %-expression.
 *
 *  "foo" -> "foo"
 *  "%foo" -> value of param "foo" as provided by context->paramRawValue("foo")
 *  "%{foo!}" -> same with param "foo!": allows special chars (excepted "}")
 *               special chars are any ascii char other than a-zA-Z0-9_
 *  "%!foo" -> value of param "!foo": one leading special char is allowed
 *  "%[bar]foo -> value of param "foo" if and only if it's in "bar" scope
 *  "%{[bar]foo!}" -> same with special chars
 *  "%=date" -> calling function =date: there are contextless functions (i.e.
 *              defined inedependently of context-provided params) and by
 *              convention their name always begin with =
 *  "%{=date:YYYY}" -> current year in local timezone, using 4 digits
 *  "%éœ§越🥨" -> value of param "éœ§越🥨": chars outside ascii are not special
 *  "%{'foo}" -> "foo": a leading quote escapes param evaluation
 *  "%'foo" -> "foo": remember, one leading special char is allowed
 *  "%%" -> "%" : % escapes itself
 *  "%{=date:%format}" -> current date using format given by "format" param
 *  "%{=left:%{input}:3}" -> 3 left most utf8 characters of param "input"
 *  "%{=left:abcdef:3}" -> "abc"
 *  "%{=left:abcde{:3}" -> invalid: unpaired {} are not supported within {}
 *
 *  @see percent_evaluation.md for more complete information
 *  @see https://gitlab.com/g76r/libp6core/-/blob/master/util/percent_evaluation.md
 *  @see ParamsProvider
 */
class LIBP6CORESHARED_EXPORT PercentEvaluator {
public:

  /** Evaluation context */
  class EvalContext {
    const ParamsProvider *_params_provider;
    Utf8StringSet _scope_filter;
    Utf8StringSet _already_evaluated_variables; // loop detection
    int _role;

  public:
    /** implicit constructor: can cast from const ParamsProvider * */
    EvalContext(
        const ParamsProvider *params_provider = nullptr,
        const Utf8String &scope_expr = {}, int role = 0)
      : _params_provider(params_provider), _role(role) {
      if (!scope_expr.isEmpty())
        setScopeFilter(scope_expr);
    }
    explicit EvalContext(const Utf8String &scope_expr, int role = 0)
      : EvalContext(nullptr, scope_expr, role) { }
    EvalContext(const EvalContext &that) = default;
    inline operator const ParamsProvider *() const { return _params_provider; }
    inline const ParamsProvider *paramsProvider() const {
      return _params_provider; }
    inline EvalContext &setParamsProvider(const ParamsProvider *params) {
      _params_provider = params; return *this; }
    LIBP6CORESHARED_EXPORT EvalContext &setScopeFilter(const Utf8String &scope_expr);
    /** has no scope === any scope is acceptable */
    inline bool hasNoScope() const { return _scope_filter.isEmpty(); }
    /** has this scope or no scope === this scope is acceptable */
    inline bool hasScopeOrNone(const Utf8String &scope) const {
      return hasNoScope() || containsScope(scope); }
    /** strictly contains this scope (sufficient but not necessary for the scope
     *  to be accepetable) */
    inline bool containsScope(const Utf8String &scope) const {
      return _scope_filter.contains(scope); }
    const Utf8StringSet &scopeFilter() const { return _scope_filter; }
    EvalContext &addVariable(const Utf8String &key) {
      _already_evaluated_variables.insert(key); return *this; }
    bool containsVariable(const Utf8String &key) const {
      return _already_evaluated_variables.contains(key); }
    int role() const { return _role; }
    LIBP6CORESHARED_EXPORT const Utf8String toUtf8() const;
  };

  PercentEvaluator() = delete;

  // %-evaluation
  /** Evaluate a %-expression.
   *
   *  About returned value type:
   *  With most possible %-expression the returned value will be a Utf8String
   *  because several fragments are concatenated to a characters string, e.g.
   *  "foo %bar baz" will be an Utf8String with value of bar converted to
   *  Utf8String, even if it was a double.
   *  On the otherhand, provided there is only one % and key expression without
   *  anything before or after, the param value will be returned as is, and so
   *  can be a QVariant of another type.
   *
   *  @param context is an evaluation context, can be empty (only contextless
   *         function like %=date will be available for evaluation) */
  [[nodiscard]] static inline const QVariant eval(
      const Utf8String &expr, const EvalContext &context = {}) {
    auto begin = expr.constData();
    return eval(begin, begin+expr.size(), context);
  }
  /** Lower level version of the method with char* params. */
  [[nodiscard]] static const QVariant eval(
      const char *expr, const char *end, const EvalContext &context = {});
  /** Low-level %-less key evaluation.
   *
   *  Kind equivalent to eval("%{"+key+"}") without the overhead.
   *  Used for instance by MathExpr where unquoted tokens are considered
   *  variable so are to be evaluated.
   *  Can also be used e.g. to evaluate the values of a regular map (not a
   *  ParamSet or to map the values of a ParamSet excluding it from the
   *  evaluating context).
   *
   *  eval_key("foo") -> value of param foo in the context
   *  eval_key("=date", 0) -> current date time without any context
   *  eval_key("'foo") -> "foo" because ' escapes all the remaining
   *
   *  @param key can begin with a scope specifier, e.g. "[bar]foo" (in which
   *         case it will override the one in context)
   *  @param context is an evaluation context, can be null (only contextless
   *         function like %=date will be available for evaluation)
   *  @param alreadyEvaluated used for loop detections, must not be null */
  [[nodiscard]] static const QVariant eval_key(
      const Utf8String &key, const EvalContext &context = {});

  // data conversion
  /** Evaluate and then convert result to utf8 text.
   *
   *  @param context is an evaluation context, can be null (only contextless
   *         function like %=date will be available for evaluation)
   *  @param alreadyEvaluated used for loop detections, must not be null */
  [[nodiscard]] inline static Utf8String eval_utf8(
      const Utf8String &expr, const Utf8String &def = {},
      const EvalContext &context = {}) {
    QVariant v = eval(expr, context);
    return v.isValid() ? Utf8String(v) : def;
  }
  [[nodiscard]] inline static Utf8String eval_utf8(
      const Utf8String &expr, const EvalContext &context) {
    return eval_utf8(expr, {}, context); }
  /** Evaluate and then convert result to utf16 text.
   *
   *  @param context is an evaluation context, can be empty (only contextless
   *         function like %=date will be available for evaluation) */
  [[nodiscard]] inline static QString eval_utf16(
      const Utf8String &expr, const QString &def = {},
      const EvalContext &context = {}) {
    QVariant v = eval(expr, context);
    return v.isValid() ? v.toString() : def;
  }
  [[nodiscard]] inline static QString eval_utf16(
      const Utf8String &expr, const EvalContext &context) {
    return eval_utf16(expr, {}, context); }
  /** Evaluate and then convert result to number (i.e. floating, integer or
   *  boolean).
   *
   *  If the expr is a QVariant with a number type (e.g. QMetaType::Double)
   *  it will be passed through as is, otherwise Utf8String::toNumber<> is used
   *  to do the conversion (so base autodection and metric (kMPpu...) and casual
   *  (kbm...) suffixes are supported).
   *
   *  @see Utf8String::toNumber<>
   *  @param context is an evaluation context, can be empty (only contextless
   *         function like %=date will be available for evaluation) */
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  [[nodiscard]] inline static T eval_number(
      const Utf8String &expr, const T &def = {},
      const EvalContext &context = {}, bool *ok = nullptr)  {
    auto v = eval(expr, context);
    if (!v.isValid()) {
      if (ok)
        *ok = false;
      return def;
    }
    auto mtid = v.metaType().id();
    // text types and types not convertible to a number are for Utf8String
    if (!v.canConvert<T>() || mtid == qMetaTypeId<Utf8String>()
        || mtid == QMetaType::QString || mtid == QMetaType::QByteArray)
      return Utf8String(v).toNumber<T>(ok, def);
    if (ok)
      *ok = true;
    return v.value<T>();
  }
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  [[nodiscard]] inline static T eval_number(
      const Utf8String &expr, const EvalContext &context, bool *ok = nullptr) {
    return eval_number<T>(expr, {}, context, ok); }

  // escape and matching patterns
  /** Escape all characters in string so that they no longer have special
   * meaning for evaluate() and splitAndEvaluate() methods.
   * That is: replace % with %% within the string. */
  [[nodiscard]] static inline QString escape(QString utf16) {
    return utf16.isNull() ? utf16 : utf16.replace('%', u"%%"_s); }
  /** Escape all characters in string so that they no longer have special
   * meaning for evaluate() and splitAndEvaluate() methods.
   * That is: replace % with %% within the string. */
  [[nodiscard]] static inline Utf8String escape(QByteArray utf8) {
    return utf8.isNull() ? utf8 : utf8.replace('%', "%%"_ba); }
  /** Convenience method */
  [[nodiscard]] static inline Utf8String escape(const QVariant &v) {
    return v.isValid() ? Utf8String(v).replace('%', "%%"_u8) : Utf8String{}; }
  /** Return a regular expression that matches any string that can result
   * in evaluation of the rawValue.
   * For instance "foo%{=date:yyyy}-%{bar}.log" is converted into some pattern
   * that can be "foo....-.*\\.log" or "foo.*-.*\\.log" (let be frank: currently
   * the second pattern is returned, not the first one, and it's likely to stay
   * this way).
   * Can be used as an input for QRegularExpression(QString) constructor. */
  [[nodiscard]] static const QString matching_regexp(
      const Utf8String &expr);

  // logging
  /** Record debug log messages when a variable evaluation is required and not
   * found.
   * Applicable to all params sets in the application (global parameter).
   * Defaults: disabled, but if "ENABLE_PERCENT_VARIABLE_NOT_FOUND_LOGGING"
   * environment variable is set to "true". */
  static void enable_variable_not_found_logging(bool enabled = true);
};

QDebug LIBP6CORESHARED_EXPORT operator<<(
    QDebug dbg, const PercentEvaluator::EvalContext &c);

LogHelper LIBP6CORESHARED_EXPORT operator<<(
    LogHelper lh, const PercentEvaluator::EvalContext &c);


#endif // PERCENTEVALUATOR_H
