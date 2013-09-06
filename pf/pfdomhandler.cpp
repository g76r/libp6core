/* Copyright 2012-2013 Hallowyn and others.
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

#include "pfdomhandler.h"
#include "pfinternals.h"

PfDomHandler::PfDomHandler() : PfHandler() {
}

PfDomHandler::~PfDomHandler() {
}

bool PfDomHandler::startDocument(PfOptions options) {
  Q_UNUSED(options)
  _roots.clear();
  _path.clear();
  return true;
}

bool PfDomHandler::startNode(QList<QString> names) {
  PfNode node(names.last());
  _path.append(node);
  return true;
}

bool PfDomHandler::text(QString text) {
  if (_path.size() == 0) {
    setErrorString(tr("text data before root node"));
    return false;
  }
  if (!_path.last().contentIsEmpty())
    _path.last().appendContent(QString(" ")+text);
  else
    _path.last().appendContent(text);
  return true;
}

bool PfDomHandler::binary(QIODevice *device, qint64 length, qint64 offset,
                          QString surface) {
  if (_path.size() == 0) {
    setErrorString(tr("binary data before root node"));
    return false;
  }
  _path.last().appendContent(device, length, offset, surface);
  return true;
}

bool PfDomHandler::binary(QByteArray data, QString surface) {
  if (_path.size() == 0) {
    setErrorString(tr("binary data before root node"));
    return false;
  }
  _path.last().appendContent(data, surface);
  return true;
}

bool PfDomHandler::array(PfArray array) {
  if (_path.size() == 0) {
    setErrorString(tr("array data before root node"));
    return false;
  }
  if (options().shouldTranslateArrayIntoTree()) {
    PfNode node(_path.takeLast());
    array.convertToChildrenTree(&node);
    _path.append(node);
  } else {
    _path.last().setContent(array);
  }
  return true;
}

bool PfDomHandler::endNode(QList<QString> names) {
  Q_UNUSED(names);
  PfNode node(_path.takeLast());
  if (!_path.isEmpty())
    _path.last().appendChild(node);
  else
    _roots.append(node);
  return true;
}

bool PfDomHandler::comment(QString content) {
  PfNode node("", content, true);
  if (_path.size())
    _path.last().appendChild(node);
  else
    _roots.append(node);
  return true;
}

bool PfDomHandler::endDocument() {
  return true;
}
