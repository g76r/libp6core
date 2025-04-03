/* Copyright 2023-2025 Gregoire Barbier and others.
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
#ifndef UTF8STRINGLIST_H
#define UTF8STRINGLIST_H

#include "util/paramsprovider.h"
#include <set>

class Utf8StringSet;
class LogHelper;

class LIBP6CORESHARED_EXPORT Utf8StringList
    : public QList<Utf8String>, public ParamsProvider {
public:
  Utf8StringList() { }
  Utf8StringList(std::initializer_list<Utf8String> args)
    : QList<Utf8String>(args) { }
  // explicit Utf8StringList(const Utf8String &item) : QList<Utf8String>({item}) { }
  Utf8StringList(const QList<Utf8String> &list) : QList<Utf8String>(list) { }
  Utf8StringList(const QList<QByteArray> &list)
    : QList<Utf8String>(list.cbegin(), list.cend()) { }
  Utf8StringList(const QList<QString> &list)
    : QList<Utf8String>(list.cbegin(), list.cend()) { }
  Utf8StringList(const QSet<Utf8String> &set)
    : QList<Utf8String>(set.cbegin(), set.cend()) { }
  Utf8StringList(const QSet<QByteArray> &set)
    : QList<Utf8String>(set.cbegin(), set.cend()) { }
  Utf8StringList(const QSet<QString> &set)
    : QList<Utf8String>(set.cbegin(), set.cend()) { }
#if __cpp_concepts >= 201907
  template <std::input_iterator InputIterator>
  Utf8StringList(InputIterator i1, InputIterator i2)
    : QList<Utf8String>(i1, i2) { }
#endif
  [[nodiscard]] Utf8String join(const Utf8String &separator) const;
  [[nodiscard]] Utf8String join(const char separator) const;
  [[nodiscard]] Utf8String headed_join(
      const Utf8String &separator) const;
  [[nodiscard]] Utf8String headed_join(const char separator) const;
  [[nodiscard]] inline QStringList toStringList() const {
    return QStringList(cbegin(), cend()); }
  [[nodiscard]] inline QByteArrayList toByteArrayList() const {
    return QByteArrayList(cbegin(), cend()); }
  [[nodiscard]] Utf8StringSet toSet() const;
  [[nodiscard]] Utf8StringList toSortedDeduplicatedList() const;
  [[nodiscard]] inline std::set<Utf8String> toStdSet() const {
    return std::set<Utf8String>(cbegin(), cend()); }
  /** Return first string as value 1 and so on. Return join(' ') as value 0. */
  [[nodiscard]] QVariant paramRawValue(
      const Utf8String &key, const QVariant &def = {},
      const ParamsProvider::EvalContext &context = {}) const override;
  /** Return integers from 0 to size(). */
  [[nodiscard]] Utf8StringSet paramKeys(
      const ParamsProvider::EvalContext &context = {}) const override;
  using QList<Utf8String>::empty; // hides ParamsProvider::empty
  inline operator QVariant() const {
    return QVariant::fromValue(*this); }
  /** Append if not already in the list. */
  inline Utf8StringList &operator*=(const Utf8String& s) {
    if (!contains(s))
      append(s);
    return *this;
  }
  [[nodiscard]] Utf8String human_readable() const;
};

Q_DECLARE_METATYPE(Utf8StringList)

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const Utf8StringList &l);


LogHelper LIBP6CORESHARED_EXPORT operator<<(LogHelper lh,
                                             const Utf8StringList &list);

/** Const Utf8String list with reverse-mapping index.
 *  e.g. Utf8StringIndexedConstList({ "id", "parent", "name", "color"})
 *          .toIndex().value("name") -> 2 */
class LIBP6CORESHARED_EXPORT Utf8StringIndexedConstList
    : private Utf8StringList {
public:
  Utf8StringIndexedConstList() { }
  Utf8StringIndexedConstList(std::initializer_list<Utf8String> args)
    : Utf8StringList(args) { build_index(); }
  Utf8StringIndexedConstList(const QList<Utf8String> &list)
    : Utf8StringList(list) { build_index(); }
  Utf8StringIndexedConstList(const QList<QByteArray> &list)
    : Utf8StringList(list.cbegin(), list.cend()) { build_index(); }
  Utf8StringIndexedConstList(const QList<QString> &list)
    : Utf8StringList(list.cbegin(), list.cend()) { build_index(); }
  Utf8StringIndexedConstList(const QSet<Utf8String> &set)
    : Utf8StringList(set.cbegin(), set.cend()) { build_index(); }
  Utf8StringIndexedConstList(const QSet<QByteArray> &set)
    : Utf8StringList(set.cbegin(), set.cend()) { build_index(); }
  Utf8StringIndexedConstList(const QSet<QString> &set)
    : Utf8StringList(set.cbegin(), set.cend()) { build_index(); }
#if __cpp_concepts >= 201907
  template <std::input_iterator InputIterator>
  Utf8StringIndexedConstList(InputIterator i1, InputIterator i2)
    : Utf8StringList(i1, i2) { build_index(); }
#endif
  [[nodiscard]] inline QMap<Utf8String,int> toIndex() const {
    return _index; }

  // make only const methods availlable (otherwise index would be inconsistent)
  [[nodiscard]] QList::const_reference back() const {
    return Utf8StringList::back(); }
  [[nodiscard]] QList::const_iterator begin() const {
    return Utf8StringList::begin(); }
  [[nodiscard]] QList::const_pointer data() const {
    return Utf8StringList::data(); }
  [[nodiscard]] QList::const_iterator end() const {
    return Utf8StringList::end(); }
  [[nodiscard]] const Utf8String &first() const {
    return Utf8StringList::first(); }
  [[nodiscard]] QList::const_reference front() const {
    return Utf8StringList::front(); }
  [[nodiscard]] const Utf8String &last() const {
    return Utf8StringList::last(); }
  [[nodiscard]] QList::const_reverse_iterator rbegin() const {
    return Utf8StringList::rbegin(); }
  [[nodiscard]] QList::const_reverse_iterator rend() const {
    return Utf8StringList::rend(); }
  [[nodiscard]] inline QList::const_reference operator[](
      qsizetype i) const {
    return Utf8StringList::operator[](i); }
  [[nodiscard]] inline QList::const_reference at(
      qsizetype i) const {
    return Utf8StringList::at(i); }
  [[nodiscard]] inline auto size() const {
    return Utf8StringList::size(); }
  [[nodiscard]] inline auto count() const {
    return Utf8StringList::count(); }
  [[nodiscard]] inline auto length() const {
    return Utf8StringList::length(); }
  [[nodiscard]] inline auto empty() const {
    return Utf8StringList::empty(); }
  [[nodiscard]] inline auto isEmpty() const {
    return Utf8StringList::isEmpty(); }
  [[nodiscard]] inline Utf8String value(
      qsizetype i, const Utf8String &def = {}) const {
    return Utf8StringList::value(i, def); }
  inline operator QVariant() const {
    return QVariant::fromValue(*this); }
  using Utf8StringList::join;
  using Utf8StringList::headed_join;

private:
  QMap<Utf8String,int> _index;
  Utf8StringIndexedConstList &build_index() {
    QMap<Utf8String,int> index;
    qsizetype n = size();
    for (qsizetype i = 0; i < n; ++i)
      index.insert(operator[](i), i);
    _index = index;
    return *this;
  }
};

Q_DECLARE_METATYPE(Utf8StringIndexedConstList)

#endif // UTF8STRINGLIST_H
