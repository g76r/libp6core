/* Copyright 2016-2017 Hallowyn, Gregoire Barbier and others.
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
#include "readonlyresourcescache.h"
#include <QDateTime>
#include <QThread>
#include <unistd.h>
#include "thread/blockingtimer.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QFile>
#include <QtDebug>
#include <QTimer>
#include "log/log.h"
#include <QMetaObject>

QRegularExpression _startsWithValidUrlSchemeRE("^[a-zA-Z][a-zA-Z0-9+.-]+:");

ReadOnlyResourcesCache::ReadOnlyResourcesCache(QObject *parent) :
  QObject(parent), _nam(new QNetworkAccessManager(this)),
  _defaultMaxAge(60), _defaultMaxStale(3600), _defaultNegativeMaxAge(60),
  _defaultRequestTimeout(30),
  _shouldHonorHttpCacheMaxAge(true), _shouldHonorHttpCacheMaxStale(true) {
  connect(_nam, &QNetworkAccessManager::finished,
          this, &ReadOnlyResourcesCache::requestFinished);
}

QByteArray ReadOnlyResourcesCache::fetchResource(
    QString pathOrUrl, qint32 waitForMsecs, QString *errorString) {
  QByteArray resource = fetchResourceFromCache(pathOrUrl);
  //qDebug() << "ReadOnlyResourcesCache::fetchResource" << pathOrUrl
  //         << waitForMsecs << !!errorString;
  //qDebug().noquote() << asDebugString();
  if (!resource.isNull()) {
    // LATER increment cache hit stats
    return resource;
  }
  // LATER true asynchronous process rather than polling
  // especialy because current behaviour is different depending on the thread
  // - really blocking if not executed by owner thread, hence not depending
  //   on calling code being reentrant
  // - calling QCoreApplication::processEvents() during wait if executed by
  //   owner thread, hence requiring reentrant caller code but supporting
  //   processing QNetworkAccessManager signals callbacks on the same event
  //   loop
  BlockingTimer timer(
        waitForMsecs, 100,
        [this, pathOrUrl]() {
    QMutexLocker ml(&_mutex);
    return _staleTimestamp.value(pathOrUrl)
        >= QDateTime::currentMSecsSinceEpoch();
  }, QThread::currentThread() == thread());
  timer.wait();
  resource = fetchResourceFromCache(pathOrUrl, false);
  // LATER increment cache hit or miss stats
  if (errorString)
    *errorString = _errorStrings.value(pathOrUrl);
  //qDebug() << "threads:" << QThread::currentThread() << thread()
  //         << _nam->thread();
  //qDebug() << "return:" << resource.size();
  //qDebug().noquote() << asDebugString();
  return resource;
}

QByteArray ReadOnlyResourcesCache::fetchResourceFromCache(
    QString pathOrUrl, bool triggerAsyncFetchingIfNotFound) {
  qint64 now = QDateTime::currentMSecsSinceEpoch();
  QByteArray resource;
  QMutexLocker ml(&_mutex);
  if (_staleTimestamp.value(pathOrUrl) >= now) {
    resource = _resources.value(pathOrUrl);
    if (triggerAsyncFetchingIfNotFound
        && _ageTimestamp.value(pathOrUrl) <= now) {
      ml.unlock();
      QMetaObject::invokeMethod(this, "planResourceFetching",
                                Q_ARG(QString, pathOrUrl));
    }
  } else {
    _resources.remove(pathOrUrl);
    _staleTimestamp.remove(pathOrUrl);
    _ageTimestamp.remove(pathOrUrl);
    if (triggerAsyncFetchingIfNotFound) {
      ml.unlock();
      QMetaObject::invokeMethod(this, "planResourceFetching",
                                Q_ARG(QString, pathOrUrl));
    }
  }
  return resource;
}

void ReadOnlyResourcesCache::planResourceFetching(QString pathOrUrl) {
  QMutexLocker ml(&_mutex);
  if (_fetching.contains(pathOrUrl))
    return;
  QString realUrl = _startsWithValidUrlSchemeRE.match(pathOrUrl).hasMatch()
      ? pathOrUrl : "file://"+pathOrUrl;
  QUrl url(realUrl);
  QNetworkRequest request(url);
  request.setAttribute(QNetworkRequest::User, pathOrUrl);
#if QT_VERSION >= 0x050600
  // LATER parametrize follow redirect features
  request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
  request.setMaximumRedirectsAllowed(5);
#endif
  QNetworkReply *reply = _nam->get(request);
  //Log::fatal() << "ReadOnlyResourceCache::planResourceFetching " << pathOrUrl;
  //qDebug() << "reply:" << reply->thread() << reply->parent();
  if (!reply)
    return;
#if QT_VERSION >= 0x050400
  QTimer::singleShot(_defaultRequestTimeout*1000, reply, &QNetworkReply::abort);
#else
  QTimer::singleShot(_defaultRequestTimeout*1000, reply, "abort");
#endif
  // LATER set a maximum data size
  _fetching.insert(pathOrUrl);
  _errorStrings.insert(pathOrUrl, QStringLiteral("Still fetching..."));
}

void ReadOnlyResourcesCache::requestFinished(QNetworkReply *reply) {
  if (!reply)
    return;
  QString pathOrUrl = reply->request().attribute(QNetworkRequest::User)
      .toString();
  //qDebug() << "ReadOnlyResourceCache::requestFinished" << pathOrUrl
  //         << reply->errorString();
  //Log::fatal() << "ReadOnlyResourceCache::requestFinished " << pathOrUrl << " "
  //             << reply->errorString();
  qint64 now = QDateTime::currentMSecsSinceEpoch();
  QByteArray resource = reply->readAll();
  QMutexLocker ml(&_mutex);
  _fetching.remove(pathOrUrl);
  if (reply->error() == QNetworkReply::NoError) {
    _resources.insert(pathOrUrl, resource);
    // LATER implement Cache-Control: max-age
    _ageTimestamp.insert(pathOrUrl, now+_defaultMaxAge*1000);
    // LATER implement Cache-Control: max-stale
    _staleTimestamp.insert(pathOrUrl, now+_defaultMaxStale*1000);
    _errorStrings.remove(pathOrUrl);
  } else {
    _resources.remove(pathOrUrl);
    _ageTimestamp.insert(pathOrUrl, now+_defaultNegativeMaxAge*1000);
    _staleTimestamp.insert(pathOrUrl, now+_defaultNegativeMaxAge*1000);
    // LATER enrich errorString, with e.g. HTTP status
    _errorStrings.insert(pathOrUrl, reply->errorString());
    // LATER plan another fetch on certains conditions (e.g. last failures)
  }
  reply->deleteLater();
}

void ReadOnlyResourcesCache::clear() {
  QMutexLocker ml(&_mutex);
  _resources.clear();
  _ageTimestamp.clear();
  _staleTimestamp.clear();
  // LATER clear _errorStrings but for "Still fetching..." ones
}

QString ReadOnlyResourcesCache::asDebugString() {
  QMutexLocker ml(&_mutex);
  QString s;
  s += "ReadOnlyResourcesCache {\n  resources: {\n";
  foreach (const QString &key, _resources.keys())
    s += "    " + key + ": " + QString::number(_resources.value(key).size())
        + "\n";
  s += "  }\n  age: {\n";
  foreach (const QString &key, _ageTimestamp.keys())
    s += "    " + key + ": "
      + QDateTime::fromMSecsSinceEpoch(_ageTimestamp.value(key)).toString()
      + "\n";
  s += "  }\n  stale: {\n";
  foreach (const QString &key, _staleTimestamp.keys())
    s += "    " + key + ": "
      + QDateTime::fromMSecsSinceEpoch(_staleTimestamp.value(key)).toString()
      + "\n";
  s += "  }\n  fetching: {\n";
  foreach (const QString &key, _fetching)
    s += "    " + key + "\n";
  s += "  }\n  errorstrings: {\n";
  foreach (const QString &key, _errorStrings.keys())
    s += "    " + key + ": " + _errorStrings.value(key) + "\n";
  s += "}\n";
  return s;
}
