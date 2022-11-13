/* Copyright 2022 Gregoire Barbier and others.
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
#ifndef UNIXSIGNALMANAGER_H
#define UNIXSIGNALMANAGER_H

#include "libp6core_global.h"
#include <QObject>
#ifdef Q_OS_UNIX
#include <signal.h>
#endif

class QSocketNotifier;

class LIBP6CORESHARED_EXPORT UnixSignalManager : public QObject {
  Q_OBJECT
protected:
  int _socketpair[2] { -1, -1 };
  QSocketNotifier *_sn;

  UnixSignalManager();
  void readSocketPair();

public:
  ~UnixSignalManager();
  static UnixSignalManager *instance();
  static void setCatchList(std::initializer_list<int> list);
  static void addToCatchList(std::initializer_list<int> list);

signals:
  void signalCaught(int signal_number);
};

#endif // UNIXSIGNALMANAGER_H
