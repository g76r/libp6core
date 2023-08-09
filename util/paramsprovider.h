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

#include "util/utf8string.h"

class ParamSet;

/** Base class for any class that may provide key/value parameters.
 * @see ParamSet */
class LIBP6CORESHARED_EXPORT ParamsProvider {
  static ParamsProvider *_environment, *_empty;

protected:
  ParamsProvider() { }

public:
  virtual ~ParamsProvider();
  /** Return a parameter value.
    * @param context is an evaluation context */
  virtual const QVariant paramValue( // FIXME reorder args like paramUtf8
    const Utf8String &key, const ParamsProvider *context,
    const QVariant &defaultValue, Utf8StringSet *alreadyEvaluated) const = 0;
  /** Convenience method */
  const QVariant paramValue( // FIXME reorder args like paramUtf8
    const Utf8String &key, const ParamsProvider *context = 0,
    const QVariant &defaultValue = {}) const;
  /** Convenience method */
  inline const QVariant paramValue( // FIXME reorder args like paramUtf8
    const Utf8String &key, const QVariant &defaultValue,
    Utf8StringSet *alreadyEvaluated) const {
    return paramValue(key, 0, defaultValue, alreadyEvaluated); }
  /** Convenience method */
  inline const QVariant paramValue( // FIXME reorder args like paramUtf8
    const Utf8String &key, const QVariant &defaultValue) const {
    return paramValue(key, 0, defaultValue); }
  /** Convenience method */
  inline const QString paramString( // FIXME reorder args like paramUtf8
    const Utf8String &key, const ParamsProvider *context,
    const QString &defaultValue, Utf8StringSet *alreadyEvaluated) const {
    return paramValue(key, context, defaultValue, alreadyEvaluated).toString();
  }
  /** Convenience method */
  inline const QString paramString( // FIXME reorder args like paramUtf8
    const Utf8String &key, const ParamsProvider *context = 0,
    const QString &defaultValue = {}) const {
    return paramValue(key, context, defaultValue).toString(); }
  /** Convenience method */
  inline const QString paramString( // FIXME reorder args like paramUtf8
    const Utf8String &key, const QString &defaultValue,
    Utf8StringSet *alreadyEvaluated) const {
    return paramValue(key, defaultValue, alreadyEvaluated).toString(); }
  /** Convenience method */
  inline const QString paramString( // FIXME reorder args like paramUtf8
    const Utf8String &key, const QString &defaultValue) const {
    return paramValue(key, defaultValue).toString(); }
  /** Convenience method */
  inline const Utf8String paramUtf8(
    const Utf8String &key, const Utf8String &defaultValue,
    const ParamsProvider *context, Utf8StringSet *alreadyEvaluated) const {
    return Utf8String(paramValue(key, context, defaultValue,
                                 alreadyEvaluated)); }
  /** Convenience method */
  inline const Utf8String paramUtf8(
    const Utf8String &key, const Utf8String &defaultValue = {},
    const ParamsProvider *context = 0) const {
    return Utf8String(paramValue(key, context, defaultValue)); }
  /** Convenience method */
  inline const Utf8String paramUtf8(
    const Utf8String &key, const ParamsProvider *context) const {
    return Utf8String(paramValue(key, context, Utf8String{})); }
  /** Convenience method */
  inline const Utf8String paramUtf8(
    const Utf8String &key, const Utf8String &defaultValue,
    Utf8StringSet *alreadyEvaluated) const {
    return Utf8String(paramValue(key, 0, defaultValue, alreadyEvaluated)); }
  /** Convenience method */
  inline const Utf8String paramUtf8(
    const Utf8String &key, Utf8StringSet *alreadyEvaluated) const {
    return Utf8String(paramValue(key, 0, Utf8String{}, alreadyEvaluated)); }
  virtual const Utf8StringSet keys() const = 0;
  /** Return params scope, that is more or less a name or type for this
   *  ParamsProvider. e.g. "env", "customer:Customer123", "root", etc.
   */
  virtual const Utf8String paramScope() const;
  /** Left part of scope, before first occurrence of separator, or
   *  whole scope if separator is not found.
   *  Handy with ':' when dealing with SharedUiItems's qualified ids as
   *  scopes and only wanting qualifiers. */
  const Utf8String paramScopeRadix(const char separator) const {
    auto s = paramScope();
    auto i = s.indexOf(separator);
    return i >= 0 ? s.first(i) : s;
  }
  /** Singleton wrapper to environment variables */
  static ParamsProvider *environment() { return _environment; }
  /** Singleton empty ParamsProvider */
  static ParamsProvider *empty() { return _empty; }
  /** take an key-values snapshot that no longer depend on ParamsProvider* not
   * being deleted nor on %-evaluation */
  virtual const ParamSet snapshot() const;
  /** evaluate a %-expression within this context.
   * short for ParamSet().evaluate(rawValue, false, this, alreadyEvaluated); */
  const Utf8String evaluate(
      const Utf8String &rawValue, Utf8StringSet *alreadyEvaluated) const;
  const Utf8String evaluate(const Utf8String &rawValue) const;
};

/** Map of params without inheritance, evaluation or any other advanced
 *  features as compared to ParamSet: just a simple Utf8String->QVariant map. */
class LIBP6CORESHARED_EXPORT RawParamsProvider : public ParamsProvider {
private:
  QMap<Utf8String,QVariant> _params;

public:
  RawParamsProvider(
      QMap<Utf8String,QVariant> params = QMap<Utf8String,QVariant>())
    : _params(params) { }
  RawParamsProvider(
      std::initializer_list<std::pair<Utf8String,QVariant>> list) {
    for (auto &p : list)
      _params.insert(p.first, p.second);
  }

public:
  const QVariant paramValue(
      const Utf8String &key, const ParamsProvider *context,
    const QVariant &defaultValue,
      Utf8StringSet *alreadyEvaluated) const override;
  const Utf8StringSet keys() const override;
  const QMap<Utf8String,QVariant> toMap() const { return _params; }
};

#endif // PARAMSPROVIDER_H
