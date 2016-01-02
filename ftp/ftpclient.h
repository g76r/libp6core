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

/** One thread, event-driven FTP client executing one or several operation in
 * a fail-at-first-error script-style fashion.
 *
 * Works only with common modern FTP dialect (only passive transfer, only binary
 * file encoding) but does not need any FTP extension (does not use e.g. EPSV).
 *
 * Should work nicely over IPv6 although not yet tested.
 */
class LIBQTSSUSHARED_EXPORT FtpClient : public QObject {
  friend class FtpScript;
  friend class FtpScriptData;
  Q_OBJECT

public:
  enum FtpError { NoError, Error };
  enum TransferState { NoTransfer, Download, Upload, TransferSucceeded,
                       TransferFailed };

private:
  Q_DISABLE_COPY(FtpClient)
  QTcpSocket *_controlSocket, *_transferSocket;
  FtpError _error;
  QString _errorString;
  TransferState _transferState;
  QIODevice *_transferLocalDevice;

public:
  explicit FtpClient(QObject *parent = 0);
  FtpScript script() { return FtpScript(this); }
  FtpError error() const { return _error; }
  QString errorString() const { return _errorString; }
  void abort();
  bool connectToHost(QString host, quint16 port = 21,
                     int msecs = FtpScript::DefaultTimeout) {
    return script().connectToHost(host, port).execAndWait(msecs);
  }
  bool login(QString login, QString password,
             int msecs = FtpScript::DefaultTimeout) {
    return script().login(login, password).execAndWait(msecs);
  }
  bool cd(QString path, int msecs = FtpScript::DefaultTimeout) {
    return script().cd(path).execAndWait(msecs);
  }
  bool mkdir(QString path, int msecs = FtpScript::DefaultTimeout) {
    return script().mkdir(path).execAndWait(msecs);
  }
  bool mkdirIgnoringFailure(QString path,
                            int msecs = FtpScript::DefaultTimeout) {
    return script().mkdirIgnoringFailure(path).execAndWait(msecs);
  }
  bool rmdir(QString path, int msecs = FtpScript::DefaultTimeout) {
    return script().rmdir(path).execAndWait(msecs);
  }
  bool rmdirIgnoringFailure(QString path,
                            int msecs = FtpScript::DefaultTimeout) {
    return script().rmdirIgnoringFailure(path).execAndWait(msecs);
  }
  bool rm(QString path, int msecs = FtpScript::DefaultTimeout) {
    return script().rm(path).execAndWait(msecs);
  }
  bool rmIgnoringFailure(QString path, int msecs = FtpScript::DefaultTimeout) {
    return script().rmIgnoringFailure(path).execAndWait(msecs);
  }
  bool ls(QStringList *relativePaths, QString path = ".",
          int msecs = FtpScript::DefaultTimeout) {
    return script().ls(relativePaths, path).execAndWait(msecs);
  }
  // LATER lsLong(QList<FtpFileInfo>*, path)
  bool get(QString path, QIODevice *dest,
           int msecs = FtpScript::DefaultTimeout) {
    return script().get(path, dest).execAndWait(msecs);
  }
  bool get(QString path, QByteArray *dest,
           int msecs = FtpScript::DefaultTimeout) {
    return script().get(path, dest).execAndWait(msecs);
  }
  bool get(QString path, QString localPath,
           int msecs = FtpScript::DefaultTimeout) {
    return script().get(path, localPath).execAndWait(msecs);
  }
  bool get(QString path, const char *localPath,
           int msecs = FtpScript::DefaultTimeout) {
    return script().get(path, localPath).execAndWait(msecs);
  }
  bool put(QString path, QIODevice *source,
           int msecs = FtpScript::DefaultTimeout) {
    return script().put(path, source).execAndWait(msecs);
  }
  bool put(QString path, QByteArray source,
           int msecs = FtpScript::DefaultTimeout) {
    return script().put(path, source).execAndWait(msecs);
  }
  bool put(QString path, QString localPath,
           int msecs = FtpScript::DefaultTimeout) {
    return script().put(path, localPath).execAndWait(msecs);
  }
  bool put(QString path, const char *localPath,
           int msecs = FtpScript::DefaultTimeout) {
    return script().put(path, localPath).execAndWait(msecs);
  }

signals:
  void connected();
  void disconnected();
  void scriptStarted(FtpScript script);
  void scriptFinished(bool success, QString errorString, FtpError error);
  // LATER more signals: transferFinished() transferProgress()

private:
  void download(quint16 port, QIODevice *transferLocalDevice);
  void upload(quint16 port, QIODevice *transferLocalDevice);
  void abortTransfer();
  void bytesWrittenToTransferSocket(quint64 bytes);
  void readyReadFromTransferSocket();
  void connectedTransferSocket();
  void disconnectedTransferSocket();
};

#endif // FTPCLIENT_H
