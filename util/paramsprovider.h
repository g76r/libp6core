/* Copyright 2013-2014 Hallowyn and others.
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
#include <QList>
#include "libqtssu_global.h"

/** Base class for any class that may provide key/value parameters.
 * @see ParamSet */
class LIBQTSSUSHARED_EXPORT ParamsProvider {
public:
  virtual ~ParamsProvider();
  /** Return a parameter value. */
  virtual QVariant paramValue(QString key,
                              QVariant defaultValue = QVariant()) const = 0;
};

/** This class builds up several ParamsProvider into only one, chaining
 * calls to paramValue() */
class LIBQTSSUSHARED_EXPORT ParamsProviderList : public ParamsProvider {
  QList<ParamsProvider*> _list;

public:
  ParamsProviderList() { }
  ParamsProviderList(const ParamsProviderList &other)
    : ParamsProvider(), _list(other._list) { }
  ParamsProviderList(ParamsProvider *provider) {
    append(provider); }
  ParamsProviderList &append(ParamsProvider *provider) {
    if (provider)
      _list.append(provider);
    return *this; }
  ParamsProviderList &append(ParamsProviderList *providerList) {
    if (providerList)
      _list.append(*providerList);
    return *this; }
  ParamsProviderList &append(const ParamsProviderList &providerList) {
    _list.append(providerList);
    return *this; }
  ParamsProviderList &prepend(ParamsProvider *provider) {
    if (provider)
      _list.prepend(provider);
    return *this; }
  ParamsProviderList &clear() {
    _list.clear();
    return *this; }
  operator const QList<ParamsProvider*>() const { return _list; }
  operator QList<ParamsProvider*>() { return _list; }
  QVariant paramValue(QString key, QVariant defaultValue = QVariant()) const;
};

#endif // PARAMSPROVIDER_H
