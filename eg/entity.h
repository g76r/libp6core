/* Copyright 2024-2025 Gregoire Barbier and others.
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
#ifndef ENTITY_H
#define ENTITY_H

#include "util/utf8string.h"

class Utf8StringList;

namespace p6 {

namespace log {
class LogHelper;
}
class World;
class WorldView;

/** Entity.
 *  Just a 64 bits id. Every information comes from subject-predicate-object
 *  triplets stored in the world, subjects, predicates and often objects being
 *  entities.
 *  The entity does not exists by itself, only the triplets exist, so an entity
 *  with no triplet (no attribute, no tag, no relation) just does not exist.
 *  @see World
 *  @see EntityRef
 *  @see Triplet
 */
class LIBP6CORESHARED_EXPORT Entity {
public:
  quint64 _id;
  const static int MetaTypeId;
  const static quint64
  /// attribute acting as a string identifier
  NAME = 1,
  /// relation of inheritance e.g. apple,$kind_of,fruit
  /// $kind_of graph is used by default to provide inheritance in e.g.
  /// find_first_attribute() or %-eval
  KIND_OF = 2,
  /// predicate whose object is a value (QVariant) e.g. $name
  /// cannot be multi-valued (one must handle multivalue by themselves, e.g.
  /// setting value as a QList or a value separated string)
  ATTRIBUTE = 3,
  /// predicate whose object is another entity e.g. $kind_of
  /// can be multi-valued
  RELATION = 4,
  /// $tag: predicate whose object is a value (QVariant)
  /// can be multi-valued so that an entity has several tags
  /// e.g. apple123,$tag,main apple123,$tag,visible
  /// but the same tag can be set only once
  /// e.g. its impossible to have apple123,$tag,visible apple123,$tag,visible
  TAG = 5,
  /// relation guaranteeing that removing subject will remove objects
  OWNS = 6,
  /// relation of instanciation e.g. apple123,$instance_of,apple
  INSTANCE_OF = 7,
  /// relation guaranteeing that instanciating subject will add
  /// object as $kind_of the instance, e.g. apple,$grants_kind,0x2a2a will
  /// make auto create apple123,$kind_of,0x2a2a on calling
  /// declare_kind_of(apple123,apple)
  GRANTS_KIND = 8,
  /// reserved value (not actual entity) use as inheritance param to find_xxx
  /// methods in order to obtain $instance_of-then-$kind_of inheritance search
  INSTANCE_THEN_KIND_OF = 0xffe,
  /// applications can use entities > LAST_RESERVED
  LAST_RESERVED = 0xfff;

  const static int ENTITY_ROLE = Qt::UserRole;
  const static int ENTITY_NAME_ROLE = Qt::UserRole+1;

  inline Entity(quint64 id = 0) : _id(id) {}
  inline quint64 id() const { return _id; }
  [[nodiscard]] inline Utf8String n3() const {
    return "0x"_u8+Utf8String::number(_id, 16); }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
  [[nodiscard]] __attribute__((dllexport)) Utf8String n3(const WorldView *wv) const;
#pragma GCC diagnostic pop
  inline operator quint64() const { return _id; }
  inline bool operator!() const { return !_id; }
  operator QVariant() const;
  friend inline std::strong_ordering operator<=>(
      const Entity &x, const Entity &y) { return x._id <=> y._id; }
  friend inline std::strong_ordering operator<=>(
      const Entity &x, quint64 y) { return x._id <=> y; }
  friend inline bool operator==(const Entity &x, const Entity &y) {
    return x._id == y._id; }
  friend inline bool operator==(const Entity &x, quint64 y) {
    return x._id == y; }
  /** syntaxic sugar for constant init */
  inline Entity operator+(quint64 incr) const { return _id+incr; }
  inline Entity &coalesce(const Entity &that) {
    return !*this ? operator=(that) : *this; }
  inline Entity &coalesce(quint64 that) {
    return !*this ? operator=(that) : *this; }
  inline Entity &operator|=(const Entity &that) { return coalesce(that); }
  inline Entity &operator|=(quint64 that) { return coalesce(that); }
};

/** Null coalesce operator */
inline Entity operator|(const Entity a1, const Entity a2) {
  return !!a1 ? a1 : a2; }
inline Entity operator|(const Entity a1, quint64 a2) {
  return !!a1 ? a1 : Entity{a2}; }
inline Entity operator|(quint64 a1, const Entity a2) {
  return !!a1 ? Entity{a1} : a2; }

class LIBP6CORESHARED_EXPORT EntityList : public QList<Entity> {
public:
  EntityList() { }
  EntityList(std::initializer_list<Entity> args) : QList(args) { }
  EntityList(const Entity &entity) : QList({entity}) { }
  EntityList(const QList<Entity> &list) : QList(list) { }
  EntityList(const QSet<Entity> &set)
    : QList(set.cbegin(), set.cend()) { }
  EntityList(std::input_iterator auto i1, std::input_iterator auto i2)
    : QList(i1, i2) { }
  inline EntityList sorted() const {
    EntityList sorted = *this;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
  }
  operator QVariant() const;
  operator Utf8StringList() const;
  [[nodiscard]] Utf8String join(const Utf8String &separator) const {
    Utf8String s;
    for (auto e: *this)
      s = s + e.n3() + separator;
    s.chop(separator.size());
    return s;
  }
  [[nodiscard]] Utf8String join(const char separator) const {
    Utf8String s;
    for (auto e: *this)
      s = s + e.n3() + separator;
    s.chop(1);
    return s;
  }
  // [[nodiscard]] Utf8String n3(
  //     const Utf8String &separator, const WorldView *wv = 0,
  //     bool decorate = true) const;
  [[nodiscard]] Utf8String n3() const;
  // [[nodiscard]] inline Utf8String n3(
  //     const WorldView *wv, bool decorate = true) const {
  //   return n3(","_u8, wv, decorate); }
  [[nodiscard]] Utf8String headed_join(const Utf8String &separator) const {
    Utf8String s;
    for (auto e: *this)
      s = s + separator + e.n3();
    return s;
  }
  [[nodiscard]] Utf8String headed_join(const char separator) const {
    return headed_join(Utf8String{separator}); }
  /** Append if not already in the list. */
  inline EntityList &operator*=(Entity e) {
    if (!contains(e))
      append(e);
    return *this;
  }
  /** names won't be resolved, only hexadecimal ids */
  static EntityList from_ids(const Utf8StringList &ids);
};

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, Entity o);

log::LogHelper LIBP6CORESHARED_EXPORT operator<<(log::LogHelper lh, Entity o);

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const EntityList &o);

log::LogHelper LIBP6CORESHARED_EXPORT operator<<(log::LogHelper lh, const EntityList &o);

} // p6 ns

Q_DECLARE_METATYPE(p6::Entity)
Q_DECLARE_TYPEINFO(p6::Entity, Q_RELOCATABLE_TYPE);

inline uint qHash(const p6::Entity &i) { return qHash(i.id()); }

Q_DECLARE_METATYPE(p6::EntityList)
Q_DECLARE_TYPEINFO(p6::EntityList, Q_RELOCATABLE_TYPE);

#endif // ENTITY_H
