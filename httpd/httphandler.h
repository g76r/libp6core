/* Copyright 2012-2019 Hallowyn, Gregoire Barbier and others.
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
#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include "httprequest.h"
#include "httpresponse.h"
#include "util/paramsprovidermerger.h"
#include <QObject>
#include <QRegularExpression>

/** HttpHandler is responsible for handling every HTTP request the server
 * receives.
 * Its method must all be thread-safe since they will be called by HttpWorker
 * threads.
 * When registred with an HttpServer, an HttpHandler stays within its previous
 * thread and keep its previous parent, however, it will be deleted at
 * HttpServer destruction, through deleteLater(). If the HttpHandler interract
 * with other QObjects (for instance with models) it may be a good idea that it
 * share the same thread than these objects (the main thread or a dedicated
 * one).
 */
class LIBPUMPKINSHARED_EXPORT HttpHandler : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(HttpHandler)
  // LATER give handlers their own threads and make server and handler threads exchange signals
  QString _name;
  QList<QRegularExpression> _corsOrigins;

public:
  HttpHandler(QString name, QObject *parent = 0);
  HttpHandler(QObject *parent = 0) : HttpHandler(QString(), parent) { }
  virtual QString name() const;
  QList<QRegularExpression> corsOrigins() const { return _corsOrigins; }
  /** List of regular expression matching Origin: header.
   * A star "*" as any element of the list or an empty list means "any origin".
   * @default content of semi colon separated env var CORS_ORIGINS or if empty
   *  CORS_DOMAINS.
   */
  void setCorsOrigins(QList<QRegularExpression> corsDomains) {
    _corsOrigins = corsDomains; }
  /** Return true iff the handler accept to handle the request.
   * Thread-safe (called by several HttpWorker threads at the same time). */
  virtual bool acceptRequest(HttpRequest req) = 0;
  /** Handle the request.
   * Thread-safe (called by several HttpWorker threads at the same time).
   * @param processingContext is shared accross the whole processing of the
   *   request (i.e. if the process is a pipeline, each step will be able to
   *   modify the context for the next one)
   * @return false if a failure should interrupt the process */
  virtual bool handleRequest(HttpRequest req, HttpResponse res,
                             ParamsProviderMerger *processingContext) = 0;
  /** Perform redirect to clean up url if needed.
   * Currently the only case is when the path contains several adjacent /
   * The redirect is performed with relative path.
   * @return true iff redirect was issued
   */
  bool redirectForUrlCleanup(HttpRequest req, HttpResponse res,
                             ParamsProviderMerger *processingContext);
  /** Handle OPTIONS preflight request.
   * @return true iff the request was a preflight and was handled (therefore if
   * handleRequest() should stop here rather than handling a GET/POST/whatever)
   */
  bool handlePreflight(HttpRequest req, HttpResponse res,
                       ParamsProviderMerger *processingContext,
                       QSet<QString> methods);
};

#endif // HTTPHANDLER_H
