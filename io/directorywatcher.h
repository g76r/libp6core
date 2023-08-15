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
#ifndef DIRECTORYWATCHER_H
#define DIRECTORYWATCHER_H

#include "libp6core_global.h"
#include <QObject>
#include "util/utf8stringset.h"
#include <QRegularExpression>
#include <QMutex>
#include <QDateTime>

class QFileSystemWatcher;

/** Specialized QFileSystemWatcher that only watches directories with enhanced
 * features such as filtering files events using regexp.
 * Uses QFileSystemWatcher internaly.
 * @see QFileSystemWatcher
 */
class LIBP6CORESHARED_EXPORT DirectoryWatcher : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(DirectoryWatcher)
  QFileSystemWatcher *_qfsw;
  QHash<QString,QList<QRegularExpression>> _watches; // dirname -> filepatterns (last inserted last in vector order)
  QHash<QString,QHash<QString,QDateTime>> _files; // dirname -> (basename,lastmodified)
  mutable QMutex _mutex;
  QString _errorString;

public:
  explicit DirectoryWatcher(QObject *parent = 0);
  /** Add a directory to watch list, with given regexp filter.
   * E.g. "/tmp", "^a" will watch every file begining with a in /tmp
   * Sets errorString and return false on error.
   * Emit fileAppeared() for preexisting files if
   * processExistingFilesAsAppearing is true.
   * Do nothing if the watch already exists.
   * thread-safe */
  bool addWatch(const QString &dirname,
                const QRegularExpression &filepattern,
                bool processExistingFilesAsAppearing = false);
  /** Add a directory to watch list, with given regexp filter.
   * E.g. "/tmp", "^a" will watch every file begining with a in /tmp
   * Sets errorString and return false on error.
   * Emit fileAppeared() for preexisting files if
   * processExistingFilesAsAppearing is true.
   * Do nothing if the watch already exists.
   * thread-safe */
  bool addWatch(const QString &dirname, const QString &filepattern,
                bool processExistingFilesAsAppearing = false) {
    return addWatch(dirname, QRegularExpression(filepattern),
                    processExistingFilesAsAppearing); }
  /** Add a directory to watch list, watching any file without filter.
   * Sets errorString and return false on error.
   * Do nothing if the watch already exists.
   * thread-safe */
  bool addDirectory(const QString &dirname) {
    return addWatch(dirname, QRegularExpression()); }
  /** Remove a watch from watch list
   * Sets errorString and return false on error.
   * thread-safe */
  bool removeWatch(const QString &dirname,
                   const QRegularExpression &filepattern);
  /** Remove a watch from watch list
   * Sets errorString and return false on error.
   * thread-safe */
  bool removeWatch(const QString &dirname,
                   const QString &filepattern) {
    return removeWatch(dirname, QRegularExpression(filepattern)); }
  /** Remove whole directory (every watches) from watch list
   * Sets errorString and return false on error.
   * thread-safe */
  bool removeDirectory(const QString &dirname);
  /** Remove all directories from watch list
   * Sets errorString and return false on error.
   * thread-safe */
  bool removeAllWatches();
  /** Give error message of last error.
   * Thread-safe since it won't crash if called by several threads, but without
   * guarantee that it's the right "last" message if several threads call
   * addXxx() and removeXxx() methods at the same time.
   * Actually DirectoryWatcher's thread-safety is mainly a matter of allowing
   * several threads one for configuration operations and as many as needed for
   * slots connected to its signals. */
  QString errorString() const;

signals:
  void directoryChanged(const QString &dirname);
  void fileAppeared(const QString &path, const QString &dirname,
                    const QString &basename,
                    const QRegularExpression &filepattern);
  void fileDisappeared(const QString &path, const QString &dirname,
                       const QString &basename,
                       const QRegularExpression &filepattern);
  void fileChanged(const QString &path, const QString &dirname,
                   const QString &basename,
                   const QRegularExpression &filepattern);

private:
  void handleDirectoryChanged(const QString &path);
};

#endif // DIRECTORYWATCHER_H
