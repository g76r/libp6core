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
#include <QBuffer>
#include <QFile>

using namespace std::placeholders;

#define FTP_WAIT_DURATION_MILLIS 1

//static QAtomicInt _sequence(1);
static QRegularExpression _pasvResultRe("\\(.*,(\\d+),(\\d+)\\)");
static QRegularExpression _resultCodeRe("^(\\d+) ");
static QRegularExpression _newlineRe("[\r\n]+");

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
             int minExpectedResult = 200,
             int maxExpectedResult = 299)
    : _command(command), _actionBefore(actionBefore),
      _isFinished(std::bind(noTransferIsFinished, _1, _2, _3, _4,
                            minExpectedResult, maxExpectedResult)) { }
  FtpCommand(QString command, int minExpectedResult = 200,
             int maxExpectedResult = 299)
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
  //int _id;
  FtpClient *_client;
  QVector<FtpCommand> _commands;
  QString _host, _login, _password;
  quint16 _port, _pasvPort;
  FtpScriptData(FtpClient *client = 0)
    : /*_id(_sequence.fetchAndAddOrdered(1)),*/ _client(client) { }

  bool pasvIsFinished(bool *success, QString *errorString, int resultCode,
                       QString result) {
    Q_UNUSED(errorString)
    Q_ASSERT(success);
    QRegularExpressionMatch match = _pasvResultRe.match(result);
    if (resultCode != 227 || !match.hasMatch()) {
      *success = false;
      return true;
    }
    *success = true;
    _pasvPort = match.captured(1).toInt()*256+match.captured(2).toInt();
    return true;
  }
  bool transferIsFinished(bool *success, QString *errorString, int resultCode,
                          QString result) {
    Q_UNUSED(errorString)
    Q_UNUSED(result)
    Q_ASSERT(success);
    if (resultCode < 200 || resultCode > 299) {
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
  bool memoryTransferIsFinished(
      bool *success, QString *errorString, int resultCode, QString result,
      QIODevice *localDevice) {
    Q_ASSERT(localDevice);
    bool finished = transferIsFinished(success, errorString, resultCode,
                                       result);
    // TODO what if the command is destoyed before finished ?
    // e.g. after abort or by destroying the script before execution
    if (finished)
      delete localDevice;
    return finished;
  }
  bool nlstTransferIsFinished(
      bool *success, QString *errorString, int resultCode, QString result,
      QBuffer *buffer, QStringList *relativePaths) {
    Q_ASSERT(buffer);
    Q_ASSERT(relativePaths);
    bool finished = transferIsFinished(success, errorString, resultCode,
                                       result);
    // TODO what if the command is destoyed before finished ?
    // e.g. after abort or by destroying the script before execution
    if (finished) {
      *relativePaths = QString::fromUtf8(buffer->data())
          .split(_newlineRe, QString::SkipEmptyParts);
      relativePaths->removeAll(".");
      relativePaths->removeAll("..");
      delete buffer;
    }
    return finished;
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

/*int FtpScript::id() const {
  const FtpScriptData *d = _data;
  return d ? d->_id : 0;
}*/

static QRegularExpression _intermidiaryStatusLineRE("^(1|\\d+-)");

static void skipIntermediaryStatusLines(QString &result) {
  while (_intermidiaryStatusLineRE.match(result).hasMatch()) {
    // skip intermidiary 1xx status lines to keep only final status
    // e.g. RETR is answered smth like "1xx transfer begin\r\n2xx ok\r\n"
    // skip multiline status line of the form xxx-
    int i = result.indexOf('\n');
    if (i > 0) {
      qDebug() << "  skipping intermediary status : "
               << result.left(i).trimmed();
      result = result.mid(i+1);
    }
  }
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
    // action before command
    qDebug() << "  action before" << !!command._actionBefore;
    if (command._actionBefore)
      command._actionBefore();
    // waiting for connection (when action before was connectToHost)
    if (!d->_client->_controlSocket->waitForConnected(
          std::max(msecs-timer.elapsed(), (qint64)0))) {
      d->_client->_error = FtpClient::Error;
      d->_client->_errorString = "Not connected to server : "
          +d->_client->_controlSocket->errorString();
      goto failed;
    }
    // send command to control socket
    QString result;
    int resultCode;
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
      skipIntermediaryStatusLines(result);
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
    d->_commands.append(FtpCommand("OPTS UTF8 ON", 0, 9999999));
  }
  return *this;
}

FtpScript &FtpScript::login(QString login, QString password) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand([d, login]() {
      d->_login = login;
    }, "USER "+login, 200, 399));
    d->_commands.append(FtpCommand([d, password]() {
      d->_password = password;
    }, "PASS "+password));
    // LATER make binary transfer an option
    d->_commands.append(FtpCommand("TYPE I"));
    // LATER make umask an option
    d->_commands.append(FtpCommand("SITE UMASK 22", 0, 9999999));
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

FtpScript &FtpScript::mkdir(QString path) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("MKD "+path));
  }
  return *this;
}

FtpScript &FtpScript::mkdirIgnoringFailure(QString path) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("MKD "+path, 0, 999999999));
  }
  return *this;
}

FtpScript &FtpScript::rmdir(QString path) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("RMD "+path));
  }
  return *this;
}

FtpScript &FtpScript::rmdirIgnoringFailure(QString path) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("RMD "+path, 0, 999999999));
  }
  return *this;
}

FtpScript &FtpScript::rm(QString path) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("DELE "+path));
  }
  return *this;
}

FtpScript &FtpScript::rmIgnoringFailure(QString path) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("DELE "+path, 0, 999999999));
  }
  return *this;
}

FtpScript &FtpScript::ls(QStringList *relativePaths, QString path) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand([]{},
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4), "PASV"));
    QBuffer *buf = new QBuffer(d->_client);
    buf->open(QIODevice::WriteOnly);
    d->_commands.append(FtpCommand([d, buf, relativePaths]() {
      d->_client->download(d->_pasvPort, buf);
    },
    std::bind(&FtpScriptData::nlstTransferIsFinished, d, _1, _2, _3, _4, buf,
              relativePaths),
    "NLST "+path));
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

FtpScript &FtpScript::get(QString path, QByteArray *dest) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand([]{},
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4), "PASV"));
    QBuffer *buf = new QBuffer(dest, d->_client);
    buf->open(QIODevice::WriteOnly);
    d->_commands.append(FtpCommand([d, buf]() {
      d->_client->download(d->_pasvPort, buf);
    },
    std::bind(&FtpScriptData::memoryTransferIsFinished, d, _1, _2, _3, _4, buf),
    "RETR "+path));
  }
  return *this;
}

FtpScript &FtpScript::get(QString path, QString localPath) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand([]{},
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4), "PASV"));
    QFile *file = new QFile(localPath);
    // LATER create and truncate only when download start
    file->open(QIODevice::WriteOnly|QIODevice::Truncate);
    d->_commands.append(FtpCommand([d, file]() {
      d->_client->download(d->_pasvPort, file);
    },
    std::bind(&FtpScriptData::memoryTransferIsFinished, d, _1, _2, _3, _4,
              file),
    "RETR "+path));
  }
  return *this;
}

FtpScript &FtpScript::put(QString path, QIODevice *source) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand([]{},
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4), "PASV"));
    d->_commands.append(FtpCommand([d, source]() {
      d->_client->upload(d->_pasvPort, source);
    },
    std::bind(&FtpScriptData::transferIsFinished, d, _1, _2, _3, _4),
    "STOR "+path));
  }
  return *this;
}

FtpScript &FtpScript::put(QString path, QByteArray source) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand([]{},
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4), "PASV"));
    QBuffer *buf = new QBuffer(d->_client);
    buf->setData(source);
    buf->open(QIODevice::ReadOnly);
    d->_commands.append(FtpCommand([d, buf]() {
      d->_client->upload(d->_pasvPort, buf);
    },
    std::bind(&FtpScriptData::memoryTransferIsFinished, d, _1, _2, _3, _4, buf),
    "STOR "+path));
  }
  return *this;
}

FtpScript &FtpScript::put(QString path, QString localPath) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand([]{},
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4), "PASV"));
    QFile *file = new QFile(localPath);
    file->open(QIODevice::ReadOnly);
    d->_commands.append(FtpCommand([d, file]() {
      d->_client->upload(d->_pasvPort, file);
    },
    std::bind(&FtpScriptData::memoryTransferIsFinished, d, _1, _2, _3, _4,
              file),
    "STOR "+path));
  }
  return *this;
}
