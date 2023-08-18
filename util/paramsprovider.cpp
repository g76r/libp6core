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
//  const QVariant paramValue(
//    const Utf8String &key, const QVariant &def, const ParamsProvider *context,
//    Utf8StringSet *alreadyEvaluated) const override {
//    auto rawValue = qgetenv(key); // TODO ::fromLocal8bit() for non utf8 oses
//    if (rawValue.isNull())
//      return def;
//    return ParamSet().evaluate(rawValue, false, context, alreadyEvaluated);
//  }
  const QVariant paramRawValue(
    const Utf8String &key, const QVariant &def) const override {
    auto v = qgetenv(key);
    return v.isNull() ? def : QString::fromLocal8Bit(v);
  }
  const Utf8StringSet paramKeys() const override {
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
    const Utf8String &, const QVariant &def) const override {
    return def;
  }
  const Utf8StringSet paramKeys() const override {
    return {};
  }
};

} // unnamed namespace

ParamsProvider *ParamsProvider::_environment = new Environment();

ParamsProvider *ParamsProvider::_empty = new Empty();

const ParamSet ParamsProvider::paramSnapshot() const {
  ParamSet snapshot;
  for (auto key: paramKeys())
    snapshot.setValue(key, PercentEvaluator::escape(
                        paramValue(key).value.toString()));
  return snapshot;
}

const ParamsProvider::ScopedValue ParamsProvider::paramValue(
  const Utf8String &key, const QVariant &def,
    const ParamsProvider *context) const {
  Utf8StringSet ae;
  return paramValue(key, def, context, &ae);
}

const ParamsProvider::ScopedValue ParamsProvider::paramValue(
    const Utf8String &key, const QVariant &def, const ParamsProvider *context,
    Utf8StringSet *ae) const {
  auto v = paramRawValue(key);
  if (!v.isValid())
    return { {}, def };
  auto expr = Utf8String(v);
  if (expr.isEmpty())
    return { paramScope(), v }; // passing QVariant trough
  return PercentEvaluator::eval(expr, context, ae);
}

const QVariant ParamsProvider::paramRawValue(
    const Utf8String &, const QVariant &def) const {
  return def;
}

const Utf8String ParamsProvider::paramRawUtf8(
    const Utf8String &key, const Utf8String &def) const {
  return Utf8String(paramRawValue(key, def));
}

const ParamsProvider::ScopedValue ParamsProvider::paramScopedRawValue(
    const Utf8String &key, const QVariant &def) const {
  auto v = paramRawValue(key, def);
  if (v.isValid())
    return {paramScope(), v};
  return {};
}

const Utf8String ParamsProvider::paramUtf8(
    const Utf8String &key, const Utf8String &def,
    const ParamsProvider *context, Utf8StringSet *alreadyEvaluated) const {
  return Utf8String(paramValue(key, def, context, alreadyEvaluated));
}

const Utf8String ParamsProvider::paramUtf8(
  const Utf8String &key, const Utf8String &def,
    const ParamsProvider *context) const {
  Utf8StringSet ae;
  return paramUtf8(key, def, context, &ae);
}

const Utf8StringSet ParamsProvider::paramKeys() const {
  return {};
}

bool ParamsProvider::paramContains(const Utf8String &key) const {
  return paramRawValue(key).isValid();
}

const Utf8String ParamsProvider::paramScope() const {
  return {};
}

const Utf8String ParamsProvider::evaluate(
    const Utf8String &key, const ParamsProvider *context,
    Utf8StringSet *ae) const {
  Utf8StringSet ae2;
  auto ppm = ParamsProviderMerger(this)(context);
  return Utf8String(PercentEvaluator::eval(key, &ppm, ae ? ae : &ae2));
}

Utf8StringList ParamsProvider::splitAndEvaluate(
    const Utf8String &key, const Utf8String &separators, bool,
    const ParamsProvider *context, Utf8StringSet *ae) const {
  Utf8StringSet ae2;
  auto ppm = ParamsProviderMerger(this)(context);
  QList<char> seps;
  for (int i = 0; i < separators.size(); ++i)
    seps.append(separators[i]);
  Utf8StringList input = key.split(seps, Qt::SkipEmptyParts), result;
  for (int i = 0; i < input.size(); ++i)
    result.append(
          Utf8String(PercentEvaluator::eval(input[i], &ppm, ae ? ae : &ae2)));
  return result;
}

const Utf8String SimpleParamsProvider::paramScope() const {
  return _scope;
}

const QVariant SimpleParamsProvider::paramRawValue(
    const Utf8String &key, const QVariant &def) const {
  return _params.value(key, def);
}

const Utf8StringSet SimpleParamsProvider::paramKeys() const {
  return _params.keys();
}

bool SimpleParamsProvider::paramContains(const Utf8String &key) const {
  return _params.contains(key);
}

const SimpleParamsProvider::ScopedValue SimpleParamsProvider::paramScopedRawValue(
    const Utf8String &key, const QVariant &def) const {
  auto v = _params.value(key);
  if (v.isValid())
    return {_scope, v};
  return {{}, def};
}
