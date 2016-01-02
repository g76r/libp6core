/* Copyright 2016 Hallowyn and others.
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
#ifndef FTPSCRIPT_H
#define FTPSCRIPT_H

#include "libqtssu_global.h"
#include <QSharedDataPointer>
#include <QIODevice>
#include <QByteArray>

class FtpScriptData;
class FtpClient;

/** Sequence of FTP operations.
 * When executed, fails on first failed operation.
 * @see FtpClient
 */
class LIBQTSSUSHARED_EXPORT FtpScript {
  QSharedDataPointer<FtpScriptData> _data;

public:
  enum { DefaultTimeout = 30000 };
  FtpScript(FtpClient *client = 0);
  FtpScript(const FtpScript &);
  ~FtpScript();
  FtpScript &operator=(const FtpScript &);
  FtpClient *client() const;
  int id() const;
  bool execAndWait(int msecs = DefaultTimeout);
  FtpScript &clearCommands();
  FtpScript &connectToHost(QString host, quint16 port = 21);
  FtpScript &login(QString login, QString password);
  FtpScript &cd(QString path);
  FtpScript &get(QString path, QIODevice *dest);
};

#endif // FTPSCRIPT_H
