/* Copyright 2023 Gregoire Barbier and others.
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
  Utf8String join(const Utf8String &separator) const;
  Utf8String join(const char separator) const;
  QStringList toStringList() const {
    return QStringList(cbegin(), cend()); }
  QByteArrayList toByteArrayList() const {
    return QByteArrayList(cbegin(), cend()); }
  Utf8StringSet toSet() const;
  inline Utf8StringList toSortedDeduplicatedList() const;
  std::set<Utf8String> toStdSet() const {
    return std::set<Utf8String>(cbegin(), cend()); }
  /** Return first string as value 1 and so on. Return join(' ') as value 0. */
  QVariant paramRawValue(
      const Utf8String &key, const QVariant &def = {},
      const ParamsProvider::EvalContext &context = {}) const override;
  /** Return numbers from 0 to size(). */
  Utf8StringSet paramKeys(
      const ParamsProvider::EvalContext &context = {}) const override;
};

Q_DECLARE_METATYPE(Utf8StringList)

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const Utf8StringList &l);


LogHelper LIBP6CORESHARED_EXPORT operator<<(LogHelper lh,
                                             const Utf8StringList &list);

/** Const Utf8String list with reverse-mapping index.
 *  e.g. Utf8StringIndexedConstList({ "id", "parent", "name", "color"})
 *          .toIndex().value("name") -> 2 */
class LIBP6CORESHARED_EXPORT Utf8StringIndexedConstList
    : public Utf8StringList {
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
  QMap<Utf8String,int> toIndex() const { return _index; }

  // make all non-const methods unavaillable (since index would be inconsistent)
  void append() = delete;
  QList::const_reference back() const { return Utf8StringList::back(); }
  QList::const_iterator begin() const { return Utf8StringList::begin(); }
  void clear() = delete;
  QList::const_pointer data() const { return Utf8StringList::data(); }
  void emplace() = delete;
  void emplaceBack() = delete;
  void emplace_back() = delete;
  QList::const_iterator end() const { return Utf8StringList::end(); }
  void erase() = delete;
  void fill() = delete;
  const Utf8String &first() const { return Utf8StringList::first(); }
  QList::const_reference front() const { return Utf8StringList::front(); }
  void insert() = delete;
  const Utf8String &last() const { return Utf8StringList::last(); }
  void move() = delete;
  void pop_back() = delete;
  void pop_front() = delete;
  void prepend() = delete;
  void push_back() = delete;
  void push_front() = delete;
  QList::const_reverse_iterator rbegin() const {
    return Utf8StringList::rbegin(); }
  void remove() = delete;
  void removeAll() = delete;
  void removeAt() = delete;
  void removeFirst() = delete;
  void removeIf() = delete;
  void removeLast() = delete;
  void removeOne() = delete;
  QList::const_reverse_iterator rend() const { return Utf8StringList::rend(); }
  void replace() = delete;
  void reserve() = delete;
  void resize() = delete;
  void shrink_to_fit() = delete;
  void squeeze() = delete;
  void swapItemsAt() = delete;
  void takeAt() = delete;
  void takeFirst() = delete;
  void takeLast() = delete;
  void operator+(int) = delete;
  void operator+=(int) = delete;
  void operator|(int) = delete;
  void operator|=(int) = delete;
  void operator<<(int) = delete;
  QList::const_reference operator[](qsizetype i) const {
    return Utf8StringList::operator[](i); }

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
