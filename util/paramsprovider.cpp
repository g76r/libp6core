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
#include "paramsprovider.h"
#include "paramset.h"
#include "util/utf8stringset.h"
#include "util/percentevaluator.h"
#include "util/paramsprovidermerger.h"

extern char **environ; // LATER use QProcessEnvironment::systemEnvironment()

namespace {

struct Environment : public ParamsProvider {
  static Utf8String _scope;
  QVariant paramRawValue(
    const Utf8String &key, const QVariant &def,
      const EvalContext &context) const override {
    if (context.hasScopeOrNone(_scope)) {
      auto v = qEnvironmentVariable(key);
      if (!v.isNull())
        return v;
    }
    return def;
  }
  Utf8StringSet paramKeys(const EvalContext &) const override {
    Utf8StringSet keys;
    for (char **e = environ; *e != nullptr; ++e) {
      int i = 0;
      while ((*e)[i] != '\0' && (*e)[i] != '=')
        ++i;
      keys << QString::fromLocal8Bit(*e, i);
    }
    return keys;
  }
  Utf8String paramScope() const override {
    return _scope;
  }
};

Utf8String Environment::_scope = "env"_u8;

struct Empty : public ParamsProvider {
  QVariant paramRawValue(
    const Utf8String &, const QVariant &def,
      const EvalContext &) const override {
    return def;
  }
  Utf8StringSet paramKeys(const EvalContext &) const override {
    return {};
  }
};

} // unnamed namespace

ParamsProvider *ParamsProvider::_environment = new Environment();

ParamsProvider *ParamsProvider::_empty = new Empty();

ParamSet ParamsProvider::paramSnapshot() const {
  ParamSet snapshot;
  for (const auto &key: paramKeys())
    snapshot.insert(key, PercentEvaluator::escape(paramValue(key)));
  return snapshot;
}

QVariant ParamsProvider::paramValue(
    const Utf8String &original_key, const QVariant &def,
    const EvalContext &original_context) const {
  Utf8String key = original_key;
  EvalContext context = original_context;
  QVariant v;
  // support for [scope] prefix
  // note that this is a duplicated test when paramValue() is called from
  // PercentEvaluator::eval_key() but when calling directly
  // ParamsProvider::paramXXX() it's far more consistent and convenient not to
  // have to call PercentEvaluator::eval_key(key, my_context_plus_this) instead
  if (key.value(0) == '['){
    [[unlikely]];
    auto eos = key.indexOf(']');
    if (eos < 0) {// no ] in key
      v = def;
      [[unlikely]] goto skip_param_raw_value;
    }
    context.setScopeFilter(key.mid(1, eos-1));
    key = key.mid(eos+1);
  }
  if (!context.functionsEvaluated()) {
    [[likely]];
    bool is_function;
    v = PercentEvaluator::eval_function(key, context, &is_function);
    if (is_function)
      return v;
    // don't call context.setFunctionsEvaluated() because ParamsProvider
    // implementation may include custom functions or other complex features
    // implying %-evaluation re-entrance needing functions evaluation again
  }
  // don't check scope filter here, because it's up to paramRawValue to do that
  v = paramRawValue(key, def, context);
skip_param_raw_value:
  auto id = v.metaType().id();
  // passing QVariant through if non string type (number, invalid, QPointF...)
  // LATER may add some types here: QJsonValue if text ?
  if (id != qMetaTypeId<Utf8String>() && id != QMetaType::QString
      && id != QMetaType::QByteArray)
    return v;
  if (!context.paramsProvider()) { // if context has no pp, use this as a pp
    context.setParamsProvider(this);
    v = PercentEvaluator::eval(Utf8String(v), context);
  } else { // else prepend this to the context pp
      auto ppm = ParamsProviderMerger(this)(context.paramsProvider());
      context.setParamsProvider(&ppm);
      v = PercentEvaluator::eval(Utf8String(v), context);
  }
  return v;
}

QVariant ParamsProvider::paramRawValue(
    const Utf8String &, const QVariant &def, const EvalContext&) const {
  return def;
}

Utf8StringSet ParamsProvider::paramKeys(const EvalContext &) const {
  return {};
}

bool ParamsProvider::paramContains(
    const Utf8String &key, const EvalContext &) const {
  return paramRawValue(key).isValid();
}

Utf8String ParamsProvider::paramScope() const {
  return {};
}

#if PARAMSET_SUPPORTS_DONTINHERIT
Utf8String ParamsProvider::evaluate(
    const Utf8String &key, const ParamsProvider *context,
    Utf8StringSet *ae) const {
  EvalContext new_context;
  auto ppm = ParamsProviderMerger(this)(context);
  new_context.setParamsProvider(&ppm);
  if (ae)
    for (const auto &variable: *ae)
      new_context.addVariable(variable);
  return PercentEvaluator::eval_utf8(key, new_context);
}

Utf8String ParamsProvider::evaluate(
    const Utf8String &key, bool inherit,
    const ParamsProvider *context, Utf8StringSet *ae) const {
  EvalContext new_context;
  auto ppm = ParamsProviderMerger(this)(context);
  new_context.setParamsProvider(&ppm);
  if (ae)
    for (const auto &variable: *ae)
      new_context.addVariable(variable);
  if (!inherit && dynamic_cast<const ParamSet*>(this))
    new_context.setScopeFilter(ParamSet::DontInheritScope);
  return PercentEvaluator::eval_utf8(key, new_context);
}

Utf8StringList ParamsProvider::splitAndEvaluate(
    const Utf8String &key, const Utf8String &separators, bool inherit,
    const ParamsProvider *context, Utf8StringSet *ae) const {
  EvalContext new_context;
  auto ppm = ParamsProviderMerger(this)(context);
  new_context.setParamsProvider(&ppm);
  if (ae)
    for (const auto &variable: *ae)
      new_context.addVariable(variable);
  new_context.setParamsProvider(&ppm);
  if (!inherit && dynamic_cast<const ParamSet*>(this))
    new_context.setScopeFilter(ParamSet::DontInheritScope);
  QList<char> seps;
  for (int i = 0; i < separators.size(); ++i)
    seps.append(separators[i]);
  Utf8StringList input = key.split(seps, Qt::SkipEmptyParts), result;
  for (int i = 0; i < input.size(); ++i)
    result += PercentEvaluator::eval_utf8(input[i], new_context);
  return result;
}
#endif

Utf8String SimpleParamsProvider::paramScope() const {
  return _scope;
}

QVariant SimpleParamsProvider::paramRawValue(
    const Utf8String &key, const QVariant &def,
    const EvalContext &context) const {
  if (!context.functionsEvaluated()) {
    bool is_function;
    auto v = PercentEvaluator::eval_function(key, context, &is_function);
    if (is_function)
      return v;
  }
  if (context.hasScopeOrNone(paramScope())) {
    auto v = _params.value(key);
    if (v.isValid())
      return v;
  }
  return def;
}

Utf8StringSet SimpleParamsProvider::paramKeys(
    const EvalContext &) const {
  return _params.keys();
}

bool SimpleParamsProvider::paramContains(
    const Utf8String &key, const EvalContext &) const {
  return _params.contains(key);
}
