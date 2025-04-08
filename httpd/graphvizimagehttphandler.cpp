/* Copyright 2013-2025 Hallowyn, Gregoire Barbier and others.
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
#include "httpd/httpworker.h"
#include "log/log.h"

GraphvizImageHttpHandler::GraphvizImageHttpHandler(
    QObject *parent, Layout layout, Format format)
  : ImageHttpHandler(parent), _layout(layout), _format(format),
    _renderingNeeded(false) {
}

QByteArray GraphvizImageHttpHandler::imageData(
    HttpRequest req, ParamsProviderMerger *context, int timeoutMillis) {
  QMutexLocker ml(&_mutex);
  if (!_renderingNeeded)
    return _data;
  if (_source.isEmpty()) {
    _data.clear();
    _renderingNeeded = false;
    return _data;
  }
  auto gvr = new GraphvizRenderer(req.worker(), _format, timeoutMillis);
  _data = gvr->run(context, _source);
  gvr->deleteLater();
  _renderingNeeded = false;
  return _data;
}

QByteArray GraphvizImageHttpHandler::contentType(
  HttpRequest, ParamsProviderMerger *) const {
  QMutexLocker ml(&_mutex);
  return _contentType;
}

QByteArray GraphvizImageHttpHandler::contentEncoding(
    HttpRequest, ParamsProviderMerger *) const {
  QMutexLocker ml(&_mutex);
  return (_format == Svgz) ? "gzip"_u8 : QByteArray{};
}

QByteArray GraphvizImageHttpHandler::source(
  HttpRequest, ParamsProviderMerger *) const {
  QMutexLocker ml(&_mutex);
  return _source;
}

void GraphvizImageHttpHandler::setSource(const QByteArray &source) {
  QMutexLocker ml(&_mutex);
  _source = source;
  _renderingNeeded = true;
}

void GraphvizImageHttpHandler::setLayout(Layout layout) {
  QMutexLocker ml(&_mutex);
  _layout = layout;
  _renderingNeeded = true;
}

void GraphvizImageHttpHandler::setFormat(Format format) {
  QMutexLocker ml(&_mutex);
  _format = format;
  _contentType = GraphvizRenderer::mime_type(format);
  _renderingNeeded = true;
}
