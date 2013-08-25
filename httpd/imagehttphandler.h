/* Copyright 2013 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
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
#ifndef IMAGEHTTPHANDLER_H
#define IMAGEHTTPHANDLER_H

#include "httphandler.h"

class LIBQTSSUSHARED_EXPORT ImageHttpHandler : public HttpHandler {
  Q_OBJECT
  Q_DISABLE_COPY(ImageHttpHandler)
  QString _urlPathPrefix;
public:
  explicit ImageHttpHandler(QObject *parent = 0);
  explicit ImageHttpHandler(QString urlPathPrefix, QObject *parent = 0);
  bool acceptRequest(HttpRequest req);
  bool handleRequest(HttpRequest req, HttpResponse res,
                     HttpRequestContext ctxt);
  /** This method must be thread-safe for the same reasons than
   * handleRequest() */
  virtual QByteArray imageData(ParamsProvider *params = 0) const = 0;
  /** This method must be thread-safe for the same reasons than
   * handleRequest() */
  virtual QString contentType(ParamsProvider *params = 0) const = 0;
  /** Return a source code or text for image, if any.
   * Default: QString()
   * This method must be thread-safe for the same reasons than
   * handleRequest() */
  virtual QString source(ParamsProvider *params = 0) const;
  // LATER sourceMimeType and imageMimeType

signals:
  /** Emited when content has changed and the new one is available (after
   * background/asynchronous update process if any). */
  void contentChanged();
};

#endif // IMAGEHTTPHANDLER_H
