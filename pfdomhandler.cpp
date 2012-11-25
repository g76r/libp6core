#include "pfdomhandler.h"
#include "pfinternals.h"

PfDomHandler::PfDomHandler() : PfHandler() {
}

PfDomHandler::~PfDomHandler() {
}

bool PfDomHandler::startDocument(const PfOptions &options) {
  Q_UNUSED(options)
  _roots.clear();
  _path.clear();
  return true;
}

bool PfDomHandler::startNode(const QList<QString> &names) {
  PfNode node(names.last());
  _path.append(node);
  return true;
}

bool PfDomHandler::text(const QString &text) {
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
                          const QString &surface) {
  if (_path.size() == 0) {
    setErrorString(tr("binary data before root node"));
    return false;
  }
  _path.last().appendContent(device, length, offset, surface);
  return true;
}

bool PfDomHandler::binary(const QByteArray &data, const QString &surface) {
  if (_path.size() == 0) {
    setErrorString(tr("binary data before root node"));
    return false;
  }
  _path.last().appendContent(data, surface);
  return true;
}

bool PfDomHandler::array(const PfArray &array) {
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

bool PfDomHandler::endNode(const QList<QString> &names) {
  Q_UNUSED(names);
  PfNode node(_path.takeLast());
  if (!_path.isEmpty())
    _path.last().appendChild(node);
  else
    _roots.append(node);
  return true;
}

bool PfDomHandler::comment(const QString &content) {
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
