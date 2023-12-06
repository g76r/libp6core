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
#include "mailsender.h"
#include "mailaddress.h"
#include <QTcpSocket>
#include <QUrl>

static int _defaultSmtpTimeoutMs = 5000;

static int staticInit() {
  bool ok;
  int i = QString::fromLocal8Bit(qgetenv("MAILSENDER_SMTP_TIMEOUT")).toInt(&ok);
  if (ok)
    _defaultSmtpTimeoutMs = i;
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

namespace {

// LATER avoid this ugly QObject subclass without Q_OBJECT
class EnhancedSocket : public QTcpSocket {
private:
  QString _lastExpectLine;
  int _smtpTimeoutMs;

public:
  EnhancedSocket(int smtpTimeoutMs = _defaultSmtpTimeoutMs)
    : _smtpTimeoutMs(smtpTimeoutMs) { }
  bool expectPrefix(const QString &prefix,
                    Qt::CaseSensitivity cs = Qt::CaseSensitive) {
    if (!waitForReadyRead(_smtpTimeoutMs)) {
      qWarning() << "read timeout on expectPrefix()" << prefix;
      return false;
    }
    QString line = readLine(1024);
    _lastExpectLine = line;
    if (line.startsWith(prefix, cs)) {
      //qDebug() << "match on expectPrefix()" << line << prefix;
      return true;
    }
    //qDebug() << "mismatch on expectPrefix()" << line << prefix;
    // LATER set errorString
    return false;
  }
  inline const QString &lastExpectLine() const { return _lastExpectLine; }
};

} // unnamed namespace

MailSender::MailSender(QUrl url)
  : _url(url), _smtpTimeoutMs(_defaultSmtpTimeoutMs) {
}

MailSender::MailSender(QString url)
  : _url(url), _smtpTimeoutMs(_defaultSmtpTimeoutMs) {
}

MailSender::MailSender(QUrl url, int smtpTimeoutMs)
  : _url(url), _smtpTimeoutMs(smtpTimeoutMs) {
}

MailSender::MailSender(QString url, int smtpTimeoutMs)
  : _url(url), _smtpTimeoutMs(smtpTimeoutMs) {
}


bool MailSender::send(QString sender, QStringList recipients, QVariant body,
                      QMultiHash<QString, QString> headers,
                      QList<QVariant> attachments, QString &errorString) {
  Q_UNUSED(attachments)
  MailAddress senderAddress(sender);
  if (!senderAddress.isValid()) {
    errorString = "invalid sender address: "+sender;
    return false;
  }
  EnhancedSocket socket(_smtpTimeoutMs);
  socket.connectToHost(_url.host(), _url.port(25));
  if (!socket.waitForConnected(_smtpTimeoutMs)) {
    errorString = "cannot connect to SMTP server "
        +_url.toString(QUrl::RemovePassword)+": "+socket.errorString();
    return false;
  }
  if (!socket.expectPrefix("2")) {
    errorString = "bad banner on SMTP server "
        +_url.toString(QUrl::RemovePassword)+": "+socket.errorString();
    return false;
  }
  socket.write("HELO 127.0.0.1\r\n");
  if (!socket.expectPrefix("2")) {
    errorString = "bad HELO response on SMTP server "
        +_url.toString(QUrl::RemovePassword)+": "+socket.errorString();
    return false;
  }
  // LATER check if addresses should be written in ASCII or in another code
  socket.write(QString("MAIL From: %1\r\n").arg(senderAddress).toLatin1());
  if (!socket.expectPrefix("2")) {
    errorString = "bad MAIL response on SMTP server "
        +_url.toString(QUrl::RemovePassword)+" for sender "+sender+": "
        +socket.errorString();
    return false;
  }
  for (auto recipient: recipients) {
    MailAddress addr(recipient);
    if (addr.isValid()) {
      socket.write(QString("RCPT To: %1\r\n").arg(addr).toLatin1());
      if (!socket.expectPrefix("2")) {
        errorString = "bad RCPT response on SMTP server "
            +_url.toString(QUrl::RemovePassword)+" for recipient "+recipient
            +": "+socket.errorString();
        return false;
      }
    } else {
      errorString = "invalid recipient address: "+recipient;
      return false;
    }
  }
  socket.write("DATA\r\n");
  if (!socket.expectPrefix("3")) {
    errorString = "bad DATA response on SMTP server "
        +_url.toString(QUrl::RemovePassword)+": "
        +socket.errorString();
    return false;
  }
  for (auto key: headers.uniqueKeys()) {
      for (auto value: headers.values(key)) {
      // LATER normalize header case, ensure values validity, handle multi line headers, etc.
      if (socket.write(QString("%1: %2\r\n").arg(key).arg(value)
                       .toLatin1()) == -1) {
        errorString = "error writing header "+key+": "+socket.errorString();
        return false;
      }
    }
  }
  if (socket.write("\r\n") == -1) {
    errorString = "error writing white line: "+socket.errorString();
    return false;
  }
  // LATER remove . line in body
  // LATER handle body encoding (force utf8 ?)
  if (socket.write(body.toString().toLatin1()) == -1) {
    errorString = "error writing body: "+socket.errorString();
    return false;
  }
  // LATER handle attachements
  if (socket.write("\r\n.\r\n") == -1) {
    errorString = "error writing footer: "+socket.errorString();
    return false;
  }
  if (!socket.expectPrefix("2")) {
    errorString = "bad end of data response on SMTP server "
        +_url.toString(QUrl::RemovePassword)+": "+socket.errorString();
    return false;
  }
  socket.write("QUIT\r\n");
  return true;
}
