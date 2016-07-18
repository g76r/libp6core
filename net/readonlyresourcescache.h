/* Copyright 2016 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef READONLYRESOURCECACHE_H
#define READONLYRESOURCECACHE_H

#include "libqtssu_global.h"
#include <QObject>
#include <QByteArray>
#include <QUrl>
#include <QHash>
#include <QMutex>
#include <QNetworkAccessManager>

// LATER provide a mean to set maximum cache size
// LATER implement max-age and max-stale for real
// LATER provide an exec: url scheme binded to QProcess (not enabled by default)
// LATER have a way to force refresh (such as HTTP request's max-age=0)

/** Local cache for read-only resources, being them remote (http, ftp...) or
 * local (file).
 */
class LIBQTSSUSHARED_EXPORT ReadOnlyResourcesCache : public QObject {
  Q_OBJECT
  QMutex _mutex;
  QHash<QString,QByteArray> _resources;
  QHash<QString,qint64> _ageTimestamp;
  QHash<QString,qint64> _staleTimestamp;
  QSet<QString> _fetching;
  QHash<QString,QString> _errorStrings;
  QNetworkAccessManager *_nam;
  // LATER make these actual parameters
  qint64 _defaultMaxAge, _defaultMaxStale, _defaultNegativeMaxAge; // in seconds
  qint64 _defaultRequestTimeout;
  bool _shouldHonorHttpCacheMaxAge; // Cache-Control: max-age=42
  bool _shouldHonorHttpCacheMaxStale; // Cache-Control: max-stale=42

public:
  ReadOnlyResourcesCache(QObject *parent = 0);
  /** Fetch a resource (from cache or for real)
   * @param waitForMsecs maximum ms to wait for the resource, must be >= 0
   */
  QByteArray fetchResource(QString pathOrUrl, qint32 waitForMsecs = 1000,
                           QString *errorString = 0);
  QByteArray fetchResource(QString pathOrUrl, QString *errorString = 0) {
    return fetchResource(pathOrUrl, 1000, errorString); }
  /** Fetch a resource, if and only if it is available in cache */
  QByteArray fetchResourceFromCache(
      QString pathOrUrl, bool triggerAsyncFetchingIfNotFound = true);
  /** Clear the cache, but doesn't cancel currently running fetching requests.
   */
  void clear();
  /** defaults to: 60 (1') */
  void setDefaultMaxAge(qint64 secs) { _defaultMaxAge = secs; }
  /** defaults to: 3600 (60') */
  void setDefaultStaleAge(qint64 secs) { _defaultMaxStale = secs; }
  /** defaults to: 60 (1') */
  void setDefaultNegativeMaxAge(qint64 secs) { _defaultNegativeMaxAge = secs; }

private:
  /** must be called by owner thread (because of qnam), locks the mutex */
  Q_INVOKABLE void planResourceFetching(QString pathOrUrl);
  /** locks the mutex */
  void requestFinished(QNetworkReply *reply);
  /** locks the mutex*/
  QString asDebugString();
};

#endif // READONLYRESOURCECACHE_H
