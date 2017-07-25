/* Copyright 2017 Hallowyn, Gregoire Barbier and others.
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
#include <QString>
#include <QRegularExpression>
#include <QHash>
#include <QMutex>
#include <QDateTime>

class QFileSystemWatcher;

/** Specialized QFileSystemWatcher that only watches directories with enhanced
 * features such as filtering files events using regexp.
 * Uses QFileSystemWatcher internaly.
 * @see QFileSystemWatcher
 */
class LIBPUMPKINSHARED_EXPORT DirectoryWatcher : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(DirectoryWatcher)
  QFileSystemWatcher *_qfsw;
  QHash<QString,QRegularExpression> _watches; // dirname -> filepattern
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
   * thread-safe */
  bool addDirectory(const QString &dirname,
                    const QRegularExpression &filepattern,
                    bool processExistingFilesAsAppearing = false);
  /** Add a directory to watch list, with given regexp filter.
   * E.g. "/tmp", "^a" will watch every file begining with a in /tmp
   * Sets errorString and return false on error.
   * Emit fileAppeared() for preexisting files if
   * processExistingFilesAsAppearing is true.
   * thread-safe */
  bool addDirectory(const QString &dirname, const QString &filepattern,
                    bool processExistingFilesAsAppearing = false) {
    return addDirectory(dirname, QRegularExpression(filepattern),
                        processExistingFilesAsAppearing); }
  /** Add a directory to watch list, watching any file without filter.
   * Sets errorString and return false on error.
   * thread-safe */
  bool addDirectory(const QString &dirname) {
    return addDirectory(dirname, QRegularExpression()); }
  /** Remove directory from watch list
   * Sets errorString and return false on error.
   * thread-safe */
  bool removeDirectory(const QString &dirname);
  /** Remove all directories from watch list
   * Sets errorString and return false on error.
   * thread-safe */
  bool removeAllDirectories();
  /** thread-safe */
  QString errorString() const;

signals:
  void directoryChanged(const QString &dirname);
  void fileAppeared(const QString &path, const QString &dirname,
                    const QString &basename);
  void fileDisappeared(const QString &path, const QString &dirname,
                       const QString &basename);
  void fileChanged(const QString &path, const QString &dirname,
                   const QString &basename);

private:
  void handleDirectoryChanged(const QString &path);
};

#endif // DIRECTORYWATCHER_H
