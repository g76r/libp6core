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
#include "httpworker.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "log/log.h"
#include <QTcpSocket>
#include <QThread>

#define MAXIMUM_LINE_SIZE 65536
#define MAXIMUM_ENCODED_FORM_POST_SIZE MAXIMUM_LINE_SIZE
#define MAXIMUM_READ_WAIT 30000
#define MAXIMUM_WRITE_WAIT 10000

static QAtomicInt _workersCounter(1);

HttpWorker::HttpWorker(HttpServer *server)
  : _server(server), _thread(new QThread()) {
  _defaultCacheControlHeader = ParamsProvider::environment()
      ->paramUtf8("HTTP_DEFAULT_CACHE_CONTROL_HEADER"_ba, "no-cache"_ba);
  _thread->setObjectName(QString("HttpWorker-%1")
                         .arg(_workersCounter.fetchAndAddOrdered(1)));
  connect(this, &HttpWorker::destroyed, _thread, &QThread::quit);
  connect(_thread, &QThread::finished, _thread, &QThread::deleteLater);
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

void HttpWorker::handleConnection(
    int socketDescriptor, std::function<void()> handledCallback) {
  // LATER replace by QDateTime when Qt >= 4.7
  //QTime before= QTime::currentTime();
  QTcpSocket *socket = new QTcpSocket(this);
  if (!socket->setSocketDescriptor(socketDescriptor)) [[unlikely]] {
    // LATER
    // emit error(_socket->error());
  }
  socket->setReadBufferSize(MAXIMUM_LINE_SIZE+2);
  QByteArrayList args;
  HttpRequest req(socket);
  HttpResponse res(socket);
  ParamsProviderMerger processingContext;
  HttpHandler *handler = 0;
  QByteArray uri;
  QUrl url;
  //qDebug() << "new client socket" << socket->peerAddress();
  QTextStream out(socket);
  QByteArray line;
  qint64 contentLength = 0;
  HttpRequest::HttpMethod method = HttpRequest::NONE;
  if (!socket->canReadLine()
      && !socket->waitForReadyRead(MAXIMUM_READ_WAIT)) [[unlikely]] {
    sendError(out, "408 Request timeout");
    goto finally;
  }
  line = socket->readLine(MAXIMUM_LINE_SIZE+2);
  if (line.size() > MAXIMUM_LINE_SIZE) [[unlikely]] {
    sendError(out, "414 Request URI too long",
              "starting with: "+line.left(200));
    goto finally;
  }
  line = line.trimmed();
  args = line.split(' ');
  if (args.size() != 3) {
    sendError(out, "400 Bad request line",
              "starting with: "+line.left(200));
    goto finally;
  }
  method = HttpRequest::methodFromText(args[0]);
  req.setMethod(method);
  if (method == HttpRequest::HEAD) {
    res.disableBodyOutput();
  } else if (method == HttpRequest::NONE
             || method == HttpRequest::ANY) [[unlikely]] {
    sendError(out, "405 Method not allowed",
              "starting with: "+args[0].left(200));
    goto finally;
  }
  if (!args[2].startsWith("HTTP/")) [[unlikely]] {
    sendError(out, "400 Bad request protocol",
              "starting with: "+args[2].left(200));
    goto finally;
  }
  for (;;) {
    if (!socket->isOpen()) [[unlikely]] {
      //qDebug() << "socket is not open";
      break;
    }
    if (!socket->canReadLine()
        && !socket->waitForReadyRead(MAXIMUM_READ_WAIT)) [[unlikely]] {
      sendError(out, "408 Request timeout");
      goto finally;
    }
    line = socket->readLine(MAXIMUM_LINE_SIZE+2).trimmed();
    if (line.size() > MAXIMUM_LINE_SIZE) [[unlikely]] {
      sendError(out, "413 Header line too long",
                "starting with: "+line.left(200));
      goto finally;
    }
    if (line.isEmpty()) {
      //qDebug() << "line is empty";
      break;
    }
    // LATER: handle multi line headers
    if (!req.parseAndAddHeader(line)) [[unlikely]] {
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
  // ensure uri starts with /
  if (uri.isEmpty() || uri[0] != '/')
    uri.insert(0, '/');
  // LATER is utf8 the right choice ? should encoding depend on headers ?
  url = QUrl::fromEncoded("http://host"_ba+uri);
  req.overrideUrl(url);
  // load post params
  // LATER should probably also remove query items
  if (method == HttpRequest::POST
      && req.header("Content-Type"_ba)
      == "application/x-www-form-urlencoded"_ba) {
    contentLength = req.header("Content-Length"_ba, "-1"_ba).toLongLong();
    if (contentLength < 0) [[unlikely]] {
      sendError(out, "411 Length Required");
      goto finally;
    }
    if (contentLength > MAXIMUM_ENCODED_FORM_POST_SIZE) [[unlikely]] {
      sendError(out, "413 Encoded form parameters string too long",
                "starting with: "+line.left(200));
      goto finally;
    }
    if (contentLength > 0) [[likely]] { // avoid enter infinite loop
      line = {};
      forever {
        line += socket->read(contentLength-line.size());
        if (contentLength && line.size() >= contentLength)
          break;
        // LATER avoid DoS by setting a maximum *total* read time out
        if (!socket->waitForReadyRead(MAXIMUM_READ_WAIT)) [[unlikely]] {
          sendError(out, "408 Request timeout");
          goto finally;
        }
      }
      // replacing + with space in URI since this cannot be done in HttpRequest
      // see above
      line.replace('+', ' ');
      foreach (const auto &p, QUrlQuery(line).queryItems(QUrl::FullyDecoded))
        req.overrideParam(p.first.toUtf8(), p.second.toUtf8());
    }
    // override body parameters with query string parameters
    foreach (const auto &p, QUrlQuery(url).queryItems(QUrl::FullyDecoded))
      req.overrideParam(p.first.toUtf8(), p.second.toUtf8());
  }
  handler = _server->chooseHandler(req);
  if (req.header("Expect"_ba) == "100-continue"_ba) {
    // LATER only send 100 Continue if the URI is actually accepted by the handler
    out << "HTTP/1.1 100 Continue\r\n\r\n";
    out.flush();
  }
  if (!_defaultCacheControlHeader.isEmpty()) [[likely]]
    res.setHeader("Cache-Control", _defaultCacheControlHeader);
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
  QMetaObject::invokeMethod(_server, handledCallback);
  //long long duration = before.msecsTo(QTime::currentTime());
  //Statistics::record("server.http.hit", "", url.path(), duration,
  //                   req.header("Content-Length").toLongLong(), 1, 0, 0,
  //                   req.param("login"));
  //qDebug() << "served" << (handler ? handler->name() : "default") << "in"
  //    << duration << "ms" << url.path() << req.header("Content-Length")
  //    << req.param("login");
}
