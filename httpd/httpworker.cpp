/* Copyright 2012 Hallowyn and others.
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

HttpWorker::HttpWorker(int socketDescriptor, HttpServer *parent)
  : _server(parent), _socketDescriptor(socketDescriptor) {
}

void HttpWorker::run() {
  // TODO replace by QDateTime when Qt >= 4.7
  QTime before= QTime::currentTime();
  QTcpSocket *socket = new QTcpSocket(this);
  if (!socket->setSocketDescriptor(_socketDescriptor)) {
    // TODO
    // emit error(_socket->error());
  }
  QStringList args;
  HttpRequest req(socket);
  HttpResponse res(socket);
  HttpHandler *handler = 0;
  QUrl url;
  //qDebug() << "new client socket" << socket->peerAddress();
  QTextStream out(socket);
  QString line;
  if (!socket->canReadLine() && !socket->waitForReadyRead(5000)) {
    // TODO check RFC, HTTP may require \r\n end of line sequence
    out << "HTTP/1.0 500 Protocol error\n\n";
    goto finally;
  }
  line = socket->readLine(1024).trimmed();
  args = line.split(" ");
  if (args.size() != 3) {
    out << "HTTP/1.0 501 Protocol error\n\n";
    goto finally;
  }
  if (args.at(0) == "HEAD")
    req.setMethod(HttpRequest::HEAD);
  else if (args.at(0) == "GET")
    req.setMethod(HttpRequest::GET);
  else if (args.at(0) == "POST")
    req.setMethod(HttpRequest::POST);
  else if (args.at(0) == "PUT")
    req.setMethod(HttpRequest::PUT);
  else if (args.at(0) == "DELETE")
    req.setMethod(HttpRequest::DELETE);
  else {
    out << "HTTP/1.0 502 Protocol error\n\n";
    goto finally;
  }
  if (!args.at(2).startsWith("HTTP/")) {
    out << "HTTP/1.0 503 Protocol error\n\n";
    goto finally;
  }
  //qDebug() << "a1" << args;
  for (;;) {
    if (!socket->isOpen()) {
      //qDebug() << "socket is not open";
      break;
    }
    if (!socket->canReadLine() && !socket->waitForReadyRead(5000)) {
      //qDebug() << "socket is not readable";
      break;
    }
    line = socket->readLine(1024).trimmed();
    if (line.isNull()) {
      //qDebug() << "line is null";
      break;
    }
    if (line.isEmpty()) {
      //qDebug() << "line is empty";
      break;
    }
    // LATER: handle multi line headers
    if (!req.parseAndAddHeader(line)) {
      out << "HTTP/1.0 504 Protocol error\n\n";
      goto finally;
    }
    //qDebug() << "a7";
  }
  url = QUrl::fromEncoded(args.at(1).toAscii()/*, QUrl::StrictMode */);
  req.setUrl(url);
  handler = _server->chooseHandler(req);
  handler->handleRequest(req, res);
  res.output()->flush(); // calling output() ensures that header was sent
  //qDebug() << req;
finally:
  out.flush();
  // LATER fix random warning "QAbstractSocket::waitForBytesWritten() is not allowed in UnconnectedState"
  while(socket->state() != QAbstractSocket::UnconnectedState
        && socket->waitForBytesWritten(10000))
    ; //qDebug() << "waitForBytesWritten returned true" << socket->bytesToWrite();
  socket->close();
  socket->deleteLater();
  // give object back to parent event loop, otherwise deleteLater() wouldn't
  // be processed
  this->moveToThread(_server ? _server->thread() : 0);
  //long long duration = before.msecsTo(QTime::currentTime());
  //Statistics::record("server.http.hit", "", url.path(), duration,
  //                   req.header("Content-Length").toLongLong(), 1, 0, 0,
  //                   req.param("login"));
  //qDebug() << "served" << (handler ? handler->name() : "default") << "in"
  //    << duration << "ms" << url.path() << req.header("Content-Length")
  //    << req.param("login");
}
