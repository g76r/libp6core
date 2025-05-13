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
#include "entity.h"
#include "util/utf8stringlist.h"
#include "log/log.h"

namespace p6 {

const int Entity::MetaTypeId = qMetaTypeId<Entity>();

static int staticInit() {
  qMetaTypeId<EntityList>();
  QMetaType::registerConverter<Entity,QVariant>();
  QMetaType::registerConverter<Entity,quint64>();
  QMetaType::registerConverter<Entity,Utf8String>(&Entity::n3);
  QMetaType::registerConverter<Entity,QString>(
        [](const Entity &e) STATIC_LAMBDA { return e.n3(); });
  // QMetaType::registerConverter<QVariant,Entity>(
  //    [](const QVariant &v) STATIC_LAMBDA {
  //   return Entity(v.toULongLong()); });
  QMetaType::registerConverter<EntityList,QVariant>();
  QMetaType::registerConverter<EntityList,Utf8StringList>();
  QMetaType::registerConverter<EntityList,Utf8String>(
        [](const EntityList &v) STATIC_LAMBDA -> Utf8String {
    return v.join(' '); });
  QMetaType::registerConverter<EntityList,QString>(
        [](const EntityList &v) STATIC_LAMBDA -> QString {
    return v.join(' '); });
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

Utf8String EntityList::n3() const {
  // this method is a simplified form of general n3 which is only needed until
  // the whole eg framework is included in libp6core
  Utf8String n3;
  for (auto e: *this)
    n3 = n3+e.n3()+',';
  n3.chop(1);
  return n3;
}

Entity::operator QVariant() const {
  return QVariant::fromValue(*this);
}

EntityList::operator QVariant() const {
  return QVariant::fromValue(*this);
}

EntityList::operator Utf8StringList() const {
  Utf8StringList list;
  for (auto e: *this)
    list += e.n3();
  return list;
}

EntityList EntityList::from_ids(const Utf8StringList &ids) {
  EntityList list;
  for (const auto &utf8: ids)
    if (auto id = utf8.toULongLong(); !!id)
      list += id;
  return list;
}

QDebug operator<<(QDebug dbg, Entity o) {
  return dbg.noquote() << o.n3();
}

log::LogHelper operator<<(log::LogHelper lh, Entity o) {
  return lh << o.n3();
}

QDebug operator<<(QDebug dbg, const EntityList &o) {
  return dbg.noquote() << o.n3();
}

log::LogHelper operator<<(log::LogHelper lh, const EntityList &o) {
  return lh << o.n3();
}

} // p6 ns
