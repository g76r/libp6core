/* Copyright 2013-2021 Hallowyn, Gregoire Barbier and others.
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
#include <QRecursiveMutex>
#include <QProcess>
#include <QBuffer>

class LIBP6CORESHARED_EXPORT GraphvizImageHttpHandler
    : public ImageHttpHandler {
  Q_OBJECT
  Q_DISABLE_COPY(GraphvizImageHttpHandler)

public:
  enum GraphvizRenderer { Dot, Neato, TwoPi, Circo, Fdp, Sfdp, Osage };
  enum RefreshStrategy { OnChange, OnDemandWithCache };
  enum ImageFormat { Png, Svg, Svgz, Plain };

private:
  GraphvizRenderer _renderer;
  QString _source, _stderr, _contentType;
  bool _renderingRequested, _renderingRunning;
  int _renderingNeeded;
  mutable QRecursiveMutex _mutex;
  QProcess *_process;
  QByteArray _imageData, _tmp;
  RefreshStrategy _refreshStrategy;
  ImageFormat _imageFormat;

public:
  explicit GraphvizImageHttpHandler(
      QObject *parent = 0, RefreshStrategy refreshStrategy = OnDemandWithCache);
  QByteArray imageData(
      ParamsProvider *params = 0, int timeoutMillis
      = IMAGEHTTPHANDLER_DEFAULT_ONDEMAND_RENDERING_TIMEOUT) override;
  QString contentType(ParamsProvider *params = 0) const override;
  QString contentEncoding(ParamsProvider *params) const override;
  QString source(ParamsProvider *params = 0) const override;
  GraphvizRenderer renderer() const { return _renderer; }
  void setRenderer(GraphvizRenderer renderer) { _renderer = renderer; }
  RefreshStrategy refreshStrategy() const { return _refreshStrategy; }
  ImageFormat imageFormat() const;
  void setImageFormat(ImageFormat imageFormat);

public slots:
  /** Set new graphviz-format source and, if refresh strategy is OnChange,
   * trigger image layout processing */
  void setSource(QString source);

protected:
  void customEvent(QEvent *event) override;

private slots:
  void processError(QProcess::ProcessError error);
  void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void readyReadStandardOutput();
  void readyReadStandardError();

private:
  void startRendering();
};

#endif // GRAPHVIZIMAGEHTTPHANDLER_H
