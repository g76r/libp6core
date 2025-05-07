/* Copyright 2015-2025 Hallowyn, Gregoire Barbier and others.
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
#ifdef __cpp_concepts
  template <shareduiitem_subclass T>
#else
  template <class T,
            std::enable_if_t<std::is_base_of_v<SharedUiItem,T>,bool> = true>
#endif
  inline SharedUiItemList(const QList<T> &other)
    : QList<SharedUiItem>(reinterpret_cast<const QList<SharedUiItem>&>(other)) {
  }
  inline SharedUiItemList(std::initializer_list<SharedUiItem> &items)
    : QList<SharedUiItem>(items) { }
  inline SharedUiItemList(const SharedUiItem &item)
    : QList<SharedUiItem>({item}) {}
  SharedUiItemList &operator=(const SharedUiItemList &that) {
    QList::operator=(that); return *this; }
  Utf8String join(const QByteArray &separator) const;
  Utf8String join(const char separator) const;
  Utf8String join(const char32_t separator) const;
  Utf8String join(const Utf8String &separator, const Utf8String &format,
                  const Utf8StringSet &qualifiers = {}) const;
  Utf8String join(const char separator, const Utf8String &format,
                  const Utf8StringSet &qualifiers = {}) const;
  Utf8String join(const char32_t separator, const Utf8String &format,
                  const Utf8StringSet &qualifiers = {}) const;
  QString joinUtf16(const QString &separator) const;
  QString joinUtf16(const QChar separator) const;
  QString joinUtf16(const QString &separator, const Utf8String &format,
                    const Utf8StringSet &qualifiers = {}) const;
  QString joinUtf16(const QChar separator, const Utf8String &format,
                    const Utf8StringSet &qualifiers = {}) const;
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
#ifdef __cpp_concepts
  template <shareduiitem_subclass T = SharedUiItem>
#else
  template<class T = SharedUiItem,
           std::enable_if_t<std::is_base_of_v<SharedUiItem,T>,bool> = true>
#endif
  inline QList<T> filtered(const Utf8String &qualifier) {
    QList<T> subset;
    for (const auto &sui: *this)
      if (sui.qualifier() == qualifier)
        subset += sui.casted<const T>();
        //subset += static_cast<const T&>(sui);
    return subset;
  }
  /** Select items given their qualifier. */
  inline SharedUiItemList filtered(const Utf8StringSet &qualifiers) {
    SharedUiItemList subset;
    for (const auto &sui: *this)
      if (qualifiers.contains(sui.qualifier()))
        subset += sui;
    return subset;
  }
  /** Blindly trust that every item in list is of type T.
   *  This is undefinied behaviour, use filtered() or join() instead. */
#ifdef __cpp_concepts
  template <shareduiitem_subclass T = SharedUiItem>
#else
  template<class T = SharedUiItem,
           std::enable_if_t<std::is_base_of_v<SharedUiItem,T>,bool> = true>
#endif
  inline QList<T> &casted() {
    return reinterpret_cast<QList<T>&>(static_cast<QList<SharedUiItem>&>(*this));
  }
  /** Blindly trust that every item in list is of type T.
   *  This is undefinied behaviour, use filtered() or join() instead. */
#ifdef __cpp_concepts
  template <shareduiitem_subclass T = SharedUiItem>
#else
  template<class T = SharedUiItem,
           std::enable_if_t<std::is_base_of_v<SharedUiItem,T>,bool> = true>
#endif
  inline const QList<T> &casted() const {
    return reinterpret_cast<const QList<T>&>(static_cast<const QList<SharedUiItem>&>(*this));
  }

  /** Append items if they're not yet present in the list.
   *  Expensive on large lists. */
  inline SharedUiItemList& operator|=(const SharedUiItemList &that) {
    for (const auto &sui: that)
      if (!contains(sui))
        operator+=(sui);
    return *this;
  }
  /** Append item if it's not yet present in the list.
   *  Expensive on large lists. */
  inline SharedUiItemList& operator|=(const SharedUiItem &sui){
    return this->operator |=(SharedUiItemList{sui});
  }
};

Q_DECLARE_METATYPE(SharedUiItemList)

#endif // SHAREDUIITEMLIST_H
