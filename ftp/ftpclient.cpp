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
#include "ftpclient.h"
#include <QHostAddress>

#define FTP_TRANSFER_CHUNK_SIZE 16384

FtpClient::FtpClient(QObject *parent)
  : QObject(parent), _controlSocket(new QTcpSocket(this)),
    _transferSocket(new QTcpSocket(this)) {
  connect(_controlSocket, &QTcpSocket::connected,
          this, &FtpClient::connected);
  connect(_controlSocket, &QTcpSocket::disconnected,
          this, &FtpClient::disconnected);
  connect(_transferSocket, &QTcpSocket::connected,
          this, &FtpClient::connectedTransferSocket);
  connect(_transferSocket, &QTcpSocket::disconnected,
          this, &FtpClient::disconnectedTransferSocket);
  connect(_transferSocket, &QTcpSocket::bytesWritten,
          this, &FtpClient::bytesWrittenToTransferSocket);
  connect(_transferSocket, &QTcpSocket::readyRead,
          this, &FtpClient::readyReadFromTransferSocket);
}

void FtpClient::abort() {
  _controlSocket->abort();
  _transferSocket->abort();
  _transferState = TransferFailed;
  _error = Error;
  _errorString = "Aborted";
}

void FtpClient::download(quint16 port, QIODevice *transferLocalDevice) {
  _transferState = Download;
  _transferLocalDevice = transferLocalDevice;
  _transferSocket->abort();
  _transferSocket->connectToHost(_controlSocket->peerAddress(), port,
                                 QIODevice::ReadOnly);
}

void FtpClient::upload(quint16 port, QIODevice *transferLocalDevice) {
  _transferState = Upload;
  _transferLocalDevice = transferLocalDevice;
  _transferSocket->abort();
  _transferSocket->connectToHost(_controlSocket->peerAddress(), port,
                                 QIODevice::WriteOnly);
}

void FtpClient::abortTransfer() {
  _transferState = NoTransfer;
  _transferSocket->abort();
  _transferLocalDevice = 0;
}

void FtpClient::bytesWrittenToTransferSocket(quint64 bytes) {
  Q_UNUSED(bytes)
  if (_transferLocalDevice->bytesAvailable()) {
    QByteArray buf = _transferLocalDevice->read(FTP_TRANSFER_CHUNK_SIZE);
    if (buf.isEmpty()) {
      _transferSocket->abort();
      _transferState = TransferFailed;
      _error = Error;
      _errorString = "Local read error : "+_transferLocalDevice->errorString();
    }
    int result = _transferSocket->write(buf);
    if (result == -1) {
      _transferSocket->abort();
      _transferState = TransferFailed;
      _error = Error;
      _errorString = "Upload error : "+_transferSocket->errorString();
    }
  } else {
    _transferSocket->disconnectFromHost();
  }
}

void FtpClient::readyReadFromTransferSocket() {
  if (_transferSocket->bytesAvailable()) { // should always be true
    QByteArray buf = _transferSocket->read(FTP_TRANSFER_CHUNK_SIZE);
    if (buf.isEmpty()) {
      _transferSocket->abort();
      _transferState = TransferFailed;
      _error = Error;
      _errorString = "Download error : "+_transferSocket->errorString();
    }
    int result = _transferLocalDevice->write(buf);
    if (result == -1) {
      _transferSocket->abort();
      _transferState = TransferFailed;
      _error = Error;
      _errorString = "Local write error : "+_transferLocalDevice->errorString();
    }
  }
}

void FtpClient::connectedTransferSocket() {
  switch (_transferState) {
  case Upload:
    bytesWrittenToTransferSocket(0);
    break;
  case Download: // nothing to do, readyRead() will follow
  case NoTransfer: // should never occur
  case TransferFailed: // should never occur
  case TransferSucceeded: // should never occur
    ;
  }
}

void FtpClient::disconnectedTransferSocket() {
  _transferState = _transferState == TransferFailed ? TransferFailed
                                                    : TransferSucceeded;
  _transferLocalDevice = 0;
}
