/* Copyright 2012-2025 Hallowyn, Gregoire Barbier and others.
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
      ->paramUtf8("HTTP_DEFAULT_CACHE_CONTROL_HEADER"_u8, "no-cache"_u8);
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

void HttpWorker::handleConnection(int socketDescriptor) {
      // LATER replace by QDateTime when Qt >= 4.7
  //QTime before= QTime::currentTime();
  QTcpSocket *socket = new QTcpSocket(this);
  if (!socket->setSocketDescriptor(socketDescriptor)) {
    [[unlikely]];
    // LATER
    // emit error(_socket->error());
  }
  socket->setReadBufferSize(MAXIMUM_LINE_SIZE+2);
  Utf8StringList args;
  HttpRequest req(socket, this);
  HttpResponse res(socket);
  ParamsProviderMerger request_context;
  HttpHandler *handler = 0;
  Utf8String uri;
  QUrl url;
  //qDebug() << "new client socket" << socket->peerAddress();
  QTextStream out(socket);
  Utf8String line;
  qint64 contentLength = 0;
  HttpRequest::HttpMethod method = HttpRequest::NONE;
  if (!socket->canReadLine()
      && !socket->waitForReadyRead(MAXIMUM_READ_WAIT)) {
    sendError(out, "408 Request timeout");
    [[unlikely]] goto finally;
  }
  line = socket->readLine(MAXIMUM_LINE_SIZE+2);
  if (line.size() > MAXIMUM_LINE_SIZE) {
    sendError(out, "414 Request URI too long",
              "starting with: "+line.left(200));
    [[unlikely]] goto finally;
  }
  line = line.trimmed();
  args = line.split(' ');
  if (args.size() != 3) {
    sendError(out, "400 Bad request line",
              "starting with: "+line.left(200));
    goto finally;
  }
  method = HttpRequest::method_from_text(args[0]);
  req.set_method(method);
  if (method == HttpRequest::HEAD) {
    res.disable_body_output();
  } else if (method == HttpRequest::NONE
             || method == HttpRequest::ANY) {
    sendError(out, "405 Method not allowed",
              "starting with: "+args[0].left(200));
    [[unlikely]] goto finally;
  }
  if (!args[2].startsWith("HTTP/")) {
    sendError(out, "400 Bad request protocol",
              "starting with: "+args[2].left(200));
    [[unlikely]] goto finally;
  }
  for (;;) {
    if (!socket->isOpen()) {
      //qDebug() << "socket is not open";
      [[unlikely]] break;
    }
    if (!socket->canReadLine()
        && !socket->waitForReadyRead(MAXIMUM_READ_WAIT)) {
      sendError(out, "408 Request timeout");
      [[unlikely]] goto finally;
    }
    line = socket->readLine(MAXIMUM_LINE_SIZE+2).trimmed();
    if (line.size() > MAXIMUM_LINE_SIZE) {
      sendError(out, "413 Header line too long",
                "starting with: "+line.left(200));
      [[unlikely]] goto finally;
    }
    if (line.isEmpty()) {
      //qDebug() << "line is empty";
      break;
    }
    // LATER: handle multi line headers
    if (!req.parse_and_add_header(line)) {
      sendError(out, "400 Bad request header line",
                "starting with: "+line.left(200));
      [[unlikely]] goto finally;
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
  url = QUrl::fromEncoded("http://host"_u8+uri);
  req.set_url(url.toString(QUrl::RemoveUserInfo));
  req.set_path(url.path());
  // load post params
  if (method == HttpRequest::POST
      && req.header("Content-Type"_u8)
      == "application/x-www-form-urlencoded"_u8) {
    contentLength = req.header("Content-Length"_u8, "-1"_u8).toLongLong();
    if (contentLength < 0) {
      sendError(out, "411 Length Required");
      [[unlikely]] goto finally;
    }
    if (contentLength > MAXIMUM_ENCODED_FORM_POST_SIZE) {
      sendError(out, "413 Encoded form parameters string too long",
                "starting with: "+line.left(200));
      [[unlikely]] goto finally;
    }
    if (contentLength > 0) { // avoid enter infinite loop
      [[likely]];
      line = {};
      forever {
        line += socket->read(contentLength-line.size());
        if (contentLength && line.size() >= contentLength)
          break;
        // LATER avoid DoS by setting a maximum *total* read time out
        if (!socket->waitForReadyRead(MAXIMUM_READ_WAIT)) {
          sendError(out, "408 Request timeout");
          [[unlikely]] goto finally;
        }
      }
      // replacing + with space in URI since this cannot be done in HttpRequest
      // see above
      line.replace('+', ' ');
      // set body (POST) parameters
      for (const auto &[key, value]:
           QUrlQuery(line).queryItems(QUrl::FullyDecoded))
        req.set_query_param(key, value);
    }
  }
  // set query string (GET) parameters
  // they will override body (POST) parameters if any
  for (const auto &[key, value]: QUrlQuery(url).queryItems(QUrl::FullyDecoded))
    req.set_query_param(key, value);
  handler = _server->chooseHandler(req);
  if (req.header("Expect"_u8) == "100-continue"_u8) {
    // LATER only send 100 Continue if the URI is actually accepted by the handler
    out << "HTTP/1.1 100 Continue\r\n\r\n";
    out.flush();
  }
  if (!_defaultCacheControlHeader.isEmpty())
    [[likely]] res.set_header("Cache-Control", _defaultCacheControlHeader);
  request_context(&req)(&res);
  handler->handleRequest(req, res, request_context);
  if (auto logPolicy = _server->logPolicy();
      logPolicy == HttpServer::LogAllHits ||
      (logPolicy == HttpServer::LogErrorHits && !res.success())) {
    res.set_handled_date();
    res.output()->flush(); // calling output() ensures that header was sent
    res.set_flushed_date();
    Log::info() << _server->logFormat() % request_context;
  } else {
    res.output()->flush(); // calling output() ensures that header was sent
  }

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
