/* Copyright 2014-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef LOGGERTHREAD_H
#define LOGGERTHREAD_H

#include <QThread>
#include "logger.h"

/** Thread class used internally by Logger when working with a dedicated thread.
 * @see Logger */
class LIBPUMPKINSHARED_EXPORT LoggerThread : public QThread {
  Q_OBJECT
  Q_DISABLE_COPY(LoggerThread)
  Logger *_logger;

public:
  LoggerThread(Logger *logger);
  ~LoggerThread();
  void run();
};

#endif // LOGGERTHREAD_H
