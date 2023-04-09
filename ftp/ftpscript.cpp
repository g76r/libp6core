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
#include <QStack>

using namespace std::placeholders;
using namespace Qt::Literals::StringLiterals;

#define FTP_WAIT_DURATION_MILLIS 1

//static QAtomicInt _sequence(1);
static QRegularExpression _pasvResultRe("\\(.*,(\\d+),(\\d+)\\)");
static QRegularExpression _resultCodeRe("\\A(\\d+) ");
static QRegularExpression _newlineRe("[\r\n]+");
static QRegularExpression _pwdRe("\\A2\\d\\d \"?([^\"]+)");

struct FtpCommand {
  std::function<void()> _actionBefore;
  std::function<QString()> _command;
  std::function<bool(bool *success, QString *errorString, int resultCode,
                     QString result)> _isFinished;
  FtpCommand(std::function<void()> actionBefore,
             std::function<QString()> command,
             std::function<bool(bool *success, QString *errorString,
                                int resultCode, QString result)> isFinished)
    : _actionBefore(actionBefore), _command(command),
      _isFinished(isFinished) { }
  FtpCommand(std::function<void()> actionBefore,
             std::function<QString()> command, int minExpectedResult = 200,
             int maxExpectedResult = 299)
    : FtpCommand(actionBefore, command,
                 std::bind(resultCodeBaseIsFinished, _1, _2, _3, _4,
                           minExpectedResult, maxExpectedResult)) { }
  FtpCommand(std::function<QString()> command, int minExpectedResult = 200,
             int maxExpectedResult = 299)
    : FtpCommand([](){}, command, minExpectedResult, maxExpectedResult) { }
  FtpCommand(std::function<QString()> command,
             std::function<bool(bool *success, QString *errorString,
                                int resultCode, QString result)> isFinished)
    : FtpCommand([](){}, command, isFinished) { }
  FtpCommand(std::function<void()> actionBefore, int minExpectedResult = 200,
             int maxExpectedResult = 299)
    : FtpCommand(actionBefore, [](){ return QString(); },
  std::bind(resultCodeBaseIsFinished, _1, _2, _3, _4, minExpectedResult,
            maxExpectedResult)) { }
  FtpCommand(std::function<void()> actionBefore, QString command,
             std::function<bool(bool *success, QString *errorString,
                                int resultCode, QString result)> isFinished)
    : FtpCommand(actionBefore, [=](){ return command; }, isFinished) { }
  FtpCommand(std::function<void()> actionBefore, QString command,
             int minExpectedResult = 200, int maxExpectedResult = 299)
    : FtpCommand(actionBefore, [=](){ return command; }, minExpectedResult,
  maxExpectedResult) { }
  FtpCommand(QString command,
             std::function<bool(bool *success, QString *errorString,
                                int resultCode, QString result)> isFinished)
    : FtpCommand([](){}, [=](){ return command; }, isFinished) { }
  FtpCommand(QString command, int minExpectedResult = 200,
             int maxExpectedResult = 299)
    : FtpCommand([](){}, [=](){ return command; }, minExpectedResult,
  maxExpectedResult) { }
  FtpCommand() : FtpCommand([](){}, [](){ return QString(); }, alwaysFail) { }

private:
  static bool resultCodeBaseIsFinished(
      bool *success, QString *errorString, int resultCode, QString result,
      int minExpectedResult, int maxExpectedResult) {
    Q_UNUSED(result)
    Q_UNUSED(errorString)
    Q_ASSERT(success);
    *success = resultCode >= minExpectedResult
        && resultCode <= maxExpectedResult;
    return true;
  }

public:
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
  static bool alwaysSucceed(bool *success, QString *errorString, int resultCode,
                            QString result) {
    Q_UNUSED(result)
    Q_UNUSED(resultCode)
    Q_UNUSED(errorString)
    Q_ASSERT(success);
    Q_ASSERT(errorString);
    *success = true;
    return true;
  }
};

// TODO fix resource leaks in functional methods and lambdas
// "delete buffer" and "delete top" won't be called if hte command is not
// executed and finished, e.g. if the script is deleted before execution or
// aborted before reaching this particular command
struct FtpScriptData : public QSharedData {
  //int _id;
  FtpClient *_client;
  QVector<FtpCommand> _commands;
  QString _host, _login, _password, _cwd;
  quint16 _port, _pasvPort;
  QStack<QString> _dirstack;
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
  bool pwdIsFinished(bool *success, QString *errorString, int resultCode,
                     QString result, QString *path) {
    Q_UNUSED(errorString)
    Q_UNUSED(resultCode)
    Q_ASSERT(success);
    QRegularExpressionMatch match = _pwdRe.match(result);
    if (match.hasMatch()) {
      _cwd = match.captured(1);
      if (path)
        *path = _cwd;
      *success = true;
      qDebug() << "  cwd is now" << _cwd;
    } else {
      if (path)
        *path = QString();
      *success = false;
    }
    return true;
  }
  bool memoryTransferIsFinished(
      bool *success, QString *errorString, int resultCode, QString result,
      QIODevice *localDevice) {
    Q_ASSERT(localDevice);
    bool finished = transferIsFinished(success, errorString, resultCode,
                                       result);
    if (finished)
      delete localDevice;
    return finished;
  }
  bool nlstTransferIsFinished(
      bool *success, QString *errorString, int resultCode, QString result,
      QBuffer *buffer, QStringList *basenames) {
    Q_ASSERT(buffer);
    Q_ASSERT(basenames);
    bool finished = transferIsFinished(success, errorString, resultCode,
                                       result);
    if (finished) {
      *basenames = QStringList();
      foreach (const QString &name, QString::fromUtf8(buffer->data())
#if QT_VERSION >= 0x050f00
               .split(_newlineRe, Qt::SkipEmptyParts)) {
#else
               .split(_newlineRe, QString::SkipEmptyParts)) {
#endif
        // never list . and ..
        if (name == "." || name == "..")
          continue;
        // next line is ok because mid(-1+1) = mid(0) = whole string
        //qDebug() << "ls:" << name << name.lastIndexOf('/')
        //         << name.mid(name.lastIndexOf('/')+1);
        //basenames->append(name.mid(name.lastIndexOf('/')+1));
        // FIXME
        basenames->append(name);
      }
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

static QRegularExpression _intermidiaryStatusLineRE("\\A(1|\\d+-)");

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
    QString cmd;
    if (command._command)
      cmd = command._command();
    qDebug() << "  command" << cmd;
    if (!cmd.isEmpty()) {
      int written = d->_client->_controlSocket->write(cmd.toUtf8()+"\r\n");
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
        qDebug() << "successfuly executed FTP command" << cmd << result;
        ++i;
      } else {
        qDebug() << "error when executing FTP command" << cmd << result;
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
    d->_commands.append(FtpCommand("OPTS UTF8 ON", &FtpCommand::alwaysSucceed));
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
    d->_commands.append(FtpCommand("SITE UMASK 22", &FtpCommand::alwaysSucceed));
    d->_commands.append(
          FtpCommand("PWD", std::bind(&FtpScriptData::pwdIsFinished, d, _1, _2,
                                      _3, _4, nullptr)));
  }
  return *this;
}

FtpScript &FtpScript::cd(QString path) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("CWD "+path));
    d->_commands.append(
          FtpCommand("PWD", std::bind(&FtpScriptData::pwdIsFinished, d, _1, _2,
                                      _3, _4, nullptr)));
  }
  return *this;
}

FtpScript &FtpScript::pushd(QString path) {
  FtpScriptData *d = _data;
  if (d) {
    // pwd needed before in case no cd or connectToHost has been performed
    // before pushd in *this* script (i.e. the client was connected in another
    // script
    // LATER find a way to avoid this if cwd has been set before
    d->_commands.append(
          FtpCommand("PWD", std::bind(&FtpScriptData::pwdIsFinished, d, _1, _2,
                                      _3, _4, nullptr)));
    if (path.isEmpty()) { // swaping dir with top of stack
      QString *top = new QString;
      d->_commands.append(FtpCommand([d, top](){
        *top = d->_dirstack.isEmpty() ? d->_cwd : d->_dirstack.top();
        d->_dirstack.push(d->_cwd);
        qDebug() << "  dirs stack is now" << d->_dirstack
                 << "after pushd w/o param";
      }, [d, top]() {
        QString tmp = *top;
        delete top;
        return "CWD "+tmp;
      }));
    } else { // regular push
      d->_commands.append(FtpCommand([d](){
        d->_dirstack.push(d->_cwd);
        qDebug() << "  dirs stack is now" << d->_dirstack
                 << "after pushd w/ param";
      }, "CWD "+path));
    }
    d->_commands.append(
          FtpCommand("PWD", std::bind(&FtpScriptData::pwdIsFinished, d, _1, _2,
                                      _3, _4, nullptr)));
  }
  return *this;
}

FtpScript &FtpScript::popd() {
  FtpScriptData *d = _data;
  if (d) {
    QString *top = new QString;
    d->_commands.append(FtpCommand([d, top](){
      *top = d->_dirstack.isEmpty() ? "." : d->_dirstack.pop();
      qDebug() << "  dirs stack is now" << d->_dirstack << "after popd";
    }, [d, top]() {
      QString tmp = *top;
      delete top;
      return "CWD "+tmp;
    }));
    d->_commands.append(
          FtpCommand("PWD", std::bind(&FtpScriptData::pwdIsFinished, d, _1, _2,
                                      _3, _4, nullptr)));
  }
  return *this;
}

FtpScript &FtpScript::pwd(QString *path) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(
          FtpCommand("PWD", std::bind(&FtpScriptData::pwdIsFinished, d, _1, _2,
                                      _3, _4, path)));
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
    d->_commands.append(FtpCommand("MKD "+path, &FtpCommand::alwaysSucceed));
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
    d->_commands.append(FtpCommand("RMD "+path, &FtpCommand::alwaysSucceed));
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
    d->_commands.append(FtpCommand("DELE "+path, &FtpCommand::alwaysSucceed));
  }
  return *this;
}

FtpScript &FtpScript::ls(QStringList *basenames, QString path) {
  FtpScriptData *d = _data;
  if (d) {
    bool subdir = !path.isEmpty() && path != u"."_s;
    if (subdir)
      pushd(path);
    d->_commands.append(FtpCommand( "PASV",
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4)));
    QBuffer *buf = new QBuffer(d->_client);
    buf->open(QIODevice::WriteOnly);
    d->_commands.append(FtpCommand([d, buf, basenames]() {
      d->_client->download(d->_pasvPort, buf);
    }, "NLST",
    std::bind(&FtpScriptData::nlstTransferIsFinished, d, _1, _2, _3, _4, buf,
              basenames)));
    if (subdir)
      popd();
  }
  return *this;
}

FtpScript &FtpScript::get(QString path, QIODevice *dest) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("PASV",
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4)));
    d->_commands.append(FtpCommand([d, dest]() {
      d->_client->download(d->_pasvPort, dest);
    }, "RETR "+path,
    std::bind(&FtpScriptData::transferIsFinished, d, _1, _2, _3, _4)));
  }
  return *this;
}

FtpScript &FtpScript::get(QString path, QByteArray *dest) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("PASV",
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4)));
    QBuffer *buf = new QBuffer(dest, d->_client);
    buf->open(QIODevice::WriteOnly);
    d->_commands.append(FtpCommand([d, buf]() {
      d->_client->download(d->_pasvPort, buf);
    }, "RETR "+path,
    std::bind(&FtpScriptData::memoryTransferIsFinished, d, _1, _2, _3, _4,
              buf)));
  }
  return *this;
}

FtpScript &FtpScript::get(QString path, QString localPath) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("PASV",
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4)));
    QFile *file = new QFile(localPath);
    d->_commands.append(FtpCommand([d, file]() {
      file->open(QIODevice::WriteOnly|QIODevice::Truncate);
      d->_client->download(d->_pasvPort, file);
    }, "RETR "+path,
    std::bind(&FtpScriptData::memoryTransferIsFinished, d, _1, _2, _3, _4,
              file)));
  }
  return *this;
}

FtpScript &FtpScript::put(QString path, QIODevice *source) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("PASV",
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4)));
    d->_commands.append(FtpCommand([d, source]() {
      d->_client->upload(d->_pasvPort, source);
    }, "STOR "+path,
    std::bind(&FtpScriptData::transferIsFinished, d, _1, _2, _3, _4)));
  }
  return *this;
}

FtpScript &FtpScript::put(QString path, QByteArray source) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("PASV",
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4)));
    QBuffer *buf = new QBuffer(d->_client);
    buf->setData(source);
    buf->open(QIODevice::ReadOnly);
    d->_commands.append(FtpCommand([d, buf]() {
      d->_client->upload(d->_pasvPort, buf);
    }, "STOR "+path,
    std::bind(&FtpScriptData::memoryTransferIsFinished, d, _1, _2, _3, _4,
              buf)));
  }
  return *this;
}

FtpScript &FtpScript::put(QString path, QString localPath) {
  FtpScriptData *d = _data;
  if (d) {
    d->_commands.append(FtpCommand("PASV",
    std::bind(&FtpScriptData::pasvIsFinished, d, _1, _2, _3, _4)));
    QFile *file = new QFile(localPath);
    file->open(QIODevice::ReadOnly);
    d->_commands.append(FtpCommand([d, file]() {
      d->_client->upload(d->_pasvPort, file);
    }, "STOR "+path,
    std::bind(&FtpScriptData::memoryTransferIsFinished, d, _1, _2, _3, _4,
              file)));
  }
  return *this;
}
