/* Copyright 2023-2024 Gregoire Barbier and others.
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
#ifndef LIBP6CORE_STABLE_H
#define LIBP6CORE_STABLE_H

// C
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>

#if defined __cplusplus

// C++
#include <climits>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <cmath>
#include <set>
#include <bit>

// C++ source_location depending on compiler version
#if __has_include(<source_location>)
#include <source_location>
using std::source_location;
#elif __has_include(<experimental/source_location>)
#include <experimental/source_location>
using std::experimental::source_location;
#endif
#include <bit>

// Qt
#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QAbstractSocket>
#include <QAtomicInt>
#include <QBuffer>
#include <QCache>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QExplicitlySharedDataPointer>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHash>
#include <QHostAddress>
#include <QIdentityProxyModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QMetaObject>
#include <QMetaProperty>
#include <QMimeData>
#include <QModelIndex>
#include <QMultiHash>
#include <QMutex>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPair>
#include <QPartialOrdering>
#include <QPointer>
#include <QProcess>
#include <QRandomGenerator>
#include <QRect>
#include <QRecursiveMutex>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSaveFile>
#include <QSemaphore>
#include <QSet>
#include <QSharedData>
#include <QSharedDataPointer>
#include <QSocketNotifier>
#include <QSortFilterProxyModel>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStack>
#include <QString>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtDebug>
#include <QTemporaryFile>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QTimeZone>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QWaitCondition>

#endif // __cplusplus

#endif // LIBP6CORE_STABLE_H



