/*
Copyright 2012 Hallowyn and others.
See the NOTICE file distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file to you under
the Apache License, Version 2.0 (the "License"); you may not use this
file except in compliance with the License. You may obtain a copy of the
License at http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
License for the specific language governing permissions and limitations
under the License.
*/
#include "uriprefixhandler.h"

UriPrefixHandler::UriPrefixHandler(QObject *parent, const QString &prefix,
                                   int allowedMethods)
  : HttpHandler(parent), _prefix(prefix), _allowedMethods(allowedMethods) {
}

QString UriPrefixHandler::name() const {
  return "UriPrefixHandler:" + _prefix;
}

bool UriPrefixHandler::acceptRequest(const HttpRequest &req) {
  if ((req.method()&_allowedMethods) && req.url().path().startsWith(_prefix))
    return true;
  return false;
}
