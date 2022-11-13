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
#include "unixsignalmanager.h"
#include <QMutexLocker>
#include "log/log.h"
#include <QSocketNotifier>
#include <unistd.h>

#ifdef Q_OS_UNIX
//#include <sys/types.h>
//#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#endif

/******************************************************************
  /!\ there must be no global variables with a destructor here /!\
  /!\ because UnixSignalManager must not crash when called     /!\
  /!\ during the program shutdown                              /!\
 ******************************************************************/

namespace {

class UnixSignalManagerImpl : public UnixSignalManager {
public:
  QMutex _list_mutex;
  QList<int> _sig_numbers;

  /// register our handler for signals to catch
  void set_handlers(QList<int> sig_numbers) {
    Q_UNUSED(sig_numbers);
#ifdef Q_OS_UNIX
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    sigset_t block_mask;
    memset(&block_mask, 0, sizeof(sigset_t));
    for (int i: sig_numbers)
      sigaddset(&block_mask, i);
    action.sa_handler = signal_handler;
    action.sa_mask = block_mask;
    action.sa_flags |= SA_RESTART;
    for (int i: sig_numbers) {
      int rc = sigaction(i, &action, 0);
      if (rc)
        Log::error() << "cannot register unix signal handler for signal " << i
                     << " errno: " << errno ;
    }
#endif
  }
  /// register default handler for signals not to catch
  void reset_handlers(QList<int> sig_numbers) {
    Q_UNUSED(sig_numbers);
#ifdef Q_OS_UNIX
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = SIG_DFL;
    for (int i: sig_numbers) {
      int rc = sigaction(i, &action, 0);
      if (rc)
        Log::error() << "cannot reset unix signal handler for signal " << i
                     << " errno: " << errno ;
    }
#endif
  }
  static void signal_handler(int signal_number);
};

static UnixSignalManagerImpl *_instance = nullptr;
static QMutex *_instance_mutex = new QMutex;

[[gnu::unused]] void UnixSignalManagerImpl::signal_handler(int signal_number) {
  char a = signal_number;
  ::write(_instance->_socketpair[1], &a, sizeof(a));
}

} // namespace

UnixSignalManager::~UnixSignalManager() {
  qDebug() << "~UnixSignalManager() -*-";
}

UnixSignalManager::UnixSignalManager() {
#ifdef Q_OS_UNIX
  //if (::socketpair(AF_UNIX, SOCK_STREAM, 0, _socketpair))
  if (::pipe2(_socketpair, O_NONBLOCK))
    Log::error() << "UnixSignalManager could not create socketpair, errno: "
                 << "errno";
#endif
  _sn = new QSocketNotifier(_socketpair[0], QSocketNotifier::Read, this);
  connect(_sn, &QSocketNotifier::activated,
          this, &UnixSignalManager::readSocketPair, Qt::QueuedConnection);
  /*Log::debug() << "socket notifier: " << _socketpair[0] << " " << _socketpair[1]
               << " " << QThread::currentThread() << " / "
               << " " << _sn << " " << _sn->thread()
               << " / " << this->thread() << " " << this << " "
               << _sn->isEnabled() << " " << _sn->isValid();*/
}

void UnixSignalManager::readSocketPair() {
  _sn->setEnabled(false);
  int r = 0;
  forever {
    char a;
    r = ::read(_socketpair[0], &a, sizeof(a));
    if (r <= 0)
      break;
    int signal_number = a;
    emit signalCaught(signal_number);
  }
  _sn->setEnabled(true);
}

UnixSignalManager *UnixSignalManager::instance() {
  QMutexLocker locker(_instance_mutex);
  if (!_instance)
    _instance = new UnixSignalManagerImpl();
  return _instance;
}

void UnixSignalManager::setCatchList(std::initializer_list<int> list) {
  UnixSignalManager::instance();
  //Log::debug() << "setCatchList " << _instance;
  QMutexLocker locker(&_instance->_list_mutex);
  auto old_numbers = _instance->_sig_numbers;
  _instance->_sig_numbers.clear();
  for (int i : list) {
    if (_instance->_sig_numbers.contains(i))
      continue;
    _instance->_sig_numbers.append(i);
  }
  std::sort(_instance->_sig_numbers.begin(), _instance->_sig_numbers.end());
  if (old_numbers != _instance->_sig_numbers) {
    _instance->reset_handlers(old_numbers);
    _instance->set_handlers(_instance->_sig_numbers);
  }
}

void UnixSignalManager::addToCatchList(std::initializer_list<int> list) {
  UnixSignalManager::instance();
  //Log::debug() << "addToCatchList " << _instance;
  QMutexLocker locker(&_instance->_list_mutex);
  bool changed = false;
  for (int i : list) {
    if (_instance->_sig_numbers.contains(i))
      continue;
    //Log::debug() << "append " << i;
    _instance->_sig_numbers.append(i);
    changed = true;
  }
  std::sort(_instance->_sig_numbers.begin(), _instance->_sig_numbers.end());
  if (changed)
    _instance->set_handlers(_instance->_sig_numbers);
}
