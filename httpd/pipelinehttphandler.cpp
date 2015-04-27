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
#include "pipelinehttphandler.h"
#include "log/log.h"

bool PipelineHttpHandler::acceptRequest(HttpRequest req) {
  return _urlPathPrefix.isEmpty()
      || req.url().path().startsWith(_urlPathPrefix);
}

bool PipelineHttpHandler::handleRequest(
    HttpRequest req, HttpResponse res, ParamsProviderMerger *processingContext) {
  if (_handlers.isEmpty()) {
    res.setStatus(404);
    res.output()->write("Error 404 - Not found");
    return true;
  }
  foreach (HttpHandler *handler, _handlers) {
    if (!handler) {
      res.setStatus(404);
      res.output()->write("Error 404 - Not found");
      Log::error() << "PipelineHttpHandler containing a null handler";
      return false;
    }
    if (!handler->handleRequest(req, res, processingContext))
      return false;
  }
  return true;
}
