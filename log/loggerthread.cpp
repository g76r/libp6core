/* Copyright 2014-2022 Hallowyn, Gregoire Barbier and others.
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

LoggerThread::LoggerThread(Logger *logger)
  : QThread(0), _logger(logger) {
  //qDebug() << "LoggerThread" << QString::number((long)_logger, 16) << _logger->metaObject()->className();
}

LoggerThread::~LoggerThread() {
}

void LoggerThread::run() {
  while (!isInterruptionRequested()) {
    Logger::LogEntry le;
    if (_logger->_buffer->tryGet(&le, 500)) {
      /*fputs(("===LoggerThread got entry "+objectName()+" "
             +QString::number(_logger->_buffer->used())+" "
             +(le.isNull() ? "===STOP===" : le.message())+"\n"
             ).toLatin1(), stderr);
      fflush(stderr);*/
      if (le.isNull()) {
        _logger->doShutdown();
        break;
      }
      _logger->doLog(le);
    }
  }
  //qDebug() << "LoggerThread received stop message" << this << _logger;
  _logger->moveToThread(QCoreApplication::instance()->thread());
  deleteLater();
}
