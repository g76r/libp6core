/* Copyright 2012-2013 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
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
#include "httpworker.h"
#include <QtDebug>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTextStream>
#include <QStringList>
#include <QList>
#include "httprequest.h"
#include "httpresponse.h"
#include <QTime>
//#include "stats/statistics.h"
#include "log/log.h"
#include <QString>

#define MAXIMUM_LINE_SIZE 65536
#define MAXIMUM_READ_WAIT 30000
#define MAXIMUM_WRITE_WAIT 10000

static QAtomicInt _workersCounter(1);

HttpWorker::HttpWorker(HttpServer *server)
  : _server(server), _thread(new QThread()) {
  _thread->setObjectName(QString("HttpWorker-%1")
                         .arg(_workersCounter.fetchAndAddOrdered(1)));
  connect(this, SIGNAL(destroyed(QObject*)), _thread, SLOT(quit()));
  connect(_thread, SIGNAL(finished()), _thread, SLOT(deleteLater()));
  _thread->start();
  moveToThread(_thread);
}

void HttpWorker::handleConnection(int socketDescriptor) {
  // LATER replace by QDateTime when Qt >= 4.7
  //QTime before= QTime::currentTime();
  QTcpSocket *socket = new QTcpSocket(this);
  if (!socket->setSocketDescriptor(socketDescriptor)) {
    // LATER
    // emit error(_socket->error());
  }
  socket->setReadBufferSize(MAXIMUM_LINE_SIZE+2);
  QStringList args;
  HttpRequest req(socket);
  HttpResponse res(socket);
  HttpHandler *handler = 0;
  QUrl url;
  //qDebug() << "new client socket" << socket->peerAddress();
  QTextStream out(socket);
  QString line, method;
  if (!socket->canReadLine() && !socket->waitForReadyRead(MAXIMUM_READ_WAIT)) {
    out << "HTTP/1.0 408 Request timeout\r\n\r\n";
    Log::error() << "HTTP/1.0 408 Request timeout";
    goto finally;
  }
  line = socket->readLine(MAXIMUM_LINE_SIZE+2);
  if (line.size() > MAXIMUM_LINE_SIZE) {
    out << "HTTP/1.0 414 Request URI too long\r\n\r\n";
    Log::error() << "HTTP/1.0 414 Request URI too long, starting with: "
                 << line.left(200);
    goto finally;
  }
  line = line.trimmed();
  args = line.split(QRegExp("[ \t]+"));
  if (args.size() != 3) {
    out << "HTTP/1.0 400 Bad request line\r\n\r\n"+line;
    Log::error() << "HTTP/1.0 400 Bad request line, starting with: "
                 << line.left(200);
    goto finally;
  }
  method = args[0];
  if (method == "HEAD")
    req.setMethod(HttpRequest::HEAD);
  else if (method == "GET")
    req.setMethod(HttpRequest::GET);
  else if (method == "POST")
    req.setMethod(HttpRequest::POST);
  else if (method == "PUT")
    req.setMethod(HttpRequest::PUT);
  else if (method == "DELETE")
    req.setMethod(HttpRequest::DELETE);
  else {
    out << "HTTP/1.0 405 Method not allowed: \r\n\r\n"+method;
    Log::error() << "HTTP/1.0 405 Method not allowed, starting with: "
                 << method.left(200);
    goto finally;
  }
  if (!args[2].startsWith("HTTP/")) {
    out << "HTTP/1.0 400 Bad request protocol\r\n\r\n";
    Log::error() << "HTTP/1.0 400 Bad request protocol, starting with: "
                 << args[2].left(200);
    goto finally;
  }
  //qDebug() << "a1" << args;
  for (;;) {
    if (!socket->isOpen()) {
      //qDebug() << "socket is not open";
      break;
    }
    if (!socket->canReadLine()
        && !socket->waitForReadyRead(MAXIMUM_READ_WAIT)) {
      //qDebug() << "socket is not readable";
      break;
    }
    line = socket->readLine(MAXIMUM_LINE_SIZE+2).trimmed();
    if (line.size() > MAXIMUM_LINE_SIZE) {
      out << "HTTP/1.0 413 Header line too long\r\n\r\n";
      Log::error() << "HTTP/1.0 413 Header line too long, starting with: "
                   << line.left(200);
      goto finally;
    }
    if (line.isEmpty()) {
      //qDebug() << "line is empty";
      break;
    }
    // LATER: handle multi line headers
    if (!req.parseAndAddHeader(line)) {
      out << "HTTP/1.0 400 Bad request header line\r\n\r\n"+line;
      Log::error() << "HTTP/1.0 400 Bad request header line, starting with: "
                   << line.left(200);
      goto finally;
    }
    //qDebug() << "a7";
  }
  url = QUrl::fromEncoded(args[1].toLatin1()/*, QUrl::StrictMode */);
  req.overrideUrl(url);
  handler = _server->chooseHandler(req);
  handler->handleRequest(req, res, HttpRequestContext());
  res.output()->flush(); // calling output() ensures that header was sent
  //qDebug() << req;
finally:
  out.flush();
  // LATER fix random warning "QAbstractSocket::waitForBytesWritten() is not allowed in UnconnectedState"
  while(socket->state() != QAbstractSocket::UnconnectedState
        && socket->waitForBytesWritten(MAXIMUM_WRITE_WAIT))
    ; //qDebug() << "waitForBytesWritten returned true" << socket->bytesToWrite();
  socket->close();
  socket->deleteLater();
  emit connectionHandled(this);
  //long long duration = before.msecsTo(QTime::currentTime());
  //Statistics::record("server.http.hit", "", url.path(), duration,
  //                   req.header("Content-Length").toLongLong(), 1, 0, 0,
  //                   req.param("login"));
  //qDebug() << "served" << (handler ? handler->name() : "default") << "in"
  //    << duration << "ms" << url.path() << req.header("Content-Length")
  //    << req.param("login");
}
