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
#ifndef DUMMYSOCKET_H
#define DUMMYSOCKET_H

#include "libqtssu_global.h"
#include <QAbstractSocket>

/** Dummy network socket without anything to read from and always ready to write
 * to. Kind of network /dev/null.
 */
class LIBQTSSUSHARED_EXPORT DummySocket : public QAbstractSocket {
  Q_OBJECT
  Q_DISABLE_COPY(DummySocket)

public:
  DummySocket(QObject *parent = 0);
  /** Return a global singleton instance. Of course it must not be deleted by
   * caller. */
  static DummySocket *singletonInstance();

  // QIODevice interface
public:
  bool isSequential() const override;
  bool open(OpenMode mode) override;
  void close() override;
  qint64 pos() const override;
  qint64 size() const override;
  bool seek(qint64 pos) override;
  bool atEnd() const override;
  bool reset() override;
  qint64 bytesAvailable() const override;
  qint64 bytesToWrite() const override;
  bool canReadLine() const override;
  bool waitForReadyRead(int msecs) override;
  bool waitForBytesWritten(int msecs) override;

protected:
  qint64 readData(char *data, qint64 maxlen) override;
  qint64 readLineData(char *data, qint64 maxlen) override;
  qint64 writeData(const char *data, qint64 len) override;

  // QAbstractSocket interface
public:
  void resume() override;
  void connectToHost(const QString &hostName, quint16 port, OpenMode mode, NetworkLayerProtocol protocol) override;
  void connectToHost(const QHostAddress &address, quint16 port, OpenMode mode) override;
  void disconnectFromHost() override;
  void setReadBufferSize(qint64 size) override;
  qintptr socketDescriptor() const override;
  bool setSocketDescriptor(qintptr socketDescriptor, SocketState state, OpenMode openMode) override;
  void setSocketOption(QAbstractSocket::SocketOption option, const QVariant &value) override;
  QVariant socketOption(QAbstractSocket::SocketOption option) override;
  bool waitForConnected(int msecs) override;
  bool waitForDisconnected(int msecs) override;
};

#endif // DUMMYSOCKET_H
