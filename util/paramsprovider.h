/* Copyright 2013 Hallowyn and others.
 * This file is part of qron, see <http://qron.hallowyn.com/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PARAMSPROVIDER_H
#define PARAMSPROVIDER_H

#include <QVariant>
#include "libqtssu_global.h"

/** Base class for any class that may provide text parameters.
 * @see ParamSet
 */
class LIBQTSSUSHARED_EXPORT ParamsProvider {
public:
  ParamsProvider();
  virtual ~ParamsProvider();
  /** Return a parameter value. */
  virtual QVariant paramValue(
      const QString key, const QVariant defaultValue = QVariant()) const = 0;
};

#endif // PARAMSPROVIDER_H
