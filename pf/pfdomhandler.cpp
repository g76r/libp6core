/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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

#include "pfdomhandler.h"
#include "pfinternals_p.h"
#include <QObject>
#include <QtDebug>

#define tr(x) QObject::tr(x)

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

bool PfDomHandler::startNode(const QStringList &names) {
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

bool PfDomHandler::endNode(const QStringList &names) {
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
