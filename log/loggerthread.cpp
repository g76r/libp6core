/* Copyright 2014-2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "loggerthread.h"

LoggerThread::LoggerThread(Logger *logger)
  : QThread(0), _logger(logger) {
  //qDebug() << "LoggerThread" << QString::number((long)_logger, 16) << _logger->metaObject()->className();
}

LoggerThread::~LoggerThread() {
  //qDebug() << "~LoggerThread" << QString::number((long)_logger, 16) << _logger->metaObject()->className();
  // deleting Logger is safe because LoggerThread::deleteLater is called from
  // within Logger::deleteLater() which is itself called from within
  // MultiplexerLogger::replaceLogger*() only when removing the Logger from
  // loggers list, therefore no log entry can be added to the Logger after that
  delete _logger;
}

void LoggerThread::run() {
  forever {
    Logger::LogEntry le = _logger->_buffer.get();
    if (le.isNull()) // this is a stop message from producer thread
      break;
    _logger->doLog(le);
  }
  deleteLater();
}
