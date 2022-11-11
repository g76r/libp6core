/* Copyright 2017-2022 Hallowyn, Gregoire Barbier and others.
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
#ifndef OBJECTSSTORE_H
#define OBJECTSSTORE_H

#include <QObject>
#include <QSharedData>
#include <QMetaObject>
#include <functional>
#include "libp6core_global.h"
#include <QVariant>

/** Abstract container for persisting QObjects to either local database or
 * network/cloud store in a way their data attributes are their QObject's
 * properties.
 *
 * The objects can be made availlable to UI (e.g. a QML ListView) in a rather
 * straightforward way through ObjectsListModel.
 *
 * Only stored properties are mapped to storage (those which are not declared
 * with "STORED false" in the Q_PROPERTY line), whereas every property, stored
 * or not, is mapped to UI.
 *
 * Only one object class should be stored at once (mixing with subclasses should
 * work whereas being somewhat strange; mixing with any other QObject may work
 * in some cases due to so-called dynamic properties but would be likely to lead
 * to headache).
 *
 * @see ObjectsListModel
 * @see ObjectsSqlStore
 */
class LIBP6CORESHARED_EXPORT ObjectsStore : public QObject {
  Q_OBJECT
  struct ResultData : public QSharedData {
    bool _success;
    QString _code;
    QString _message;
    QObject *_object;
    ResultData() : ResultData(false) { }
    ResultData(bool success, QString code = { }, QString message = { },
               QObject *object = nullptr)
      : _success(success), _code(code), _message(message), _object(object) { }
    ResultData(bool success, QObject *object)
      : ResultData(success, 0, QString(), object) { }
    ResultData(QObject *object) : ResultData(true, object) { }
  };

protected:
  const QMetaObject *_metaobject;

  explicit ObjectsStore(
    const QMetaObject *metaobject, QObject *parent = nullptr)
    : QObject(parent), _metaobject(metaobject) { }

public:
  class Result {
    QSharedDataPointer<ResultData> d;
  public:
    Result() { }
    Result(bool success, QString code = { }, QString message = { })
      : d(new ResultData(success, code, message)) { }
    bool success() const { return d ? d->_success : false; }
    operator bool() const { return success(); }
    QString code() const { return d ? d->_code : QString(); }
    QString message() const { return d ? d->_message : QString(); }
    QObject *object() const { return d ? d->_object : nullptr; }
  };

  /** Apply f to every object in the store.
   * Index is given in call order, without order warranty, and even without the
   * warranty that the order will be the same every time apply() is called. */
  virtual long apply(std::function<void(QObject *object, ObjectsStore *store,
                                        long index)> f) = 0;
  /** Apply f to every object in the store */
  virtual long apply(std::function<void(QObject *object)> f);

public slots:
  /** Create a new object in the store and fetch it.
   * fetched() is emitted
   * @param params: template that can contain some properties set */
  virtual ObjectsStore::Result create(
      const QHash<QString,QVariant> &params = { }) = 0;
  /** Persist an object in the store, i.e. ensure its state is saved. */
  virtual ObjectsStore::Result persist(QObject *object) = 0;
  /** Remove an object from the store and optionally delete it.
   * disposed() is emitted.
   * If required object deletion is done using deleteLater() rather than
   * immediate delete. */
  virtual ObjectsStore::Result dispose(
      QObject *object, bool shouldDelete = true) = 0;
  /** Remove an object from the store and reparent it instead of deleting it.
   * Internally calls dispose(object, false) and thus emit disposed() */
  ObjectsStore::Result withdraw(QObject *object, QObject *newParent);
  /** Fetch initial (all if possible) data and emit fetched() signals to
   * populate connected models */
  virtual ObjectsStore::Result fetch() = 0;

signals:
  /** emitted by fetchAll(), create(), persist() and even spontaneously e.g.
   * if some other program creates or modifies objects, for shared stores */
  void fetched(QObject *object);
  /** emitted by destroy() and spontaneously */
  void disposed(QObject *object);
};

Q_DECLARE_METATYPE(ObjectsStore::Result)

#endif // OBJECTSSTORE_H
