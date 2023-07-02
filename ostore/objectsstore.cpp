/* Copyright 2017-2023 Hallowyn, Gregoire Barbier and others.
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
#include "objectsstore.h"

ObjectsStore::Result ObjectsStore::withdraw(
    QObject *object, QObject *newParent) {
  object->setParent(newParent);
  return dispose(object, false);
}

long ObjectsStore::apply(std::function<void(QObject*)> f) {
  return apply([&f](QObject *o, ObjectsStore*, long) { f(o); });
}

ObjectsStore::Result ObjectsStore::create(const QHash<QString,QVariant> &){
  return Result();
}

ObjectsStore::Result ObjectsStore::persist(QObject *) {
  return Result();
}

ObjectsStore::Result ObjectsStore::dispose(QObject *, bool) {
  return Result();
}

ObjectsStore::Result ObjectsStore::fetch() {
  return Result();
}
