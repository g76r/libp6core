/* Copyright 2016-2023 Hallowyn, Gregoire Barbier and others.
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
#include "dummysocket.h"
#include <QVariant>

Q_GLOBAL_STATIC(DummySocket, _singletonInstance) // FIXME don't singleton

DummySocket::DummySocket(QObject *parent)
  : QAbstractSocket(QAbstractSocket::UnknownSocketType, parent) {
  setOpenMode(QIODevice::ReadWrite);
}

DummySocket *DummySocket::singletonInstance() { // FIXME don't singleton
  return _singletonInstance;
}

bool DummySocket::isSequential() const {
  return true;
}

bool DummySocket::open(OpenMode mode) {
  Q_UNUSED(mode)
  setSocketError(QAbstractSocket::UnknownSocketError);
  setErrorString("DummySocket: cannot open");
  return false;
}

void DummySocket::close() {
}

qint64 DummySocket::pos() const {
  return 0;
}

qint64 DummySocket::size() const {
  return 0;
}

bool DummySocket::seek(qint64 pos) {
  Q_UNUSED(pos)
  setSocketError(QAbstractSocket::UnknownSocketError);
  setErrorString("DummySocket: cannot seek");
  return false;
}

bool DummySocket::atEnd() const {
  return true;
}

bool DummySocket::reset() {
  return true;
}

qint64 DummySocket::bytesAvailable() const {
  return 0;
}

qint64 DummySocket::bytesToWrite() const {
  return 0;
}

bool DummySocket::canReadLine() const {
  return false;
}

bool DummySocket::waitForReadyRead(int msecs) {
  Q_UNUSED(msecs)
  return false;
}

bool DummySocket::waitForBytesWritten(int msecs) {
  Q_UNUSED(msecs)
  return true;
}

qint64 DummySocket::readData(char *data, qint64 maxlen) {
  Q_UNUSED(data)
  Q_UNUSED(maxlen)
  return 0;
}

qint64 DummySocket::readLineData(char *data, qint64 maxlen) {
  Q_UNUSED(data)
  Q_UNUSED(maxlen)
  return 0;
}

qint64 DummySocket::writeData(const char *data, qint64 len) {
  Q_UNUSED(data)
  return len;
}

void DummySocket::resume() {
}

void DummySocket::connectToHost(const QString &hostName, quint16 port,
                                OpenMode mode, NetworkLayerProtocol protocol) {
  Q_UNUSED(hostName)
  Q_UNUSED(port)
  Q_UNUSED(mode)
  Q_UNUSED(protocol)
}

void DummySocket::connectToHost(const QHostAddress &address, quint16 port,
                                OpenMode mode) {
  Q_UNUSED(address)
  Q_UNUSED(port)
  Q_UNUSED(mode)
}

void DummySocket::disconnectFromHost() {
}

void DummySocket::setReadBufferSize(qint64 size) {
  Q_UNUSED(size)
}

qintptr DummySocket::socketDescriptor() const {
  return -1;
}

bool DummySocket::setSocketDescriptor(qintptr socketDescriptor,
                                      SocketState state, OpenMode openMode) {
  Q_UNUSED(socketDescriptor)
  Q_UNUSED(state)
  Q_UNUSED(openMode)
  setSocketError(QAbstractSocket::UnknownSocketError);
  setErrorString("DummySocket: cannot setSocketDescriptor");
  return false;
}

void DummySocket::setSocketOption(QAbstractSocket::SocketOption option,
                                  const QVariant &value) {
  Q_UNUSED(option)
  Q_UNUSED(value)
}

QVariant DummySocket::socketOption(QAbstractSocket::SocketOption option) {
  Q_UNUSED(option)
  return {};
}

bool DummySocket::waitForConnected(int msecs) {
  Q_UNUSED(msecs)
  return true;
}

bool DummySocket::waitForDisconnected(int msecs) {
  Q_UNUSED(msecs)
  return true;
}
