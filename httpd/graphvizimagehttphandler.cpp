/* Copyright 2013-2017 Hallowyn, Gregoire Barbier and others.
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
#include "graphvizimagehttphandler.h"
#include <QCoreApplication>
#include "log/log.h"
#include <QThread>
#include <unistd.h>

#define UPDATE_EVENT (QEvent::Type(QEvent::User+1))


GraphvizImageHttpHandler::GraphvizImageHttpHandler(QObject *parent,
                                                   RefreshStrategy refreshStrategy)
  : ImageHttpHandler(parent), _renderer(Dot), _renderingRequested(false),
    _renderingRunning(false), _renderingNeeded(0), _mutex(QMutex::Recursive),
    _process(new QProcess(this)), _refreshStrategy(refreshStrategy),
    _imageFormat(Png) {
  connect(_process, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
          this, &GraphvizImageHttpHandler::processFinished);
#if QT_VERSION >= 0x050600
  connect(_process, &QProcess::errorOccurred,
          this, &GraphvizImageHttpHandler::processError);
#endif
  connect(_process, &QProcess::readyReadStandardOutput,
          this, &GraphvizImageHttpHandler::readyReadStandardOutput);
  connect(_process, &QProcess::readyReadStandardError,
          this, &GraphvizImageHttpHandler::readyReadStandardError);
}

QByteArray GraphvizImageHttpHandler::imageData(ParamsProvider *params,
                                               int timeoutMillis) {
  Q_UNUSED(params)
  QMutexLocker ml(&_mutex);
  if (_refreshStrategy == OnDemandWithCache && _renderingNeeded) {
    //Log::debug() << "imageData() with rendering needed";
    qint64 deadline = QDateTime::currentMSecsSinceEpoch()+timeoutMillis;
    ml.unlock();
    if (QThread::currentThread() == thread())
      startRendering();
    else
      QCoreApplication::postEvent(this, new QEvent(UPDATE_EVENT));
    while (QDateTime::currentMSecsSinceEpoch() <= deadline
           && _renderingNeeded) {
      if (QThread::currentThread() == thread()) {
        if (_process->waitForFinished(
              deadline - QDateTime::currentMSecsSinceEpoch())) {
          processFinished(_process->exitCode(), _process->exitStatus());
          break; // avoid testing _renderingNeeded: in case a new modification
          // has been done meanwhile we don't want to wait again for a rendering
        }
      } else
        usleep(qMin(50000LL, deadline - QDateTime::currentMSecsSinceEpoch()));
    }
    ml.relock();
  }
  return _imageData;
}

QString GraphvizImageHttpHandler::contentType(ParamsProvider *params) const {
  Q_UNUSED(params)
  QMutexLocker ml(&_mutex);
  return _contentType;
}

QString GraphvizImageHttpHandler::contentEncoding(
    ParamsProvider *params) const {
  Q_UNUSED(params)
  QMutexLocker ml(&_mutex);
  return (_imageFormat == Svgz) ? "gzip" : QString();
}

QString GraphvizImageHttpHandler::source(ParamsProvider *params) const {
  Q_UNUSED(params)
  QMutexLocker ml(&_mutex);
  return _source;
}

void GraphvizImageHttpHandler::setSource(QString source) {
  QMutexLocker ml(&_mutex);
  _source = source;
  ++_renderingNeeded;
  if (_refreshStrategy == OnChange) {
    //Log::debug() << "setSource() with OnChange strategy";
    QCoreApplication::postEvent(this, new QEvent(UPDATE_EVENT));
  }
}

void GraphvizImageHttpHandler::customEvent(QEvent *event) {
  if (event->type() == UPDATE_EVENT) {
    QCoreApplication::removePostedEvents(this, UPDATE_EVENT);
    startRendering();
  } else {
    ImageHttpHandler::customEvent(event);
  }
}

void GraphvizImageHttpHandler::startRendering() {
  QMutexLocker ml(&_mutex);
  if (_renderingRunning)
    return; // postponing after currently running rendering
  _renderingRequested = false;
  _renderingRunning = true;
  _tmp.clear();
  _stderr.clear();
  QString cmd = "dot"; // default to dot
  switch (_renderer) {
  case Neato:
    cmd = "neato";
    break;
  case TwoPi:
    cmd = "twopi";
    break;
  case Circo:
    cmd = "circo";
    break;
  case Dot:
    cmd = "dot";
    break;
  case Fdp:
    cmd = "fdp";
    break;
  case Sfdp:
    cmd = "sfdp";
    break;
  case Osage:
    cmd = "osage";
    break;
  }
  QStringList args;
  switch (_imageFormat) {
  case Png:
    args << "-Tpng";
    break;
  case Svg:
    args << "-Tsvg";
    break;
  case Svgz:
    args << "-Tsvgz";
    break;
  case Plain:
    args << "-Tdot";
    break;
  }
  QByteArray ba = _source.toUtf8();
  Log::debug() << "starting graphviz rendering with this data: "
               << _source;
  _process->start(cmd, args);
  _process->waitForStarted();
  qint64 written = _process->write(ba);
  if (written != ba.size())
    Log::debug() << "cannot write to graphviz processor "
                 << written << " " << ba.size() << " "
                 << _process->error() << " " << _process->errorString();
  _process->closeWriteChannel();
}

void GraphvizImageHttpHandler::processError(QProcess::ProcessError error) {
  readyReadStandardError();
  readyReadStandardOutput();
  Log::warning() << "graphviz rendering process crashed with "
                    "QProcess::ProcessError code " << error << " ("
                 << _process->errorString() << ") and stderr content: "
                 << _stderr;
  _process->kill();
  processFinished(-1, QProcess::CrashExit);
}

void GraphvizImageHttpHandler::processFinished(
    int exitCode, QProcess::ExitStatus exitStatus) {
  QMutexLocker ml(&_mutex);
  if (!_renderingRunning)
    return; // avoid double execution of processFinished in OnDemand strategy
  readyReadStandardError();
  readyReadStandardOutput();
  bool success = (exitStatus == QProcess::NormalExit && exitCode == 0);
  if (success) {
    Log::debug() << "graphviz rendering process successful with return code "
                 << exitCode << " and QProcess::ExitStatus " << exitStatus
                 << " having produced a " << _tmp.size() << " bytes output";
    _imageData = _tmp;
  } else {
    Log::warning() << "graphviz rendering process failed with return code "
                   << exitCode << ", QProcess::ExitStatus " << exitStatus
                   << " and stderr content: " << _stderr;
    //_contentType = "text/plain;charset=UTF-8";
    _imageData = _stderr.toUtf8(); // LATER placeholder image
  }
  _renderingRunning = false;
  if (_refreshStrategy == OnChange) {
    if (_renderingRequested) {
      ml.unlock();
      startRendering();
    }
  } else {
    if (_renderingNeeded > 1) {
      _renderingNeeded = 1;
      ml.unlock();
      startRendering();
    } else {
      _renderingNeeded = 0;
      ml.unlock();
    }
  }
  emit contentChanged();
  _tmp.clear();
  _stderr.clear();
}

void GraphvizImageHttpHandler::readyReadStandardOutput() {
  _process->setReadChannel(QProcess::StandardOutput);
  QByteArray ba;
  while (!(ba = _process->read(1024)).isEmpty())
    _tmp.append(ba);
}

void GraphvizImageHttpHandler::readyReadStandardError() {
  _process->setReadChannel(QProcess::StandardError);
  QByteArray ba;
  while (!(ba = _process->read(1024)).isEmpty())
    _stderr.append(QString::fromUtf8(ba));
}

void GraphvizImageHttpHandler::setImageFormat(ImageFormat imageFormat) {
  QMutexLocker ml(&_mutex);
  _imageFormat = imageFormat;
  switch (_imageFormat) {
  case Png:
    _contentType = "image/png";
    break;
  case Svg:
    _contentType = "image/svg+xml";
    break;
  case Svgz:
    _contentType = "image/svg+xml";
    break;
  case Plain:
    _contentType = "text/plain;charset=UTF-8";
    break;
  }
}
