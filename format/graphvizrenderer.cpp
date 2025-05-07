/* Copyright 2024-2025 Hallowyn, Gregoire Barbier and others.
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
#include "graphvizrenderer.h"
#include "util/paramsprovidermerger.h"
#include "util/containerutils.h"
#include <QMap>
#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <QtDebug>
#include <QDateTime>

GraphvizRenderer::GraphvizRenderer(QObject *parent,
    const Utf8String &source, Layout layout, Format format, int timeoutms,
    const ParamSet &params)
  : QProcess(parent), _source(source), _layout(layout), _format(format),
    _params(params), _timeoutms(timeoutms) {
  connect(this, &QProcess::finished,
          this, &GraphvizRenderer::process_finished);
  connect(this, &QProcess::errorOccurred,
          this, &GraphvizRenderer::process_error);
  connect(this, &QProcess::readyReadStandardOutput,
          this, &GraphvizRenderer::read_stdout);
  connect(this, &QProcess::readyReadStandardError,
          this, &GraphvizRenderer::read_stderr);
}

Utf8String GraphvizRenderer::run(
    const ParamsProvider &context, const Utf8String &start_source) {
  QMutexLocker ml(&_mutex);
  _output.clear();
  do_start(context, start_source);
  if (QThread::currentThread() == thread()) {
    do QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    while (_output.isEmpty());
  } else {
    do QThread::usleep(10'000);
    while (_output.isEmpty());
  }
  return _output;
}

void GraphvizRenderer::do_start(
    const ParamsProvider &context, const Utf8String &start_source) {
  auto ppm = ParamsProviderMerger(&context)(_params);
  auto source = start_source | ppm.paramRawUtf8("source") | _source;
  Format format = formatFromString(ppm.paramRawUtf8("format"), _format);
  Layout layout = layoutFromString(ppm.paramRawUtf8("layout"), _layout);
  int timeoutms = ppm.paramNumber<double>("timeout", _timeoutms/1e3)*1e3;
  if (timeoutms > 0) {
    _timout_timer = new QTimer(this);
    connect(_timout_timer, &QTimer::timeout, this, &GraphvizRenderer::kill);
    _timout_timer->setSingleShot(true);
    _timout_timer->start(timeoutms);
  }
  auto command = layout == UnknownLayout ? "false"_u8 : layoutAsString(layout);
  QStringList options = _options;
  options << "-T"+formatAsString(format);
  qDebug() << "graphviz rendering process starting" << command << options;
  _startms = QDateTime::currentMSecsSinceEpoch();
  QProcess::start(command, options);
  waitForStarted();
  qint64 written = write(source);
  if (written != source.size())
    qDebug() << "cannot write to graphviz processor" << written << source.size()
             << (int)error() << errorString();
  closeWriteChannel();
}

void GraphvizRenderer::process_finished(
    int exitCode, QProcess::ExitStatus exitStatus) {
  auto elapsed = QDateTime::currentMSecsSinceEpoch() - _startms;
  if (_timout_timer) {
    _timout_timer->stop();
    _timout_timer->deleteLater();
    _timout_timer = 0;
  }
  read_stderr();
  read_stdout();
  bool success = (exitStatus == QProcess::NormalExit && exitCode == 0);
  if (success) {
    qDebug() << "graphviz rendering process successful with return code"
             << exitCode << "and QProcess::ExitStatus" << (int)exitStatus
             << "having produced a" << _tmp.size() << "bytes output in"
             << elapsed/1e3 << "seconds";
    _output = _tmp | "empty"_u8;
  } else {
    qWarning() << "graphviz rendering process failed with return code"
               << exitCode << ", QProcess::ExitStatus" << (int)exitStatus
               << "after" << elapsed/1e3 << "seconds" << "with error:"
               << errorString() << "and stderr content:" << _stderr;
    _output = _stderr | "error"_u8; // LATER placeholder image
  }
  _tmp.clear();
  _stderr.clear();
}

void GraphvizRenderer::process_error(QProcess::ProcessError error) {
  read_stderr();
  read_stdout();
  qWarning() << "graphviz rendering process crashed with QProcess::ProcessError"
                " code" << (int)error << "(" << errorString()
             << ") and stderr content:" << _stderr;
  kill();
  // calling this would create race condition on _output:
  // process_finished(-1, QProcess::CrashExit);
}

void GraphvizRenderer::read_stdout() {
  setReadChannel(QProcess::StandardOutput);
  Utf8String s;
  while (!(s = read(4096)).isEmpty())
    _tmp.append(s);
}

void GraphvizRenderer::read_stderr() {
  setReadChannel(QProcess::StandardError);
  Utf8String s;
  while (!(s = read(4096)).isEmpty())
    _stderr.append(s);
}

static QMap<Utf8String,GraphvizRenderer::Format> _formatFromString {
  { "unknown", GraphvizRenderer::UnknownFormat },
  { "png", GraphvizRenderer::Png },
  { "svg", GraphvizRenderer::Svg },
  { "svgz", GraphvizRenderer::Svgz },
  { "plain", GraphvizRenderer::Plain },
  { "dot", GraphvizRenderer::Gv }, // hidden by gv in reversed _formatAsString
  { "gv", GraphvizRenderer::Gv },
  { "xdot", GraphvizRenderer::Xdot },
};

static auto _formatAsString = ContainerUtils::reversed_map(_formatFromString);

GraphvizRenderer::Format GraphvizRenderer::formatFromString(
    const Utf8String &s, GraphvizRenderer::Format def) {
  return _formatFromString.value(s, def);
}

Utf8String GraphvizRenderer::formatAsString(
    GraphvizRenderer::Format format) {
  return _formatAsString.value(format, "dot"_u8);
}

static QMap<Utf8String,GraphvizRenderer::Layout> _layoutFromString {
  { "unknown", GraphvizRenderer::UnknownLayout },
  { "dot", GraphvizRenderer::Dot },
  { "neato", GraphvizRenderer::Neato },
  { "twopi", GraphvizRenderer::TwoPi },
  { "circo", GraphvizRenderer::Circo },
  { "fdp", GraphvizRenderer::Fdp },
  { "sfdp", GraphvizRenderer::Sfdp },
  { "osage", GraphvizRenderer::Osage },
};

static auto _layoutAsString = ContainerUtils::reversed_map(_layoutFromString);

GraphvizRenderer::Layout GraphvizRenderer::layoutFromString(
    const Utf8String &s, GraphvizRenderer::Layout def) {
  return _layoutFromString.value(s, def);
}

Utf8String GraphvizRenderer::layoutAsString(GraphvizRenderer::Layout layout) {
  return _layoutAsString.value(layout, "dot"_u8);
}

Utf8String GraphvizRenderer::mime_type(Format format) {
  switch (format) {
  case Png:
    return "image/png"_u8;
  case Svg:
  case Svgz:
    return "image/svg+xml"_u8;
  case Plain:
  case Gv:
  case Xdot:
    return "text/plain;charset=UTF-8"_u8;
  case UnknownFormat:
    ;
  }
  return "application/octet-stream"_u8;
}
