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

#include "io/directorywatcher.h"
#include <QtDebug>
#include <QCoreApplication>
#include <QTimer>

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  DirectoryWatcher dw;
  QObject::connect(&dw, &DirectoryWatcher::directoryChanged,
                   [](const QString &path) {
    qDebug() << "directoryChanged" << path;
  });
  QObject::connect(&dw, &DirectoryWatcher::fileAppeared,
                   [](const QString &path, const QString &dirname,
                   const QString &basename) {
    qDebug() << "fileAppeared" << path << dirname << basename;
  });
  QObject::connect(&dw, &DirectoryWatcher::fileDisappeared,
                   [](const QString &path, const QString &dirname,
                   const QString &basename) {
    qDebug() << "fileDisappeared" << path << dirname << basename;
  });
  QObject::connect(&dw, &DirectoryWatcher::fileChanged,
                   [](const QString &path, const QString &dirname,
                   const QString &basename) {
    qDebug() << "fileChanged" << path << dirname << basename;
  });
  system("touch /tmp/4");
  system("touch /tmp/3");
  dw.addDirectory("/tmp", "^4", true);
  dw.addDirectory("/tmp", "^3");
  QTimer::singleShot(3000, &app, &QCoreApplication::quit);
  QTimer::singleShot(1000, []() {
      system("touch /tmp/4");
      system("touch /tmp/3");
      system("touch /tmp/44");
      system("touch /tmp/33");
  });
  qDebug() << "---";
  app.exec();
}
