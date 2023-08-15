/* Copyright 2022-2023 Gregoire Barbier and others.
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
#include <signal.h>

class QSocketNotifier;

/** Install Unix signals handlers for specified signals and send Qt
 * signals when they occur.
 * One must specify which signals to receive with addToCatchList() or
 * setCatchList() and connect to signalCaught().
 * Keep in mind that signalCaught() is fired every time a signal on the
 * catch list occurs, including signals another part of the application has
 * subscribed. Connected slot must be able to bear (and ignore) signals
 * numbers it does not want.
 * This is a singleton.
 */
class LIBP6CORESHARED_EXPORT UnixSignalManager : public QObject {
  Q_OBJECT
protected:
  int _pipe[2] { -1, -1 };
  QSocketNotifier *_sn;

  UnixSignalManager();
  void readPipe();

public:
  static UnixSignalManager *instance();
  static void setCatchList(std::initializer_list<int> list);
  static void addToCatchList(std::initializer_list<int> list);

signals:
  void signalCaught(int signal_number);
};

#endif // UNIXSIGNALMANAGER_H
