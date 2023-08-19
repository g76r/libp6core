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

class ParamsProvider;
class Utf8StringSet;

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
class PercentEvaluator {
public:

  struct ScopedValue {
    Utf8String scope;
    QVariant value;
    operator const QVariant &() const { return value; }
    bool isValid() const { return value.isValid(); }
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
   *  @param context is an evaluation context, can be null (only contextless
   *         function like %=date will be available for evaluation)
   *  @param alreadyEvaluated used for loop detections, must not be null */
  [[nodiscard]] static const ScopedValue eval(
      const Utf8String &expr, const ParamsProvider *context = 0);
  /** Lower-level reentrant version of the method. */
  [[nodiscard]] static inline const ScopedValue eval(
      const Utf8String &expr, const ParamsProvider *context,
      Utf8StringSet *alreadyEvaluated) {
    auto begin = expr.constData();
    return eval(begin, begin+expr.size(), context, alreadyEvaluated);
  }
  /** Even lower-level reentrant version of the method with char* params. */
  [[nodiscard]] static const ScopedValue eval(
      const char *expr, const char *end, const ParamsProvider *context,
      Utf8StringSet *alreadyEvaluated);
  /** Low-level %-less key evaluation.
   *
   *  Kind equivalent to eval("%{"+key+"}") without the overhead.
   *  Used for instance by MathExpr where unquoted tokens are considered
   *  variable so are to be evaluated.
   *
   *  eval_key("foo") -> value of param foo in the context
   *  eval_key("=date", 0) -> current date time without any context
   *  eval_key("'foo") -> "foo" because ' escapes all the remaining
   *
   *  @param key can begin with a scope specifier, e.g. "[bar]foo"
   *  @param context is an evaluation context, can be null (only contextless
   *         function like %=date will be available for evaluation)
   *  @param alreadyEvaluated used for loop detections, must not be null */
  [[nodiscard]] static const ScopedValue eval_key(
      const Utf8String &key, const ParamsProvider *context,
      Utf8StringSet *already_evaluated);

  // data conversion
  /** Evaluate and then convert result to utf8 text.
   *
   *  @param context is an evaluation context, can be null (only contextless
   *         function like %=date will be available for evaluation)
   *  @param alreadyEvaluated used for loop detections, must not be null */
  [[nodiscard]] inline static Utf8String eval_utf8(
      const Utf8String &expr, const Utf8String &def = {},
      const ParamsProvider *context = 0) {
    QVariant v = eval(expr, context);
    return v.isValid() ? Utf8String(v) : def;
  }
  /** Lower-level reentrant version of the method. */
  [[nodiscard]] inline static Utf8String eval_utf8(
      const Utf8String &expr, const ParamsProvider *context,
      Utf8StringSet *alreadyEvaluated, const Utf8String &def = {}) {
    QVariant v = eval(expr, context, alreadyEvaluated);
    return v.isValid() ? Utf8String(v) : def;
  }
  /** Evaluate and then convert result to utf16 text.
   *
   *  @param context is an evaluation context, can be null (only contextless
   *         function like %=date will be available for evaluation)
   *  @param alreadyEvaluated used for loop detections, must not be null */
  [[nodiscard]] inline static QString eval_string(
      const Utf8String &expr, const QString &def = {},
      const ParamsProvider *context = 0) {
    QVariant v = eval(expr, context);
    return v.isValid() ? v.toString() : def;
  }
  /** Lower-level reentrant version of the method. */
  [[nodiscard]] inline static QString eval_string(
      const Utf8String &expr, const ParamsProvider *context,
      Utf8StringSet *alreadyEvaluated, const QString &def = {}) {
    QVariant v = eval(expr, context, alreadyEvaluated);
    return v.isValid() ? v.toString() : def;
  }
  /** Evaluate and then convert result to number (i.e. floating, integer or
   *  boolean).
   *
   *  If the expr is a QVariant with a number type (e.g. QMetaType::Double)
   *  it will be passed through as is, otherwise Utf8String::toNumber<> is used
   *  to do the conversion (so base autodection and metric (kMPpu...) and casual
   *  (kbm...) suffixes are supported).
   *
   *  @see Utf8String::toNumber<>
   *  @param context is an evaluation context, can be null (only contextless
   *         function like %=date will be available for evaluation)
   *  @param alreadyEvaluated used for loop detections, must not be null */
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  [[nodiscard]] inline static T eval_number(
      const Utf8String &expr, const T &def = {},
      const ParamsProvider *context = 0, bool *ok = nullptr)  {
    QVariant v = eval(expr, context);
    auto mtid = v.metaType().id();
    if (!v.canConvert<T>() || mtid == QMetaType::QString
        || mtid == QMetaType::QByteArray)
      return Utf8String(v).toNumber<T>(ok, def);
    if (ok)
      *ok = true;
    return v.value<T>();
  }
  /** Lower-level reentrant version of the methods. */
  template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
  [[nodiscard]] inline static T eval_number(
      const Utf8String &expr, const ParamsProvider *context,
      Utf8StringSet *alreadyEvaluated, const T &def = {},
      bool *ok = nullptr) {
      QVariant v = eval(expr, context, alreadyEvaluated);
      auto mtid = v.metaType().id();
      if (!v.canConvert<T>() || mtid == QMetaType::QString
          || mtid == QMetaType::QByteArray)
        return Utf8String(v).toNumber<T>(ok, def);
      if (ok)
        *ok = true;
      return v.value<T>();
    }

  // escape and matching patterns
  /** Escape all characters in string so that they no longer have special
   * meaning for evaluate() and splitAndEvaluate() methods.
   * That is: replace % with %% within the string. */
  [[nodiscard]] static inline QString escape(QString string) {
    return string.isNull() ? string : string.replace('%', u"%%"_s); }
  /** Escape all characters in string so that they no longer have special
   * meaning for evaluate() and splitAndEvaluate() methods.
   * That is: replace % with %% within the string. */
  [[nodiscard]] static inline Utf8String escape(Utf8String utf8) {
    return utf8.isNull() ? utf8 : utf8.replace('%', "%%"_u8); }
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

#endif // PERCENTEVALUATOR_H
