/* Copyright 2017-2024 Hallowyn, Gregoire Barbier and others.
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
#include "sqlobjectsstore.h"
#include "format/stringutils.h"
#include <QMetaProperty>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QtDebug>

SqlObjectsStore::SqlObjectsStore(const QMetaObject *metaobject,
    QSqlDatabase db, QByteArray tableName,
    QByteArray pkPropName, QObject *parent)
  : ObjectsStore(metaobject, parent), _db(db), _pkPropName(pkPropName) {
  if (tableName.isEmpty())
    tableName = StringUtils::toSnakeCase(metaobject->className())+'s';
  _tableName = tableName;
  int first = metaobject->propertyOffset();
  int count = metaobject->propertyCount();
  for (int i = first; i < count; ++i) {
    QMetaProperty prop = metaobject->property(i);
    if (!prop.isStored()) // MAYDO maybe there are other props to ignore
      continue;
    _storedProperties.append(prop);
    _storedPropertiesByName.insert(prop.name(), prop);
  }
  metaobject = metaObject(); // switching to this' meta object
  count = metaobject->methodCount();
  for (int i = 0; i < count; ++i) {
    const QMetaMethod &method = metaobject->method(i);
    if (method.methodSignature() == "persistSenderSlot()")
      _persistSenderSlot = method;
  }
}

ObjectsStore::Result SqlObjectsStore::create(
    const QHash<QString,QVariant> &params) {
  // TODO sanitize table and keys names
  QSqlQuery query(_db);
  QString sql = "INSERT INTO "+_tableName;
  QStringList keys = params.keys();
  keys.removeAll(_pkPropName); // won't try to set id on creation
  int size = keys.size();
  if (size) {
    std::sort(keys.begin(), keys.end());
    sql = sql+" ("+keys.join(",")+") VALUES (";
    for (int i = 0; i < size; ++i) {
      if (i)
        sql += ",";
      sql += "?";
    }
    sql += ")";
  } else {
    sql += " DEFAULT VALUES";
  }
  query.prepare(sql);
  for (int i = 0; i < size; ++i) {
    query.bindValue(i, params.value(keys.value(i)));
  }
  if (query.exec()) {
    // LATER support other RDBMS than sqlite
    query.prepare(sql = "SELECT last_insert_rowid()");
    if (query.exec() && query.next()) {
      QVariant rowid = query.value(0);
      query.prepare(sql = "SELECT * from "+_tableName+" WHERE rowid = ?");
      query.bindValue(0, rowid);
      if (query.exec() && query.next()) {
        QSqlRecord r = query.record();
        QObject *object = mapToObject(r);
        if (object)
          return Result(object);
      }
    }
  }
  QSqlError error = query.lastError();
  return Result(false, error.nativeErrorCode(),
                error.driverText()+" "+error.databaseText()+" : "+sql);
}

inline QObject *SqlObjectsStore::mapToObject(const QSqlRecord &r) {
  QObject *object = _metaobject->newInstance(Q_ARG(QObject*, this));
  if (!object) {
    qWarning() << "cannot create object by calling"
        << _metaobject->className() << "(QObject *parent) constructor";
    return 0;
  }
  for (int i = 0; i < r.count(); ++i) {
    QByteArray name = r.fieldName(i).toUtf8();
    const QMetaProperty prop = _storedPropertiesByName.value(name);
    if (!prop.isValid()) {
      qWarning() << "fetching database record with unknown column:" << name;
      continue;
    }
    //qDebug() << "    " << name << "=" << r.value(i);
    bool success;
    if (prop.isEnumType()) // QVariant casts to enum only from int or uint
      success = object->setProperty(name, r.value(i).toInt());
    else
      success = object->setProperty(name, r.value(i));
    if (!success)
      qWarning() << "fetching database record denied by QObject for column:"
                 << name << r.value(i);
  }
  for (const QMetaProperty &prop : _storedProperties) {
    const QMetaMethod method = prop.notifySignal();
    if (method.isValid())
      connect(object, method, this, _persistSenderSlot,
              Qt::UniqueConnection);
  }
  QString pk = object->property(_pkPropName).toString();
  if (pk.isEmpty()) {
    qWarning() << "error when fetching object: empty primary key"
               << _pkPropName << "in" << r;
    delete object;
    return 0;
  } else {
    _byPk.insert(pk, object); // useless ?
    emit fetched(object);
    return object;
  }
}

ObjectsStore::Result SqlObjectsStore::fetch() {
  // TODO sanitize table and keys names
  // LATER limit fetched rows count
  QSqlQuery query(_db);
  QString sql = "SELECT * from "+_tableName;
  query.prepare(sql);
  if (query.exec()) {
    while (query.next())
      mapToObject(query.record());
    return Result(true);
  }
  QSqlError error = query.lastError();
  return Result(false, error.nativeErrorCode(),
                error.driverText()+" "+error.databaseText()+" : "+sql);
}

void SqlObjectsStore::persistSenderSlot() {
  // TODO add a buffer when there are several changes in the object at the
  // same time (in the same event loop iteration) to avoid calling persist()
  // this could probably be achieved e.g. using a "QSet<QObject*> _dirties"
  // and calling actual persist() method in next loop iteration, only once
  // per object (or calling a batch persist(QSet<>) once for all objects)
  persist(sender());
}

ObjectsStore::Result SqlObjectsStore::persist(QObject *object) {
  // TODO sanitize table and keys names
  if (!object)
    return Result(false, "null", "null object");
  QVariant pk = object->property(_pkPropName);
  if (!pk.isValid())
    return Result(false, "bad_pk", "invalid primary key");
  QSqlQuery query(_db);
  QString sql = "UPDATE "+_tableName+" SET ";
  const QMetaObject *metaobject = object->metaObject();
  int first = metaobject->propertyOffset();
  int count = metaobject->propertyCount();
  QList<QMetaProperty> props;
  for (int i = first, j = 0; i < count; ++i) {
    QMetaProperty prop = metaobject->property(i);
    if (!prop.isStored())
      continue;
    if (prop.name() == _pkPropName)
      continue;
    props.append(prop);
    if (j)
      sql += ", ";
    sql += QString::fromLatin1(prop.name())+" = ?";
    ++j;
  }
  sql += " WHERE "+_pkPropName+" = ?";
  query.prepare(sql);
  int i = 0;
  for (const QMetaProperty &prop: props) {
    query.bindValue(i, prop.read(object));
    ++i;
  }
  query.bindValue(i, pk);
  if (query.exec()) {
    emit fetched(object);
    return Result(object);
  }
  QSqlError error = query.lastError();
  qWarning() << "cannot update database for object" << _metaobject->className()
             << pk << "error:" << error.nativeErrorCode() << error.driverText()
             << error.databaseText() << "request:" << sql;
  return Result(false, error.nativeErrorCode(),
                error.driverText()+" "+error.databaseText()+" : "+sql);
}

ObjectsStore::Result SqlObjectsStore::dispose(
    QObject *object, bool shouldDelete) {
  // TODO sanitize table and keys names
  if (!object)
    return Result(false, "null", "null object");
  QVariant pk = object->property(_pkPropName);
  if (!pk.isValid())
    return Result(false, "bad_pk", "invalid primary key");
  QSqlQuery query(_db);
  QString sql = "DELETE FROM "+_tableName+" WHERE "+_pkPropName+" = ?";
  query.prepare(sql);
  query.bindValue(0, pk);
  if (query.exec()) {
    disconnect(object, 0, this, 0);
    _byPk.remove(pk.toString());
    emit disposed(object);
    if (shouldDelete)
      object->deleteLater();
    return Result(true);
  }
  QSqlError error = query.lastError();
  return Result(false, error.nativeErrorCode(),
                error.driverText()+" "+error.databaseText()+" : "+sql);
}

long SqlObjectsStore::apply(
    std::function<void(QObject*,ObjectsStore*,long)> f) {
  long i = 0;
  for (QObject *o: _byPk) {
    f(o, this, i);
    ++i;
  }
  return i;
}
