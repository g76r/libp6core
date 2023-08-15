/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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
#ifndef FILELOGGER_H
#define FILELOGGER_H

#include "logger.h"

class QIODevice;
class QThread;

class LIBP6CORESHARED_EXPORT FileLogger : public Logger {
  Q_OBJECT
  Q_DISABLE_COPY(FileLogger)
  QIODevice *_device;
  QString _pathPattern, _currentPath;
  QDateTime _lastOpen;
  int _secondsReopenInterval;
  bool _buffered;

public:
  /** Takes ownership of the device (= will delete it).
    * @param buffered only applies if device is not already open */
  explicit FileLogger(QIODevice *device, Log::Severity minSeverity = Log::Info,
                      bool buffered = true);
  explicit FileLogger(QString pathPattern,
                      Log::Severity minSeverity = Log::Info,
                      int secondsReopenInterval = 300,
                      bool buffered = true);
  ~FileLogger();
  Utf8String currentPath() const override;
  Utf8String pathPattern() const override;

protected:
  void doLog(const LogEntry &entry) override;
  void doShutdown() override;
};

#endif // FILELOGGER_H
