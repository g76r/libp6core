/* Copyright 2013-2022 Hallowyn, Gregoire Barbier and others.
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

#include "libp6core_global.h"
#include <QVariant>
#include <QList>
#include <QSet>

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
  virtual const QVariant paramValue(
    const QString &key, const ParamsProvider *context,
    const QVariant &defaultValue, QSet<QString> *alreadyEvaluated) const = 0;
  /** Convenience method */
  inline const QVariant paramValue(
    const QString &key, const ParamsProvider *context = 0,
    const QVariant &defaultValue = QVariant()) const {
    QSet<QString> ae; return paramValue(key, context, defaultValue, &ae); }
  /** Convenience method */
  inline const QVariant paramValue(
    const QString &key, const QVariant &defaultValue,
    QSet<QString> *alreadyEvaluated) const {
    return paramValue(key, 0, defaultValue, alreadyEvaluated); }
  /** Convenience method */
  inline const QVariant paramValue(
    const QString &key, const QVariant &defaultValue) const {
    return paramValue(key, 0, defaultValue); }
  /** Convenience method */
  inline const QString paramString(
    const QString key, const ParamsProvider *context,
    const QVariant defaultValue, QSet<QString> *alreadyEvaluated) const {
    return paramValue(key, context, defaultValue, alreadyEvaluated).toString();
  }
  /** Convenience method */
  inline const QString paramString(
    const QString key, const ParamsProvider *context = 0,
    const QVariant defaultValue = QVariant()) const {
    return paramValue(key, context, defaultValue).toString(); }
  /** Convenience method */
  inline const QString paramString(
    const QString &key, const QVariant &defaultValue,
    QSet<QString> *alreadyEvaluated) const {
    return paramValue(key, defaultValue, alreadyEvaluated).toString(); }
  /** Convenience method */
  inline const QString paramString(
    const QString &key, const QVariant &defaultValue) const {
    return paramValue(key, defaultValue).toString(); }
  /** Convenience method */
  inline const QByteArray paramUtf8(
    const QString key, const ParamsProvider *context,
    const QVariant defaultValue, QSet<QString> *alreadyEvaluated) const {
    return paramValue(key, context, defaultValue, alreadyEvaluated).toString()
        .toUtf8(); }
  /** Convenience method */
  inline const QByteArray paramUtf8(
    const QString key, const ParamsProvider *context = 0,
    const QVariant defaultValue = QVariant()) const {
    return paramValue(key, context, defaultValue).toString().toUtf8(); }
  /** Convenience method */
  inline const QByteArray paramUtf8(
    const QString &key, const QVariant &defaultValue,
    QSet<QString> *alreadyEvaluated) const {
    return paramValue(key, defaultValue, alreadyEvaluated).toString().toUtf8(); }
  /** Convenience method */
  inline const QByteArray paramUtf8(
    const QString &key, const QVariant &defaultValue) const {
    return paramValue(key, defaultValue).toString().toUtf8(); }
  virtual const QSet<QString> keys() const = 0;
  /** Singleton wrapper to environment variables */
  static ParamsProvider *environment() { return _environment; }
  /** Singleton empty ParamsProvider */
  static ParamsProvider *empty() { return _empty; }
  /** take an key-values snapshot that no longer depend on ParamsProvider* not
   * being deleted nor on %-evaluation */
  virtual const ParamSet snapshot() const;
  /** evaluate a %-expression within this context.
   * short for ParamSet().evaluate(rawValue, false, this, alreadyEvaluated); */
  const QString evaluate(
    const QString &rawValue, QSet<QString> *alreadyEvaluated) const;
  inline const QString evaluate(const QString &rawValue) const {
    QSet<QString> ae; return evaluate(rawValue, &ae); }
};

/** Map of params without inheritance, evaluation or any other advanced
 *  features as compared to ParamSet: just a simple QString->QVariant map. */
class LIBP6CORESHARED_EXPORT RawParamsProvider : public ParamsProvider {
private:
  QMap<QString,QVariant> _params;

public:
  RawParamsProvider(QMap<QString,QVariant> params = QMap<QString,QVariant>())
    : _params(params) { }
  RawParamsProvider(std::initializer_list<std::pair<QString,QVariant>> list) {
    for (auto &p : list)
      _params.insert(p.first, p.second);
  }

public:
  const QVariant paramValue(
    const QString &key, const ParamsProvider *context,
    const QVariant &defaultValue, QSet<QString> *alreadyEvaluated) const override;
  const QSet<QString> keys() const override;
  const QMap<QString,QVariant> toMap() const { return _params; }
};

#endif // PARAMSPROVIDER_H
