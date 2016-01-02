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
  //int id() const;
  // LATER int lastResultCode() const
  bool execAndWait(int msecs = DefaultTimeout);
  FtpScript &clearCommands();
  FtpScript &connectToHost(QString host, quint16 port = 21);
  FtpScript &login(QString login, QString password);
  FtpScript &cd(QString path);
  FtpScript &mkdir(QString path);
  FtpScript &mkdirIgnoringFailure(QString path);
  FtpScript &rmdir(QString path);
  FtpScript &rmdirIgnoringFailure(QString path);
  FtpScript &rm(QString path);
  FtpScript &rmIgnoringFailure(QString path);
  FtpScript &ls(QStringList *basenames, QString path = ".");
  // LATER lsLong(QList<FtpFileInfo>*, path)
  // FtpFileInfo: { QString basename, QString path, QDateTime mtime, qint64 size }
  // using NLST, PWD or memorized pwd from last login() or cd(), MDTM, SIZE
  FtpScript &get(QString path, QIODevice *dest);
  FtpScript &get(QString path, QByteArray *dest);
  FtpScript &get(QString path, QString localPath);
  FtpScript &get(QString path, const char *localPath) {
    return get(path, QString(localPath));
  }
  FtpScript &put(QString path, QIODevice *source);
  FtpScript &put(QString path, QByteArray source);
  FtpScript &put(QString path, QString localPath);
  FtpScript &put(QString path, const char *localPath) {
    return put(path, QString(localPath));
  }
};

#endif // FTPSCRIPT_H
