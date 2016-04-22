/* Copyright 2012-2016 Hallowyn and others.
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

static inline void sendError(QTextStream &out, const char *httpMessage,
                             QString logDetails = QString()) {
  out << "HTTP/1.1 " << httpMessage << "\r\nConnection: close\r\n\r\n";
  if (logDetails.isNull())
    Log::error() << httpMessage;
  else
    Log::error() << QString(httpMessage)+", "+logDetails;
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
  ParamsProviderMerger processingContext;
  HttpHandler *handler = 0;
  QString uri;
  QUrl url;
  //qDebug() << "new client socket" << socket->peerAddress();
  QTextStream out(socket);
  QString line;
  HttpRequest::HttpRequestMethod method = HttpRequest::NONE;
  if (!socket->canReadLine() && !socket->waitForReadyRead(MAXIMUM_READ_WAIT)) {
    sendError(out, "408 Request timeout");
    goto finally;
  }
  line = socket->readLine(MAXIMUM_LINE_SIZE+2);
  if (line.size() > MAXIMUM_LINE_SIZE) {
    sendError(out, "414 Request URI too long",
              "starting with: "+line.left(200));
    goto finally;
  }
  line = line.trimmed();
  args = line.split(QRegExp("[ \t]+"));
  if (args.size() != 3) {
    sendError(out, "400 Bad request line",
              "starting with: "+line.left(200));
    goto finally;
  }
  method = HttpRequest::methodFromText(args[0]);
  req.setMethod(method);
  if (method == HttpRequest::HEAD) {
    res.disableBodyOutput();
  } else if (method == HttpRequest::NONE || method == HttpRequest::ANY) {
    sendError(out, "405 Method not allowed",
              "starting with: "+args[0].left(200));
    goto finally;
  }
  if (!args[2].startsWith("HTTP/")) {
    sendError(out, "400 Bad request protocol",
              "starting with: "+args[2].left(200));
    goto finally;
  }
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
      sendError(out, "413 Header line too long",
                "starting with: "+line.left(200));
      goto finally;
    }
    if (line.isEmpty()) {
      //qDebug() << "line is empty";
      break;
    }
    // LATER: handle multi line headers
    if (!req.parseAndAddHeader(line)) {
      sendError(out, "400 Bad request header line",
                "starting with: "+line.left(200));
      goto finally;
    }
    //qDebug() << "a7";
  }
  uri = args[1];
  // replacing + with space in URI since this cannot be done in HttpRequest
  // unless QUrl implements a full HTML form encoding (including + for space)
  // in addition to current QUrl::FullyDecoded
  // see (among other references) QTBUG-10146
  uri.replace('+', ' ');
  // LATER is utf8 the right choice ? should encoding depend on headers ?
  url = QUrl::fromEncoded(uri.toUtf8());
  req.overrideUrl(url);
  handler = _server->chooseHandler(req);
  handler->handleRequest(req, res, &processingContext);
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
