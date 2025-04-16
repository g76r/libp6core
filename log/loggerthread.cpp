/* Copyright 2014-2025 Hallowyn, Gregoire Barbier and others.
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
#include "loggerthread.h"
#include <QCoreApplication>

using namespace std::chrono_literals;

namespace p6::log {

LoggerThread::LoggerThread(Logger *logger)
  : QThread(0), _logger(logger) {
  connect(this, &QThread::finished, logger, [this](){
    _logger->moveToThread(this->thread()); // most often == main thread
    // _logger->moveToThread(QCoreApplication::instance()->thread());
    _logger->deleteLater();
  });
  connect(_logger, &QObject::destroyed, this, &QObject::deleteLater);
}

LoggerThread::~LoggerThread() {
  fflush(stdout);
}

void LoggerThread::run() {
  while (!isInterruptionRequested()) {
    Record record;
    if (_logger->_buffer->tryGet(&record, 500ms)) {
      if (!record) {
        _logger->do_shutdown();
        break;
      }
      _logger->do_log(record);
    }
  }
}

} // ns p6
