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
#ifndef PARAMSPROVIDERMERGER_H
#define PARAMSPROVIDERMERGER_H

#include "paramset.h"

/** This class builds up several ParamsProvider into only one, chaining
 * calls to paramRawValue().
 * Does not take ownership on referenced ParamsProvider, these objects must
 * not be deleted before the last call to ParamsProviderList::paramValue()
 * or must be removed from the ParamsProviderMerger before their destruction.
 * Therefore ParamsProviderMerger should only be used as a temporary object
 * around a call to some method taking a ParamsProvider as a parameter. */
class LIBP6CORESHARED_EXPORT ParamsProviderMerger : public ParamsProvider {
  friend QDebug LIBP6CORESHARED_EXPORT operator<<(
      QDebug dbg, const ParamsProviderMerger *params);
  friend LogHelper LIBP6CORESHARED_EXPORT operator<<(
      LogHelper lh, const ParamsProviderMerger *merger);

  class ProviderData : public QSharedData {
  public:
    const ParamsProvider *_wild;
    ParamSet _owned;
    ProviderData(const ParamsProvider *wild = 0)
      : _wild(wild) { }
    ProviderData(const ParamSet &owned)
      : _wild(0), _owned(owned) { }
  };

  class Provider {
  public:
    QSharedDataPointer<ProviderData> d;
    Provider(const ParamsProvider *wild = 0)
      : d(new ProviderData(wild)) { }
    Provider(const ParamSet &owned) : d(new ProviderData(owned)) { }
  };

  QList<Provider> _providers;
  ParamSet _overridingParams;
  QList<QList<Provider> > _providersStack;
  QList<ParamSet> _overridingParamsStack;
  Utf8String _scope;

public:
  ParamsProviderMerger() { }
  ParamsProviderMerger(const ParamsProviderMerger &other)
    : ParamsProvider(), _providers(other._providers),
      _overridingParams(other._overridingParams), _scope(other._scope) { }
  ParamsProviderMerger(const ParamsProvider *provider, Utf8String scope = {})
    : _scope(scope) {
    append(provider);
  }
  ParamsProviderMerger(
      ParamSet provider, bool inherit = true, Utf8String scope = {})
    : _scope(scope) {
    append(provider, inherit);
  }
  ParamsProviderMerger(ParamSet provider, Utf8String scope)
    : ParamsProviderMerger(provider, true, scope) { }
  /** Add a ParamsProvider that will be evaluated after those already added. */
  ParamsProviderMerger &append(const ParamsProvider *provider) {
    if (provider)
      _providers.append(provider);
    return *this;
  }
  /** Add a ParamsProvider that will be evaluated after those already added. */
  ParamsProviderMerger &append(ParamSet provider, bool inherit = true) {
    if (!provider.isNull()) {
      if (!inherit)
        provider.setParent({});
      _providers.append(provider);
    }
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
  ParamsProviderMerger &prepend(ParamSet provider, bool inherit = true) {
    if (!provider.isNull()) {
      if (!inherit)
        provider.setParent({});
      _providers.prepend(provider);
    }
    return *this;
  }
  ParamsProviderMerger &pop_front() {
    if (!_providers.isEmpty())
      _providers.pop_front();
    return *this;
  }
  ParamsProviderMerger &pop_back() {
    if (!_providers.isEmpty())
      _providers.pop_back();
    return *this;
  }
  /** Convenience operator for append() */
  ParamsProviderMerger &operator()(const ParamsProvider *provider) {
    return append(provider);
  }
  /** Convenience operator for append() */
  ParamsProviderMerger &operator()(ParamSet provider, bool inherit = true) {
    return append(provider, inherit);
  }
  /** Parameters set through overrideParamValue() will override any
   * ParamsProvider, even those prepended. */
  ParamsProviderMerger &overrideParamValue(
      const Utf8String &key, const QVariant &value) {
    _overridingParams.setValue(key, value);
    return *this;
  }
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
  using ParamsProvider::paramRawValue;
  [[nodiscard]] QVariant paramRawValue(
      const Utf8String &key, const QVariant &def = {},
      const EvalContext &context = {}) const override;
  /** Give access to currently overriding params. */
  [[nodiscard]] const ParamSet overridingParams() const {
    return _overridingParams; }
  [[nodiscard]] Utf8StringSet paramKeys(
      const EvalContext &context = {}) const override;
  [[nodiscard]] Utf8String paramScope() const override;
  ParamsProviderMerger &setScope(Utf8String scope) {
    _scope = scope; return *this; }
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

QDebug LIBP6CORESHARED_EXPORT operator<<(
    QDebug dbg, const ParamsProviderMerger *params);

LogHelper LIBP6CORESHARED_EXPORT operator<<(
    LogHelper lh, const ParamsProviderMerger *merger);

#endif // PARAMSPROVIDERMERGER_H
