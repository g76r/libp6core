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
#include "paramsprovider.h"
#include "paramset.h"
#include "util/utf8stringset.h"
#include "util/percentevaluator.h"
#include "util/paramsprovidermerger.h"

extern char **environ; // LATER use QProcessEnvironment::systemEnvironment()

namespace {

struct Environment : public ParamsProvider {
  const QVariant paramRawValue(
    const Utf8String &key, const QVariant &def,
      const EvalContext &) const override {
    auto v = qgetenv(key);
    return v.isNull() ? def : QString::fromLocal8Bit(v);
  }
  const Utf8StringSet paramKeys(const EvalContext &) const override {
    Utf8StringSet keys;
    for (char **e = environ; *e != nullptr; ++e) {
      int i = 0;
      while ((*e)[i] != '\0' && (*e)[i] != '=')
        ++i;
      keys << QString::fromLocal8Bit(*e, i);
    }
    return keys;
  }
  const Utf8String paramScope() const override {
    return "env"_u8;
  }
};

struct Empty : public ParamsProvider {
  const QVariant paramRawValue(
    const Utf8String &, const QVariant &def,
      const EvalContext &) const override {
    return def;
  }
  const Utf8StringSet paramKeys(const EvalContext &) const override {
    return {};
  }
};

} // unnamed namespace

ParamsProvider *ParamsProvider::_environment = new Environment();

ParamsProvider *ParamsProvider::_empty = new Empty();

const ParamSet ParamsProvider::paramSnapshot() const {
  ParamSet snapshot;
  for (auto key: paramKeys())
    snapshot.setValue(key, PercentEvaluator::escape(paramValue(key)));
  return snapshot;
}

const QVariant ParamsProvider::paramValue(
    const Utf8String &key, const QVariant &def,
    const EvalContext &context) const {
  auto v = paramRawValue(key, {}, context);
  if (!v.isValid())
    return def;
  auto id = v.metaType().id();
  if (v.canConvert<double>() && id != qMetaTypeId<Utf8String>()
      && id != QMetaType::QString && id != QMetaType::QByteArray)
    return v; // passing QVariant through if number
  v =  PercentEvaluator::eval(Utf8String(v), context);
  if (!v.isValid())
    return def;
  return v;
}

const QVariant ParamsProvider::paramRawValue(
    const Utf8String &, const QVariant &def, const EvalContext&) const {
  return def;
}

const Utf8String ParamsProvider::paramRawUtf8(
    const Utf8String &key, const Utf8String &def,
    const EvalContext &context) const {
  return Utf8String(paramRawValue(key, def, context));
}

const Utf8String ParamsProvider::paramUtf8(
    const Utf8String &key, const Utf8String &def,
    const EvalContext &context) const {
  return Utf8String(paramValue(key, def, context));
}

const Utf8StringSet ParamsProvider::paramKeys(const EvalContext &) const {
  return {};
}

bool ParamsProvider::paramContains(
    const Utf8String &key, const EvalContext &) const {
  return paramRawValue(key).isValid();
}

const Utf8String ParamsProvider::paramScope() const {
  return {};
}

const Utf8String ParamsProvider::evaluate(
    const Utf8String &key, const ParamsProvider *context,
    Utf8StringSet *ae) const {
  EvalContext new_context;
  auto ppm = ParamsProviderMerger(this)(context);
  new_context.setParamsProvider(&ppm);
  if (ae)
    for (auto variable: *ae)
      new_context.addVariable(variable);
  return Utf8String(PercentEvaluator::eval(key, new_context));
}

const Utf8String ParamsProvider::evaluate(
    const Utf8String &key, bool inherit,
    const ParamsProvider *context, Utf8StringSet *ae) const {
  EvalContext new_context;
  auto ppm = ParamsProviderMerger(this)(context);
  new_context.setParamsProvider(&ppm);
  if (ae)
    for (auto variable: *ae)
      new_context.addVariable(variable);
  if (!inherit)
    new_context.setScopeFilter(ParamSet::DontInheritScope);
  return Utf8String(PercentEvaluator::eval(key, new_context));
}

Utf8StringList ParamsProvider::splitAndEvaluate(
    const Utf8String &key, const Utf8String &separators, bool inherit,
    const ParamsProvider *context, Utf8StringSet *ae) const {
  EvalContext new_context;
  auto ppm = ParamsProviderMerger(this)(context);
  new_context.setParamsProvider(&ppm);
  if (ae)
    for (auto variable: *ae)
      new_context.addVariable(variable);
  new_context.setParamsProvider(&ppm);
  if (!inherit)
    new_context.setScopeFilter(ParamSet::DontInheritScope);
  QList<char> seps;
  for (int i = 0; i < separators.size(); ++i)
    seps.append(separators[i]);
  Utf8StringList input = key.split(seps, Qt::SkipEmptyParts), result;
  for (int i = 0; i < input.size(); ++i)
    result += Utf8String(PercentEvaluator::eval(input[i], new_context));
  return result;
}

const Utf8String SimpleParamsProvider::paramScope() const {
  return _scope;
}

const QVariant SimpleParamsProvider::paramRawValue(
    const Utf8String &key, const QVariant &def, const EvalContext &) const {
  return _params.value(key, def);
}

const Utf8StringSet SimpleParamsProvider::paramKeys(
    const EvalContext &) const {
  return _params.keys();
}

bool SimpleParamsProvider::paramContains(
    const Utf8String &key, const EvalContext &) const {
  return _params.contains(key);
}
