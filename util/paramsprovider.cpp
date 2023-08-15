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

extern char **environ; // LATER use QProcessEnvironment::systemEnvironment()

namespace {

struct Environment : public ParamsProvider {
  const QVariant paramValue(
    const Utf8String &key, const ParamsProvider *context,
    const QVariant &defaultValue,
    Utf8StringSet *alreadyEvaluated) const override {
    auto rawValue = qgetenv(key);
    if (rawValue.isNull())
      return defaultValue;
    return ParamSet().evaluate(rawValue, false, context, alreadyEvaluated);
  }
  const Utf8StringSet keys() const override {
    Utf8StringSet keys;
    for (char **e = environ; *e != nullptr; ++e) {
      int i = 0;
      while ((*e)[i] != '\0' && (*e)[i] != '=')
        ++i;
      keys << QString::fromLocal8Bit(*e, i);
    }
    return keys;
  }
};

struct Empty : public ParamsProvider {
  const QVariant paramValue(
    const Utf8String &, const ParamsProvider *, const QVariant &defaultValue,
    Utf8StringSet *) const override {
    return defaultValue;
  }
  const Utf8StringSet keys() const override {
    return {};
  }
};

} // unnamed namespace

ParamsProvider *ParamsProvider::_environment = new Environment();

ParamsProvider *ParamsProvider::_empty = new Empty();

const ParamSet ParamsProvider::snapshot() const {
  ParamSet snapshot;
  for (auto key: keys())
    snapshot.setValue(key, ParamSet::escape(paramValue(key).toString()));
  return snapshot;
}

const Utf8String ParamsProvider::evaluate(
    const Utf8String &rawValue, Utf8StringSet *alreadyEvaluated) const {
  return ParamSet().evaluate(rawValue, false, this, alreadyEvaluated);
}

const QVariant ParamsProvider::paramValue(
    const Utf8String &, const ParamsProvider *, const QVariant &,
  Utf8StringSet *) const {
  return {};
}

const Utf8StringSet ParamsProvider::keys() const {
  return {};
}

const Utf8String ParamsProvider::paramScope() const {
  return {};
}

const QVariant RawParamsProvider::paramValue(
    const Utf8String &key, const ParamsProvider *, const QVariant &defaultValue,
    Utf8StringSet *) const {
  return _params.value(key, defaultValue);
}

const Utf8StringSet RawParamsProvider::keys() const {
  auto keys = _params.keys();
  return QSet<Utf8String>(keys.begin(), keys.end());
}

const QVariant ParamsProvider::paramValue(
  const Utf8String &key, const ParamsProvider *context,
  const QVariant &defaultValue) const {
  Utf8StringSet ae;
  return paramValue(key, context, defaultValue, &ae);
}

const Utf8String ParamsProvider::evaluate(const Utf8String &rawValue) const {
  Utf8StringSet ae;
  return evaluate(rawValue, &ae);
}
