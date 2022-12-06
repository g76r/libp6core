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
#include "paramsprovider.h"
#include "paramset.h"

extern char **environ;

namespace {

struct Environment : public ParamsProvider {
  const QVariant paramValue(
    const QString &key, const ParamsProvider *context,
    const QVariant &defaultValue,
    QSet<QString> *alreadyEvaluated) const override {
    const char *value = getenv(key.toUtf8());
    return value ? ParamSet().evaluate(value, false, context, alreadyEvaluated)
                 : defaultValue;
  }
  const QSet<QString> keys() const override {
    QSet<QString> keys;
    for (char **e = environ; *e != nullptr; ++e) {
      int i = 0;
      while ((*e)[i] != '\0' && (*e)[i] != '=')
        ++i;
      keys << QString::fromUtf8(*e, i);
    }
    return keys;
  }
};

struct Empty : public ParamsProvider {
  const QVariant paramValue(
    const QString &, const ParamsProvider *, const QVariant &defaultValue,
    QSet<QString> *) const override {
    return defaultValue;
  }
  const QSet<QString> keys() const override {
    return {};
  }
};

} // unnamed namespace

ParamsProvider *ParamsProvider::_environment = new Environment();

ParamsProvider *ParamsProvider::_empty = new Empty();

ParamsProvider::~ParamsProvider() {
}

const ParamSet ParamsProvider::snapshot() const {
  ParamSet snapshot;
  for (auto key: keys())
    snapshot.setValue(key, ParamSet::escape(paramValue(key).toString()));
  return snapshot;
}

const QString ParamsProvider::evaluate(
    const QString &rawValue, QSet<QString> *alreadyEvaluated) const {
  return ParamSet().evaluate(rawValue, false, this, alreadyEvaluated);
}

const QVariant ParamsProvider::paramValue(
  const QString &, const ParamsProvider *, const QVariant &,
  QSet<QString> *) const {
  return {};
}

const QSet<QString> ParamsProvider::keys() const {
  return {};
}

const QVariant RawParamsProvider::paramValue(
  const QString &key, const ParamsProvider *, const QVariant &defaultValue,
  QSet<QString> *) const {
  return _params.value(key, defaultValue);
}

const QSet<QString> RawParamsProvider::keys() const {
  auto keys = _params.keys();
  return QSet<QString>(keys.begin(), keys.end());
}

