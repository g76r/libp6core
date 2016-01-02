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
#include "ftpscript.h"
#include "ftpclient.h"
#include <QVector>
#include <functional>
#include <QAtomicInt>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <unistd.h>

using namespace std::placeholders;

#define FTP_WAIT_DURATION_MILLIS 1

static QAtomicInt _sequence(1);
static QRegularExpression _pasvResultRe(",(\\d+),(\\d+)\\)$");
static QRegularExpression _resultCodeRe("^(\\d+) ");

struct FtpCommand {
  QString _command;
  std::function<void()> _actionBefore;
  std::function<bool(bool *success, QString *errorString, int resultCode,
                     QString result)> _isFinished;
  FtpCommand(std::function<void()> actionBefore,
             std::function<bool(bool *success, QString *errorString,
                                int resultCode, QString result)> isFinished,
             QString command = QString())
    : _command(command), _actionBefore(actionBefore),
      _isFinished(isFinished) { }
  FtpCommand(std::function<void()> actionBefore,
             QString command = QString(),
             int minExpectedResult = 100,
             int maxExpectedResult = 399)
    : _command(command), _actionBefore(actionBefore),
      _isFinished(std::bind(noTransferIsFinished, _1, _2, _3, _4,
                            minExpectedResult, maxExpectedResult)) { }
  FtpCommand(QString command, int minExpectedResult = 100,
             int maxExpectedResult = 399)
    : _command(command),
      _isFinished(std::bind(noTransferIsFinished, _1, _2, _3, _4,
                            minExpectedResult, maxExpectedResult)) { }
  FtpCommand() : _isFinished(alwaysFail) { }

private:
  static bool noTransferIsFinished(
      bool *success, QString *errorString, int resultCode, QString result,
      int minExpectedResult, int maxExpectedResult) {
    Q_UNUSED(result)
    Q_UNUSED(errorString)
    Q_ASSERT(success);
    *success = resultCode >= minExpectedResult
        && resultCode <= maxExpectedResult;
    return true;
  }
  static bool alwaysFail(bool *success, QString *errorString, int resultCode,
                         QString result) {
    Q_UNUSED(result)
    Q_UNUSED(resultCode)
    Q_ASSERT(success);
    Q_ASSERT(errorString);
    *success = false;
    *errorString = "Failed";
    return true;
  }
};

struct FtpScriptData : public QSharedData {
  int _id;
  FtpClient *_client;
  QVector<FtpCommand> _commands;
  QString _host, _login, _password;
  quint16 _port, _pasvPort;
  FtpScriptData(FtpClient *client = 0)
    : _id(_sequence.fetchAndAddOrdered(1)), _client(client) { }

  bool pasvIsFinished(bool *success, QString *errorString, int resultCode,
                       QString result) {
    Q_UNUSED(errorString)
    Q_ASSERT(success);
    QRegularExpressionMatch match = _pasvResultRe.match(result);
    if (resultCode != 227 || !match.hasMatch())
      *success = false;
    _pasvPort = match.captured(1).toInt()*256+match.captured(2).toInt();
    return true;
  }
  bool transferIsFinished(bool *success, QString *errorString, int resultCode,
                          QString result) {
    Q_UNUSED(errorString)
    Q_UNUSED(result)
    Q_ASSERT(success);
    if (resultCode < 200 || resultCode > 399) {
      _client->abortTransfer();
      *success = false;
      return true;
    }
    switch (_client->_transferState) {
    case FtpClient::TransferFailed:
      *success = false;
      return true;
    case FtpClient::TransferSucceeded:
      *success = true;
      return true;
    default:
      return false;
    }
  }
};

FtpScript::FtpScript(FtpClient *client)
  : _data(client ? new FtpScriptData(client) : 0) {
}

FtpScript::FtpScript(const FtpScript &rhs) : _data(rhs._data) {
}

FtpScript::~FtpScript() {
}

FtpScript &FtpScript::operator=(const FtpScript &rhs) {
  if (this != &rhs)
    _data.operator=(rhs._data);
  return *this;
}

FtpClient *FtpScript::client() const {
  const FtpScriptData *d = _data;
  return d ? d->_client : 0;
}

int FtpScript::id() const {
  const FtpScriptData *d = _data;
  return d ? d->_id : 0;
}

bool FtpScript::execAndWait(int msecs) {
  // note that execAndWait must not be const since FtpCommand's lambdas are
  // capturing non-const references on FtpScriptData and use them to change
  // values such as login or pasvPort
  FtpScriptData *d = _data;
  if (!d)
    return false;
  emit d->_client->scriptStarted(*this);
  QElapsedTimer timer;
  timer.start();
  int i = 0;
  // FIXME QEventLoop::ExcludeUserInputEvents ?
  while (!timer.hasExpired(msecs)) {
    qDebug() << "ftp exec loop" << i << d->_commands.size();
    if (i >= d->_commands.size()) {
      d->_client->_error = FtpClient::NoError;
      d->_client->_errorString = "Success";
      emit d->_client->scriptFinished(true, d->_client->_errorString,
                                      d->_client->_error);
      return true;
    }
    FtpCommand &command = d->_commands[i];
    QString result;
    int resultCode;
    qDebug() << "  action before";
    if (command._actionBefore)
      command._actionBefore();
    // send command to control socket
    qDebug() << "  command" << command._command;
    if (!command._command.isEmpty()) {
      int written = d->_client->_controlSocket->write(
            command._command.toUtf8()+"\r\n");
      if (written < 0) {
        d->_client->_error = FtpClient::Error;
        d->_client->_errorString = "Cannot send request to server : "
            +d->_client->_controlSocket->errorString();
        goto failed;
      }
    }
    // wait for result (or for banner, since connectToServer has no command)
    forever {
      QByteArray buf = d->_client->_controlSocket->readAll();
      result += QString::fromUtf8(buf);
      //qDebug() << "    temporary result" << result << buf;
      if (result.endsWith('\n'))
        break;
      if (timer.hasExpired(msecs))
        goto timeout;
      usleep(FTP_WAIT_DURATION_MILLIS*1000);
      QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    result = result.trimmed();
    qDebug() << "  result" << result;
    if (result.isEmpty()) {
      d->_client->_error = FtpClient::Error;
      if (!d->_client->_controlSocket->isOpen())
        d->_client->_errorString = "Connection closed : "
            +d->_client->_controlSocket->errorString();
      else
        d->_client->_errorString = "No response : "
            +d->_client->_controlSocket->errorString();
      goto failed;
    }
    QRegularExpressionMatch match = _resultCodeRe.match(result);
    resultCode = match.hasMatch() ? match.captured(1).toInt() : 0;
    // wait for command being actually finished (transfers are not finished
    // as soon as the control connection receive the result string) and
    // interpret result as success or failure
    if (command._isFinished) {
      bool success = false;
      d->_client->_errorString = result;
      forever {
        if (command._isFinished(&success, &d->_client->_errorString,
                                resultCode, result))
          break;
        if (timer.hasExpired(msecs))
          goto timeout;
        usleep(FTP_WAIT_DURATION_MILLIS*1000);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
      }
      if (success) {
        qDebug() << "successfuly executed FTP command" << command._command
                 << result;
        ++i;
      } else {
        qDebug() << "error when executing FTP command" << command._command
                 << result;
        d->_client->_error = FtpClient::Error;
        goto failed;
      }
    } else { // should never happen
      d->_client->_error = FtpClient::Error;
      d->_client->_errorString = "Internal error : no isFinished function";
      goto failed;
    }
  }
timeout:
  d->_client->_error = FtpClient::Error;
  d->_client->_errorString = "Timeout expired";
failed:
  emit d->_client->scriptFinished(false, d->_client->_errorString,
                                  d->_client->_error);
  return false;
}

FtpScript &FtpScript::clearCommands() {
  FtpScriptData *d = _data;
  if (d)
    d->_commands.clear();
  return *this;
}

FtpScript &FtpScript::connectToHost(QString host, quint16 port) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand([d, host, port]() {
      d->_client->_controlSocket->close();
      d->_client->_controlSocket->connectToHost(host, port);
      d->_host = host;
      d->_port = port;
    }));
  }
  return *this;
}

FtpScript &FtpScript::login(QString login, QString password) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand([d, login]() {
      d->_login = login;
    }, "USER "+login));
    d->_commands.append(FtpCommand([d, password]() {
      d->_password = password;
    }, "PASS "+password));
    // LATER make binary transfer an option
    d->_commands.append(FtpCommand("TYPE I"));
    // LATER execute PWD and store its result
  }
  return *this;
}

FtpScript &FtpScript::cd(QString path) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("CWD "+path));
    // LATER execute PWD and store its result
  }
  return *this;
}

FtpScript &FtpScript::get(QString path, QIODevice *dest) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand([]{},
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4), "PASV"));
    d->_commands.append(FtpCommand([d, dest]() {
      d->_client->download(d->_pasvPort, dest);
    },
    std::bind(&FtpScriptData::transferIsFinished, d, _1, _2, _3, _4),
    "RETR "+path));
  }
  return *this;
}

