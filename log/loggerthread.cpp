/* Copyright 2014-2016 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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
  while (!isInterruptionRequested()) {
    Logger::LogEntry le;
    if (_logger->_buffer->tryGet(&le, 500))
      _logger->doLog(le);
  }
  //qDebug() << "LoggerThread received stop message" << this << _logger;
  // only connect deleteLater() now because in case of unwanted thread stop,
  // i.e. before Logger calls QThread::requestInterruption(), deleting QThread
  // object would lead to dandling pointer on it in Logger object
  connect(this, &LoggerThread::finished, this, &LoggerThread::deleteLater);
}
