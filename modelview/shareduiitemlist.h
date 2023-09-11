/* Copyright 2015-2023 Hallowyn, Gregoire Barbier and others.
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

/** Specializing QList for SharedUiItems, the same way QStringList does. */
class LIBP6CORESHARED_EXPORT SharedUiItemList
    : public QList<SharedUiItem>, public ParamsProvider {
public:
  inline SharedUiItemList() {}
  inline SharedUiItemList(const SharedUiItemList &other)
    : QList<SharedUiItem>(other) {}
  inline SharedUiItemList(const QList<SharedUiItem> &other)
    : QList<SharedUiItem>(other) {}
  template <class T,
            std::enable_if_t<std::is_base_of_v<SharedUiItem,T>,bool> = true>
  inline SharedUiItemList(const QList<T> &other)
    : QList<SharedUiItem>(reinterpret_cast<const QList<SharedUiItem>&>(other)) {
  }
  inline SharedUiItemList(std::initializer_list<SharedUiItem> &items)
    : QList<SharedUiItem>(items) { }
  Utf8String join(const QByteArray &separator, bool qualified = false) const;
  Utf8String join(const char separator, bool qualified = false) const;
  Utf8String join(const char32_t separator, bool qualified = false) const;
  QString joinUtf16(const QString &separator, bool qualified = false) const;
  QString joinUtf16(const QChar separator, bool qualified = false) const;
  QVariant paramRawValue(
      const Utf8String &key, const QVariant &def = {},
      const ParamsProvider::EvalContext &context = {}) const override;
  Utf8StringSet paramKeys(
      const ParamsProvider::EvalContext &context = {}) const override;
  inline SharedUiItemList sorted() const {
    SharedUiItemList sorted = *this;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
  }
  /** Select items given their qualifier. Blindly trust that T and qualifier
   *  match each other. */
  template<class T = SharedUiItem>
  inline QList<T> filtered(const Utf8String &qualifier) {
    QList<T> subset;
    for (auto sui: *this)
      if (sui.qualifier() == qualifier)
        subset += sui.casted<const T>();
        //subset += static_cast<const T&>(sui);
    return subset;
  }
  /** Select items given their qualifier. */
  inline SharedUiItemList filtered(const Utf8StringSet &qualifiers) {
    SharedUiItemList subset;
    for (auto sui: *this)
      if (qualifiers.contains(sui.qualifier()))
        subset += sui;
    return subset;
  }
  /** Append items if they're not yet present in the list.
   *  Expensive on large lists. */
  inline SharedUiItemList& operator|=(const SharedUiItemList &that) {
    for (auto sui: that)
      if (!contains(sui))
        operator+=(sui);
    return *this;
  }
  /** Append item if it's not yet present in the list.
   *  Expensive on large lists. */
  inline SharedUiItemList& operator|=(const SharedUiItem &sui) {
    return this->operator |=({sui});
  }
};

Q_DECLARE_METATYPE(SharedUiItemList)

#endif // SHAREDUIITEMLIST_H
