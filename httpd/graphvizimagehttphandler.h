/* Copyright 2013-2024 Hallowyn, Gregoire Barbier and others.
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
#ifndef GRAPHVIZIMAGEHTTPHANDLER_H
#define GRAPHVIZIMAGEHTTPHANDLER_H

#include "imagehttphandler.h"
#include "format/graphvizrenderer.h"
#include <QMutex>
#include <QProcess>

class LIBP6CORESHARED_EXPORT GraphvizImageHttpHandler
    : public ImageHttpHandler {
  Q_OBJECT
  Q_DISABLE_COPY(GraphvizImageHttpHandler)

public:
  using Layout = GraphvizRenderer::Layout;
  using enum GraphvizRenderer::Layout;
  using Format = GraphvizRenderer::Format;
  using enum GraphvizRenderer::Format;

private:
  Layout _layout;
  Format _format;
  Utf8String _source, _contentType;
  bool _renderingNeeded;
  mutable QMutex _mutex;
  QByteArray _data;

public:
  explicit GraphvizImageHttpHandler(
      QObject *parent = 0, Layout layout = Dot, Format format = Svg);
  QByteArray imageData(
      HttpRequest req, ParamsProviderMerger *params = 0, int timeoutMillis
      = IMAGEHTTPHANDLER_DEFAULT_ONDEMAND_RENDERING_TIMEOUT) override;
  QByteArray contentType(
    HttpRequest req, ParamsProviderMerger *processingContext) const override;
  QByteArray contentEncoding(
    HttpRequest req, ParamsProviderMerger *processingContext) const override;
  QByteArray source(
    HttpRequest req, ParamsProviderMerger *processingContext) const override;
  Layout layout() const { return _layout; }
  void setLayout(Layout layout);
  Format format() const { return _format; }
  void setFormat(Format format);

public slots:
  /** Set new graphviz-format source and, if refresh strategy is OnChange,
   * trigger image layout processing */
  void setSource(const QByteArray &source);
};

#endif // GRAPHVIZIMAGEHTTPHANDLER_H
