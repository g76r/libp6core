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
#ifndef PARAMSPROVIDER_H
#define PARAMSPROVIDER_H

#include <QVariant>
#include <QList>
#include <QSet>
#include "libp6core_global.h"

/** Base class for any class that may provide key/value parameters.
 * @see ParamSet */
class LIBP6CORESHARED_EXPORT ParamsProvider {
    static ParamsProvider *_environment;

public:
  virtual ~ParamsProvider();
  /** Return a parameter value.
    * @param context is an evaluation context */
  virtual QVariant paramValue(
          QString key, const ParamsProvider *context = 0,
          QVariant defaultValue = QVariant(),
          QSet<QString> alreadyEvaluated = QSet<QString>()) const = 0;
  /** Backward compatibility and convenience method callable w/o context */
  QVariant paramValue(
          QString key, QVariant defaultValue,
          QSet<QString> alreadyEvaluated = QSet<QString>()) const {
    return paramValue(key, 0, defaultValue, alreadyEvaluated);
  }
  /** Convenience method converting QVariant to QString. */
  QString paramString(QString key, QVariant defaultValue = QVariant(),
                      QSet<QString> alreadyEvaluated = QSet<QString>()) const {
    return paramValue(key, defaultValue, alreadyEvaluated).toString();
  }
  /** Convenience method converting QVariant to QString. */
  QString paramString(QString key, const ParamsProvider *context,
                      QVariant defaultValue = QVariant(),
                      QSet<QString> alreadyEvaluated = QSet<QString>()) const {
    return paramValue(key, context, defaultValue, alreadyEvaluated).toString();
  }
  static ParamsProvider *environment() { return _environment; }
};

#endif // PARAMSPROVIDER_H
