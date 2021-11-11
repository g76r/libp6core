/* Copyright 2012-2021 Hallowyn and others.
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
#include "pfinternals_p.h"
#include <QVector>

PfDomHandler::PfDomHandler() {
}

PfDomHandler::~PfDomHandler() {
}

bool PfDomHandler::startDocument(const PfOptions &options) {
  Q_UNUSED(options)
  _roots.clear();
  _path.clear();
  return true;
}

bool PfDomHandler::startNode(const QVector<QString> &names) {
  PfNode node(names.last());
  _path.append(node);
  return true;
}

bool PfDomHandler::text(const QString &text) {
  if (_path.isEmpty()) {
    setErrorString(tr("text data before root node"));
    return false;
  }
  _path.last().appendContent(text);
  return true;
}

bool PfDomHandler::binary(QIODevice *device, qint64 length, qint64 offset,
                          const QString &surface) {
  if (_path.isEmpty()) {
    setErrorString(tr("binary data before root node"));
    return false;
  }
  _path.last().appendContent(device, length, offset, surface);
  return true;
}

bool PfDomHandler::binary(const QByteArray &data, const QString &surface) {
  if (_path.isEmpty()) {
    setErrorString(tr("binary data before root node"));
    return false;
  }
  _path.last().appendContent(data, surface);
  return true;
}

bool PfDomHandler::array(const PfArray &array) {
  if (_path.isEmpty()) {
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

bool PfDomHandler::endNode(const QVector<QString> &names) {
  Q_UNUSED(names);
  PfNode node(_path.takeLast());
  if (!_path.isEmpty())
    _path.last().appendChild(node);
  else
    _roots.append(node);
  return true;
}

bool PfDomHandler::comment(const QString &content) {
  PfNode node = PfNode::createCommentNode(content);
  if (_path.isEmpty())
    _roots.append(node);
  else
    _path.last().appendChild(node);
  return true;
}

bool PfDomHandler::endDocument() {
  return true;
}
