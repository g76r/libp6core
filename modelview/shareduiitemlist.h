/* Copyright 2015-2022 Hallowyn, Gregoire Barbier and others.
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
#ifndef SHAREDUIITEMLIST_H
#define SHAREDUIITEMLIST_H

#include "shareduiitem.h"
#include <QList>

class SharedUiItemListParamsProvider;

/** Specializing QList for SharedUiItems, the same way QStringList does. */
template <class T = SharedUiItem>
class LIBP6CORESHARED_EXPORT SharedUiItemList : public QList<T> {
public:
  inline SharedUiItemList() {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
  }
  inline SharedUiItemList(const SharedUiItemList<T> &other)
    : QList<T>(other) {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
  }
  inline SharedUiItemList(const QList<T> &other) : QList<T>(other) {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
  }
  inline QString join(const QString &separator, bool qualified = false) const;
  inline QString join(const QChar separator, bool qualified = false) const;
  // conversion operator to enable upcasting any list to a SharedUiItem list
  operator SharedUiItemList<SharedUiItem> &() {
    return reinterpret_cast<SharedUiItemList<SharedUiItem>&>(*this);
  }
  // conversion operator to enable upcasting any list to a SharedUiItem list
  operator const SharedUiItemList<SharedUiItem> &() const {
    return reinterpret_cast<const SharedUiItemList<SharedUiItem>&>(*this);
  }
  /** Make uiData() available through ParamsProvider interface, using
   * a %{qualifierId:sectionNameOrNumber} or %{sectionNameOrNumber} syntax,
   * the former being a filter among items depending of their qualifier/type.
   * @see SharedUiItemParamsProvider */
  inline SharedUiItemListParamsProvider toParamsProvider() const;
  // MAYDO add features
  //SharedUiItemList filterByQualifier(QString qualifier) const;
  //SharedUiItemList filterByQualifier(QRegularExpression qualifier) const;
  //operator<<(SharedUiItem)
  //operator<<(SharedUiItemList)
  //operator<<(QList<SharedUiItem>)
  //operator+=
};

/** Template specialization for SharedUiItemList<SharedUiItem> */
template <>
class LIBP6CORESHARED_EXPORT SharedUiItemList<SharedUiItem>
    : public QList<SharedUiItem> {
private:
  template <class S>
  inline QString generic_join(const S &separator, bool qualified) const {
    QString s;
    bool first = true;
    for (const SharedUiItem &item : *this) {
      if (first)
        first = false;
      else
        s += separator;
      if (qualified) {
        s += item.idQualifier();
        s += ':';
        s += item.id();
      } else
        s += item.id();
    }
    return s;
  }

public:
  inline SharedUiItemList() { }
  inline SharedUiItemList(const SharedUiItem &item)
    : QList<SharedUiItem>({ item }) { }
  inline SharedUiItemList(std::initializer_list<SharedUiItem> &items)
    : QList<SharedUiItem>(items) { }
  inline SharedUiItemList(const SharedUiItemList<SharedUiItem> &other)
    : QList<SharedUiItem>(other) { }
  inline SharedUiItemList(const QList<SharedUiItem> &other)
    : QList<SharedUiItem>(other) { }
  // upcasting constructor to convert any list to a SharedUiItem list
  template<class T>
  inline SharedUiItemList(const SharedUiItemList<T> &other)
    : QList<SharedUiItem>(reinterpret_cast<const QList<SharedUiItem>&>(other)) {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
  }
  // upcasting constructor to convert any list to a SharedUiItem list
  template<class T>
  inline SharedUiItemList(const QList<T>& other)
    : QList<SharedUiItem>(reinterpret_cast<const QList<SharedUiItem>&>(other)) {
    T *dummy;
    Q_UNUSED(static_cast<SharedUiItem*>(dummy)); // ensure T is a SharedUiItem
  }
  QString join(const QString &separator, bool qualified = false) const {
    return generic_join(separator, qualified);
  }
  QString join(const QChar separator, bool qualified = false) const {
    return generic_join(separator, qualified);
  }
  inline SharedUiItemListParamsProvider toParamsProvider() const;
};

Q_DECLARE_METATYPE(SharedUiItemList<>)

template <class T>
inline QString SharedUiItemList<T>::join(
    const QString &separator, bool qualified) const {
  const SharedUiItemList<SharedUiItem> &upcasted = *this;
  return upcasted.join(separator, qualified);
}

template <class T>
inline QString SharedUiItemList<T>::join(
    const QChar separator, bool qualified) const {
  const SharedUiItemList<SharedUiItem> &upcasted = *this;
  return upcasted.join(separator, qualified);
}

/** ParamsProvider wrapper for SharedUiItemList. */
class LIBP6CORESHARED_EXPORT SharedUiItemListParamsProvider
    : public ParamsProvider {
  SharedUiItemList<> _list;
  int _role;

public:
  inline SharedUiItemListParamsProvider(
      const SharedUiItemList<> &list, int role = Qt::DisplayRole)
    : _list(list), _role(role) { }
  QVariant paramValue(QString key, const ParamsProvider *context = 0,
                      QVariant defaultValue = QVariant(),
                      QSet<QString> alreadyEvaluated = QSet<QString>()
          ) const override;
  QSet<QString> keys() const override;
};

template <class T>
inline SharedUiItemListParamsProvider
SharedUiItemList<T>::toParamsProvider() const {
  return SharedUiItemListParamsProvider(*this);
}

inline SharedUiItemListParamsProvider
SharedUiItemList<SharedUiItem>::toParamsProvider() const {
  return SharedUiItemListParamsProvider(*this);
}

#endif // SHAREDUIITEMLIST_H
