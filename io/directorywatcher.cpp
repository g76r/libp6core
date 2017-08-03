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
#include "directorywatcher.h"
#include <QFileSystemWatcher>
#include <QMutexLocker>
#include <QSet>
//#include <QtDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

DirectoryWatcher::DirectoryWatcher(QObject *parent)
  : QObject(parent), _qfsw(new QFileSystemWatcher(this)) {
  connect(_qfsw, &QFileSystemWatcher::directoryChanged,
          this, &DirectoryWatcher::handleDirectoryChanged);
}

bool DirectoryWatcher::addWatch(
    const QString &dirname, const QRegularExpression &filepattern,
    bool processExistingFilesAsAppearing) {
  QMutexLocker locker(&_mutex);
  QDir dir(dirname);
  QFileInfo fi(dirname);
  if (!fi.isDir()) {
    _errorString = tr("DirectoryWatcher: directory %1 not found").arg(dirname);
    return false;
  }
  if (!dir.isReadable()) {
    _errorString = tr("DirectoryWatcher: directory %1 not readable")
        .arg(dirname);
    return false;
  }
  // LATER check & warn for identical directories registred with different paths due to symlinks or multiple mounts
  if (!_watches.contains(dirname)) {
    if (!_qfsw->addPath(dirname)) {
      _errorString = tr("DirectoryWatcher: cannot subscribe to system events"
                        "on directory %1").arg(dirname);
      return false;
    }
  }
  if (!_watches[dirname].contains(filepattern)) {
    _watches[dirname].append(filepattern);
    dir.setFilter(QDir::Files|QDir::Hidden);
    QHash<QString,QDateTime> &files = _files[dirname];
    for (const QFileInfo &fi : dir.entryInfoList()) {
      const QString basename = fi.fileName();
      if (!filepattern.match(basename).hasMatch())
        continue;
      //qDebug() << "  fileInit" << fi.filePath() << dirname << basename;
      files.insert(basename, fi.lastModified());
      if (processExistingFilesAsAppearing) {
        //qDebug() << "  fileAppeared" << fi.filePath() << dirname << basename;
        emit fileAppeared(fi.filePath(), dirname, basename, filepattern);
      }
    }
  }
  _errorString = QString();
  return true;
}

bool DirectoryWatcher::removeWatch(const QString &dirname,
                                   const QRegularExpression &filepattern) {
  QMutexLocker locker(&_mutex);
  if (!_watches.value(dirname).contains(filepattern)) {
    _errorString = tr("DirectoryWatcher: cannot remove unknown watch %1 %2")
        .arg(dirname).arg(filepattern.pattern());
    return false;
  }
  _watches[dirname].removeAll(filepattern);
  if (_watches[dirname].size()) {
    _watches.remove(dirname);
    if (!_qfsw->removePath(dirname)) {
      _errorString = tr("DirectoryWatcher: cannot unsubscribe from system "
                        "events on directory %1").arg(dirname);
      return false;
    }
  }
  _errorString = QString();
  return true;
}

bool DirectoryWatcher::removeDirectory(const QString &dirname) {
  QMutexLocker locker(&_mutex);
  if (!_watches.contains(dirname)) {
    _errorString = tr("DirectoryWatcher: cannot remove unknown directory %1")
        .arg(dirname);
    return false;
  }
  _watches.remove(dirname);
  if (!_qfsw->removePath(dirname)) {
    _errorString = tr("DirectoryWatcher: cannot unsubscribe from system events"
                      "on directory %1").arg(dirname);
    return false;
  }
  _errorString = QString();
  return true;
}

bool DirectoryWatcher::removeAllWatches() {
  QMutexLocker locker(&_mutex);
  _errorString = QString();
  for (const QString &dirname : _watches.keys()) {
    _watches.remove(dirname);
    if (!_qfsw->removePath(dirname)) {
      if (_errorString.isNull())
        _errorString = tr("DirectoryWatcher: cannot unsubscribe from system events"
                          "on directories: %1").arg(dirname);
      else
        _errorString.append(" ").append(dirname);
    }
  }
  return _errorString.isNull();
}

QString DirectoryWatcher::errorString() const {
  QMutexLocker locker(&_mutex);
  return _errorString;
}

void DirectoryWatcher::handleDirectoryChanged(const QString &dirname) {
  QMutexLocker locker(&_mutex);
  //qDebug() << "handleDirectoryChanged" << dirname;
  QDir dir(dirname);
  dir.setFilter(QDir::Files|QDir::Hidden);
  QHash<QString,QDateTime> &files = _files[dirname];
  QSet<QString> newFiles;
  for (const QFileInfo &fi : dir.entryInfoList()) {
    const QString basename = fi.fileName();
    newFiles.insert(basename);
    if (files.contains(basename)) {
      const QDateTime oldLastModified = files.value(basename);
      if (oldLastModified != fi.lastModified()) {
        for (const QRegularExpression &filepattern: _watches.value(dirname)) {
          if (filepattern.match(basename).hasMatch()) {
            //qDebug() << "  fileChanged" << fi.filePath() << dirname << basename;
            //qDebug() << "    " << oldLastModified << fi.lastModified();
            emit fileChanged(fi.filePath(), dirname, basename, filepattern);
            files.insert(basename, fi.lastModified());
            goto found;
          }
        }
        files.remove(basename); // file no longer matched by any pattern
        found:;
      }
    } else {
      for (const QRegularExpression &filepattern: _watches.value(dirname)) {
        if (filepattern.match(basename).hasMatch()) {
          //qDebug() << "  fileAppeared" << fi.filePath() << dirname << basename;
          emit fileAppeared(fi.filePath(), dirname, basename, filepattern);
          files.insert(basename, fi.lastModified());
        }
      }
    }
  }
  for (const QString &basename : files.keys()) {
    if (!newFiles.contains(basename)) {
      files.remove(basename);
      for (const QRegularExpression &filepattern: _watches.value(dirname)) {
        if (filepattern.match(basename).hasMatch()) {
          //qDebug() << "  fileDisappeared" << dirname+QDir::separator()+basename
          //         << dirname << basename;
          emit fileDisappeared(dirname+QDir::separator()+basename, dirname,
                               basename, filepattern);
          break;
        }
      }
    }
  }
  emit directoryChanged(dirname);
}
