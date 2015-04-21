/* Copyright 2013-2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PARAMSPROVIDERLIST_H
#define PARAMSPROVIDERLIST_H

#include "paramsprovider.h"

/** This class builds up several ParamsProvider into only one, chaining
 * calls to paramValue().
 * Keep in mind that it does not take ownership on referenced ParamsProvider
 * objects and that these objects must not be deleted before the last call
 * to ParamsProviderList::paramValue().
 */
class LIBQTSSUSHARED_EXPORT ParamsProviderList : public ParamsProvider {
  QList<const ParamsProvider*> _list;

public:
  ParamsProviderList() { }
  ParamsProviderList(const ParamsProviderList &other)
    : ParamsProvider(), _list(other._list) { }
  ParamsProviderList(const ParamsProvider *provider) {
    append(provider); }
  ParamsProviderList &append(const ParamsProvider *provider) {
    if (provider)
      _list.append(provider);
    return *this; }
  ParamsProviderList &append(const ParamsProviderList *providerList) {
    if (providerList)
      _list.append(providerList->_list);
    return *this; }
  ParamsProviderList &append(const ParamsProviderList &providerList) {
    _list.append(providerList._list);
    return *this; }
  ParamsProviderList &prepend(const ParamsProvider *provider) {
    if (provider)
      _list.prepend(provider);
    return *this; }
  ParamsProviderList &clear() {
    _list.clear();
    return *this; }
  operator QList<const ParamsProvider*>() const { return _list; }
  int size() const { return _list.size(); }
  /** Convenience operator for append() */
  ParamsProviderList &operator()(const ParamsProvider *provider) {
    if (provider)
      _list.append(provider);
    return *this; }
  QVariant paramValue(QString key, QVariant defaultValue = QVariant(),
                      QSet<QString> alreadyEvaluated = QSet<QString>()) const;
};

#endif // PARAMSPROVIDERLIST_H
