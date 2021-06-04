/* Copyright 2013-2018 Hallowyn, Gregoire Barbier and others.
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
#ifndef PARAMSPROVIDERMERGER_H
#define PARAMSPROVIDERMERGER_H

#include "paramset.h"

/** This class builds up several ParamsProvider into only one, chaining
 * calls to paramValue().
 * Keep in mind that it does not take ownership on referenced ParamsProvider
 * objects and that these objects must not be deleted before the last call
 * to ParamsProviderList::paramValue() or must be removed from the
 * ParamsProviderMerger before their destruction.
 * Therefore ParamsProviderMerger should only be used as a temporary object
 * around a call to some method taking a ParamsProvider as a parameter. */
class LIBP6CORESHARED_EXPORT ParamsProviderMerger : public ParamsProvider {
  class ProviderData : public QSharedData {
  public:
    const ParamsProvider *_paramsProvider;
    ParamSet _paramset;
    ProviderData(const ParamsProvider *paramsProvider = 0)
      : _paramsProvider(paramsProvider) { }
    ProviderData(ParamSet paramset)
      : _paramsProvider(0), _paramset(paramset) { }
  };

  class Provider {
  public:
    QSharedDataPointer<ProviderData> d;
    Provider(const ParamsProvider *paramsProvider = 0)
      : d(new ProviderData(paramsProvider)) { }
    Provider(ParamSet paramset) : d(new ProviderData(paramset)) { }
  };

  QList<Provider> _providers;
  ParamSet _overridingParams;
  QList<QList<Provider> > _providersStack;
  QList<ParamSet> _overridingParamsStack;

public:
  ParamsProviderMerger() { }
  ParamsProviderMerger(const ParamsProviderMerger &other)
    : ParamsProvider(), _providers(other._providers),
      _overridingParams(other._overridingParams) { }
  ParamsProviderMerger(const ParamsProvider *provider) {
    append(provider);
  }
  ParamsProviderMerger(ParamSet provider) {
    append(provider);
  }
  /** Add a ParamsProvider that will be evaluated after those already added. */
  ParamsProviderMerger &append(const ParamsProvider *provider) {
    if (provider)
      _providers.append(provider);
    return *this;
  }
  /** Add a ParamsProvider that will be evaluated after those already added. */
  ParamsProviderMerger &append(ParamSet provider) {
    if (!provider.isNull())
      _providers.append(provider);
    return *this;
  }
  /** Add a ParamsProvider that will be evaluated before those already added but
   * after parameters set with overrideParamValue(). */
  ParamsProviderMerger &prepend(const ParamsProvider *provider) {
    if (provider)
      _providers.prepend(provider);
    return *this;
  }
  /** Add a ParamsProvider that will be evaluated before those already added but
   * after parameters set with overrideParamValue(). */
  ParamsProviderMerger &prepend(ParamSet provider) {
    if (!provider.isNull())
      _providers.prepend(provider);
    return *this;
  }
  /** Convenience operator for append() */
  ParamsProviderMerger &operator()(const ParamsProvider *provider) {
    if (provider)
      _providers.append(provider);
    return *this;
  }
  /** Convenience operator for append() */
  ParamsProviderMerger &operator()(ParamSet provider) {
    if (!provider.isNull())
      _providers.append(provider);
    return *this;
  }
  /** Parameters set through overrideParamValue() will override any
   * ParamsProvider, even those prepended. */
  ParamsProviderMerger &overrideParamValue(QString key, QString value) {
    _overridingParams.setValue(key, value);
    return *this;
  }
  /** Convenience method. */
  inline ParamsProviderMerger &overrideParamValue(QString key, QVariant value) {
    return overrideParamValue(key, value.toString());
  }
  /** Convenience method. */
  inline ParamsProviderMerger &overrideParamValue(
      QString key, const char *value){
    return overrideParamValue(key, QString(value)); }
  /** Disambiguation method.
   * Avoids implicit conversion of 0 to char* */
  inline ParamsProviderMerger &overrideParamValue(QString key, int value){
    return overrideParamValue(key, QVariant::fromValue(value)); }
  /** Remove all ParamsProvider and overriding params. */
  ParamsProviderMerger &clear() {
    _providers.clear();
    _overridingParams.clear();
    return *this;
  }
   /** Saves the current state (pushes the state onto a stack). */
  void save();
  /** Restores the current state (pops a saved state off the stack). */
  void restore();
  QVariant paramValue(QString key, const ParamsProvider *context = 0,
                      QVariant defaultValue = QVariant(),
                      QSet<QString> alreadyEvaluated = QSet<QString>()
          ) const override;
  /** Give access to currently overriding params. */
  ParamSet overridingParams() const { return _overridingParams; }
};

/** RAII helper for ParamsProviderMerger save/restore.
 *
 * Calls ParamsProviderMerger::save() in constructor and
 * ParamsProviderMerger::restore() in destructor.
 *
 * Can be used that way:
 * void myfunc(ParamsProviderMerger *merger) {
 *   ParamsProviderMergerRestorer restorer(merger);
 *   // modify merger the way you want, it'll be restored when myfunc() exits
 * }
 */
class LIBP6CORESHARED_EXPORT ParamsProviderMergerRestorer {
  ParamsProviderMerger *_merger;
  ParamsProviderMergerRestorer() = delete;
  ParamsProviderMergerRestorer(const ParamsProviderMergerRestorer &) = delete;
public:
  ParamsProviderMergerRestorer(ParamsProviderMerger *merger) : _merger(merger) {
    if (_merger)
      _merger->save();
  }
  ParamsProviderMergerRestorer(ParamsProviderMerger &merger)
    : ParamsProviderMergerRestorer(&merger) { }
  ~ParamsProviderMergerRestorer() {
    if (_merger)
      _merger->restore();
  }
};

#endif // PARAMSPROVIDERMERGER_H
