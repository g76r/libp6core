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
#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "httphandler.h"
#include <QTcpServer>
#include <QMutex>

class HttpWorker;

class LIBP6CORESHARED_EXPORT HttpServer : public QTcpServer {
  Q_OBJECT

public:
  enum LogPolicy : signed char {
    LogDisabled,
    LogErrorHits,
    LogAllHits,
  };

private:
  QMutex _handlersMutex;
  QList<HttpHandler *> _handlers;
  HttpHandler *_defaultHandler;
  QList<HttpWorker*> _workersPool;
  QList<int> _queuedSockets;
  int _maxQueuedSockets;
  QThread *_thread;
  LogPolicy _logPolicy;
  Utf8String _logFormat;

public:
  explicit HttpServer(int workersPoolSize = 16, int maxQueuedSockets = 32,
                      QObject *parent = 0);
  virtual ~HttpServer();
  /** The handler does not become a child of HttpServer but its deleteLater()
   * method is called by ~HttpServer(). */
  HttpServer &appendHandler(HttpHandler *handler);
  /** The handler does not become a child of HttpServer but its deleteLater()
   * method is called by ~HttpServer(). */
  HttpServer &prependHandler(HttpHandler *handler);
  HttpHandler *chooseHandler(HttpRequest req);
  bool listen(QHostAddress address = QHostAddress::Any, quint16 port = 0);
  inline bool listen(quint16 port) { return listen(QHostAddress::Any, port); }
  void close();
  inline HttpServer &setLogPolicy(LogPolicy policy) {
    _logPolicy = policy; return *this; }
  inline LogPolicy logPolicy() const { return _logPolicy; }
  static Utf8String logPolicyAsText(LogPolicy policy);
  inline Utf8String logPolicyAsText() { return logPolicyAsText(logPolicy()); }
  static LogPolicy logPolicyFromText(const Utf8String &text);
  inline HttpServer &setLogPolicy(const Utf8String &policy) {
    return setLogPolicy(logPolicyFromText(policy)); }
  inline HttpServer &setLogFormat(const Utf8String &format) {
    _logFormat = format; return *this; }
  inline Utf8String logFormat() const { return _logFormat; }

protected:
  void incomingConnection(qintptr handle) override;

private:
  void connectionHandled(HttpWorker *worker);
  Q_DISABLE_COPY(HttpServer)
};

#endif // HTTPSERVER_H
