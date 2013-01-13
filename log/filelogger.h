/* Copyright 2012-2013 Hallowyn and others.
 * This file is part of qron, see <http://qron.hallowyn.com/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef FILELOGGER_H
#define FILELOGGER_H

#include <QObject>
#include "logger.h"

class QIODevice;
class QThread;

class LIBQTSSUSHARED_EXPORT FileLogger : public Logger {
  Q_OBJECT
  QIODevice *_device;
  QThread *_thread;
  QString _patternPath, _currentPath;

public:
  /** Takes ownership of the device (= will delete it). */
  explicit FileLogger(QIODevice *device, Log::Severity minSeverity = Log::Info);
  explicit FileLogger(QString path, Log::Severity minSeverity = Log::Info);
  ~FileLogger();
  QString currentPath() const;

protected:
  void doLog(QDateTime timestamp, QString message, Log::Severity severity,
             QString task, QString execId, QString sourceCode);
};

#endif // FILELOGGER_H
