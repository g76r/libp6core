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
#include "pipelinehttphandler.h"
#include "log/log.h"

bool PipelineHttpHandler::acceptRequest(HttpRequest &req) {
  return _urlPathPrefix.isEmpty()
      || req.path().startsWith(_urlPathPrefix.toUtf8());
}

bool PipelineHttpHandler::handleRequest(
    HttpRequest &req, HttpResponse &res,
    ParamsProviderMerger &request_context) {
  if (_handlers.isEmpty()) {
    res.set_status(HttpResponse::HTTP_Not_Found);
    res.output()->write("Error 404 - Not found");
    [[unlikely]] return true;
  }
  for (HttpHandler *handler: _handlers)
    if (!handler->handleRequest(req, res, request_context))
      return false;
  return true;
}
