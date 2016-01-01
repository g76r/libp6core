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
#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include "ftpscript.h"

class FtpClient : public QObject {
  friend class FtpScript;
  Q_OBJECT

public:
  enum FtpError { NoError, Error };

private:
  Q_DISABLE_COPY(FtpClient)
  QTcpSocket *_controlSocket, *_transferSocket;
  FtpError _error;
  QString _errorString;

public:
  explicit FtpClient(QObject *parent = 0);
  FtpScript script() { return FtpScript(this); }
  FtpError error() const { return _error; }
  QString errorString() const { return _errorString; }
  bool connectToHost(QString host, quint16 port,
                     int msecs = FtpScript::DefaultTimeout) {
    return script().connectToHost(host, port).execAndWait(msecs);
  }

signals:
  void	connected();
  void	disconnected();
  // TODO more signals
};

#endif // FTPCLIENT_H
