/* Copyright 2012-2015 Hallowyn and others.
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

#include "pfnode.h"
#include "pfinternals.h"
#include <QtDebug>
#include <QAtomicInt>
#include <QStringList>
#include "pfparser.h"
#include "pfdomhandler.h"
#include <QRegExp>

#define INDENTATION_EOL_STRING "\n"
#define INDENTATION_STRING "  "

static int staticInit() {
  qRegisterMetaType<PfNode>("PfNode");
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

static QRegExp whitespace("\\s");
static QRegExp leadingwhitespace("^\\s+");

qint64 PfNodeData::writePf(QIODevice *target, PfOptions options) const {
  if (options.shouldIndent())
    return internalWritePf(target, "", options);
  else
    return internalWritePf(target, QString(), options);
}

qint64 PfNodeData::writeFlatXml(QIODevice *target,
                                PfOptions options) const  {
  // may indent one day (however xmllint does that well)
  qint64 total = 0, r;
  // opening tag
  if (isComment()) {
    if (options.shouldIgnoreComment())
      return 0;
    if ((r = target->write("<!--")) < 0)
      return -1;
    total += r;
  } else {
    if ((r = target->write("<")) < 0)
      return -1;
    total += r;
    if ((r = target->write(pftoxmlname(_name).toUtf8())) < 0)
      return -1;
    total += r;
    if ((r = target->write(">")) < 0)
      return -1;
    total += r;
  }
  // subnodes
  //buildChildrenFromArray();
  for (int i = 0; i < _children.size(); ++i) {
    if ((r = _children.at(i).writeFlatXml(target, options)) < 0)
      return -1;
    total += r;
  }
  // content
  if ((r = writeXmlUsingBase64Content(target, options)) < 0)
    return -1;
  total += r;
  // closing tag
  if (isComment()) {
    if ((r = target->write("-->")) < 0)
      return -1;
    total += r;
  } else {
    if ((r = target->write("</")) < 0)
      return -1;
    total += r;
    if ((r = target->write(pftoxmlname(_name).toUtf8())) < 0)
      return -1;
    total += r;
    if ((r = target->write(">")) < 0)
      return -1;
    total += r;
  }
  return total;
}

/*qint64 PfNodeData::writeCompatibleXml(QIODevice &target) const  {
  // LATER indent
  qint64 total = 0, r;
  // opening tag
  if ((r = target.write("<")) < 0)
    return -1;
  total += r;
  if ((r = target.write(pftoxmlname(_name).toUtf8())) < 0) // fixme xmlnameescape
    return -1;
  total += r;
  // attributes
  // LATER maybe create an "attributes" method
  foreach (PfNode *child, _children) {
    int n = 0;
    if (child->children().size() != 0 ||
        child->content().containsBinaryData() ||
        child->content().size() >= 256)
      goto not_an_attribute;
    foreach (PfNode *other, _children) {
      if(child->name() == other->name()) {
        ++n;
        if (n > 1)
          goto not_an_attribute;
      }
    }
    if ((r = target.write(" ")) < 0)
      return -1;
    total += r;
    if ((r = target.write(pftoxmlname(child->name()).toUtf8()) < 0))
      return -1;
    total += r;
    if ((r = target.write("=\"")) < 0)
      return -1;
    total += r;
    if ((r = target.write(pftoxmltext(child->name()).toUtf8())) < 0)
      return -1;
    total += r;
    if ((r = target.write("\"")) < 0)
      return -1;
    total += r;
    not_an_attribute:;
  }
  if ((r = target.write(">")) < 0)
    return -1;
  total += r;
  // subnodes
  for (int i = 0; i < _children.size(); ++i) {
    if ((r = _children.at(i)->writeCompatibleXml(target)) < 0)
      return -1;
    total += r;
  }
  // content
  if ((r = _content.writeXmlUsingBase64(target)) < 0)
    return -1;
  total += r;
  // closing tag
  if ((r = target.write("</")) < 0)
    return -1;
  total += r;
  if ((r = target.write(pftoxmlname(_name).toUtf8())) < 0)
    return -1;
  total += r;
  if ((r = target.write(">")) < 0)
    return -1;
  total += r;
  return total;
}*/

qint64 PfNodeData::internalWritePf(QIODevice *target, QString indent,
                                   PfOptions options) const {
  qint64 total = 0, r;
  if (isComment()) {
    // comment node
    if (options.shouldIgnoreComment())
      return 0;
    // must split content on \n because whereas it is not allowed in the on-disk
    // format, it can be added through the API
    QStringList lines = contentAsString().split("\n");
    foreach (const QString line, lines) {
      if (!indent.isNull()) {
        if ((r = target->write(indent.toUtf8())) < 0)
          return -1;
        total += r;
      }
      if ((r = target->write("#")) < 0)
        return -1;
      total += r;
      if ((r = target->write(line.toUtf8().constData())) < 0)
        return -1;
      total += r;
      if ((r = target->write("\n")) < 0)
        return -1;
      total += r;
    }
  } else {
    // regular node
    // opening parenthesis and node name
    if (!indent.isNull()) {
      if ((r = target->write(indent.toUtf8())) < 0)
        return -1;
      total += r;
    }
    if ((r = target->write("(")) < 0)
      return -1;
    total += r;
    if ((r = target->write(PfUtils::escape(_name, options, true).toUtf8())) < 0)
      return -1;
    total += r;
    // subnodes & content
    if (options.shouldWriteContentBeforeSubnodes() && !isArray()) {
      if ((r = internalWritePfContent(target, indent, options)) < 0)
        return -1;
      total += r;
      if ((r = internalWritePfSubNodes(target, indent, options)) < 0)
        return -1;
      total += r;
    } else {
      if ((r = internalWritePfSubNodes(target, indent, options)) < 0)
        return -1;
      total += r;
      if ((r = internalWritePfContent(target, indent, options)) < 0)
        return -1;
      total += r;
    }
    // closing parenthesis
    if (!indent.isNull() && _children.size()) {
      if (!_children.last().isComment()) {
        if ((r = target->write(INDENTATION_EOL_STRING)) < 0)
          return -1;
        total += r;
      }
      if ((r = target->write(indent.toUtf8())) < 0)
        return -1;
      total += r;
    }
    if ((r = target->write(")")) < 0)
      return -1;
    total += r;
    // end of line at end of toplevel node
    if (!indent.isNull() && indent.isEmpty()) {
      if ((r = target->write(INDENTATION_EOL_STRING)) < 0)
        return -1;
      total += r;
    }
  }
  return total;
}

qint64 PfNodeData::internalWritePfSubNodes(QIODevice *target, QString indent,
                                       PfOptions options) const {
  qint64 total = 0, r;
  if(_children.size()) {
    if (!indent.isNull())
      indent.append(INDENTATION_STRING);
    for (int i = 0; i < _children.size(); ++i) {
      if (!indent.isNull() && (i == 0 || !_children[i-1].isComment())) {
        if ((r = target->write(INDENTATION_EOL_STRING)) < 0)
          return -1;
        total += r;
      }
      const PfNode &child = _children[i];
      if (!child.isNull()) {
        if ((r = child.d->internalWritePf(target, indent, options)) < 0)
          return -1;
        total += r;
      }
    }
    if (!indent.isNull())
      indent.chop(sizeof INDENTATION_STRING - 1);
  }
  return total;
}

qint64 PfNodeData::internalWritePfContent(QIODevice *target, QString indent,
                                      PfOptions options) const {
  qint64 total = 0, r;
  if (isArray()) {
    if ((r = target->write("\n")) < 0)
      return -1;
    total += r;
    // array content
    if ((r = writePfContent(target, options)) < 0)
      return -1;
    total += r;
    if (!indent.isNull()) {
      if ((r = target->write(indent.toUtf8())) < 0)
        return -1;
      total += r;
    }
  } else if (!isEmpty()){
    // text or binary content
    if (options.shouldWriteContentBeforeSubnodes() || _children.size() == 0) {
      if ((r = target->write(" ")) < 0)
        return -1;
      total += r;
    } else if (!indent.isNull()) {
      if ((r = target->write(INDENTATION_EOL_STRING)) < 0)
        return -1;
      total += r;
      if ((r = target->write(indent.toUtf8())) < 0)
        return -1;
      total += r;
      if ((r = target->write(INDENTATION_STRING)) < 0)
        return -1;
      total += r;
    }
    if ((r = writePfContent(target, options)) < 0)
      return -1;
    total += r;
  }
  return total;
}

const QList<PfNode> PfNode::childrenByName(QString name) const {
  QList<PfNode> list;
  if (!name.isEmpty())
    foreach (PfNode child, children())
      if (!child.isNull() && child.d->_name == name)
        list.append(child);
  return list;
}

bool PfNode::hasChild(QString name) const {
  if (!name.isEmpty())
    foreach (PfNode child, children())
      if (!child.isNull() && child.d->_name == name)
        return true;
  return false;
}

PfNode PfNode::firstTextChildByName(QString name) const {
  if (!name.isEmpty())
    foreach (PfNode child, children())
      if (!child.isNull() && child.d->_name == name && child.isText())
        return child;
  return PfNode();
}

QStringList PfNode::stringChildrenByName(QString name) const {
  QStringList sl;
  if (!name.isEmpty())
    foreach (PfNode child, children())
      if (!child.isNull() && child.d->_name == name && child.isText())
        sl.append(child.contentAsString());
  return sl;
}

QList<QPair<QString,QString> > PfNode::stringsPairChildrenByName(
    QString name) const {
  QList<QPair<QString,QString> > l;
  if (!name.isEmpty())
    foreach (PfNode child, children())
      if (!child.isNull() && child.d->_name == name && child.isText()) {
        QString s = child.contentAsString().remove(leadingwhitespace);
        int i = s.indexOf(whitespace);
        if (i >= 0)
          l.append(QPair<QString,QString>(s.left(i), s.mid(i+1)));
        else
          l.append(QPair<QString,QString>(s, QString()));
      }
  return l;
}

QList<QPair<QString, qint64> > PfNode::stringLongPairChildrenByName(
    QString name) const {
  QList<QPair<QString,qint64> > l;
  if (!name.isEmpty())
    foreach (PfNode child, children())
      if (!child.isNull() && child.d->_name == name && child.isText()) {
        QString s = child.contentAsString().remove(leadingwhitespace);
        int i = s.indexOf(whitespace);
        if (i >= 0)
          l.append(QPair<QString,qint64>(s.left(i),
                                         s.mid(i).trimmed().toLongLong(0, 0)));
        else
          l.append(QPair<QString,qint64>(s, 0));
      }
  return l;
}

qint64 PfNode::contentAsLong(qint64 defaultValue, bool *ok) const {
  bool myok;
  qint64 v = contentAsString().trimmed().toLongLong(&myok, 0);
  if (ok)
    *ok = myok;
  return myok ? v : defaultValue;
}

double PfNode::contentAsDouble(double defaultValue, bool *ok) const {
  bool myok;
  double v = contentAsString().trimmed().toDouble(&myok);
  if (ok)
    *ok = myok;
  return myok ? v : defaultValue;
}

bool PfNode::contentAsBool(bool defaultValue, bool *ok) const {
  QString s = contentAsString().trimmed();
  bool myok = true, v;
  if (s.compare("true", Qt::CaseInsensitive))
    v = true;
  else if (s.compare("false", Qt::CaseInsensitive))
    v = false;
  else
    v = s.toLongLong(&myok, 0) != 0;
  if (ok)
    *ok = myok;
  return myok ? v : defaultValue;
}

QStringList PfNode::contentAsStringList() const {
  QString v = contentAsString(), s;
  QStringList l;
  for (int i = 0; i < v.size(); ++i) {
    const QChar &c = v[i];
    if (c == '\\') {
      if (++i < v.size())
        s.append(v[i]);
    } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      if (!s.isNull())
        l.append(s);
      s = QString();
    } else
      s.append(c);
  }
  if (!s.isEmpty())
    l.append(s);
  return l;
}

void PfNode::setAttribute(QString name, QString content) {
  removeChildrenByName(name);
  appendChild(PfNode(name, content));
}

void PfNode::setAttribute(QString name, QStringList content) {
  removeChildrenByName(name);
  PfNode child(name);
  child.setContent(content);
  appendChild(child);
}

void PfNode::setContent(QStringList strings) {
  QString v;
  foreach(QString s, strings) {
    s.replace('\\', "\\\\").replace(' ', "\\ ");
    v.append(s).append(' ');
  }
  if (!v.isEmpty())
    v.chop(1);
  setContent(v);
}

QByteArray PfNode::toPf(PfOptions options) const {
  QByteArray ba;
  QBuffer b(&ba);
  b.open(QIODevice::WriteOnly);
  writePf(&b, options);
  return ba;
}

void PfNode::removeChildrenByName(QString name) {
  if (d)
    for (int i = 0; i < d->_children.size(); ) {
      PfNode child = d->_children.at(i);
      if (child.name() == name)
        d->_children.removeAt(i);
      else
        ++i;
    }
}

PfNode PfNode::fromPf(QByteArray source, PfOptions options) {
  PfDomHandler h;
  PfParser p(&h);
  if (p.parse(source, options)) {
    if (h.roots().size() > 0)
      return h.roots().at(0);
  }
  return PfNode();
}

qint64 PfNodeData::writePfContent(QIODevice *target, PfOptions options) const {
  if (isArray()) {
    if (options.shouldTranslateArrayIntoTree()) {
      PfNode tmp;
      _array.convertToChildrenTree(&tmp);
      qint64 total = 0, r;
      foreach (PfNode child, tmp.children()) {
        r = child.writePf(target, options);
        if (r == -1)
          return -1;
        total += r;
      }
      return total;
    } else
      return _array.writePf(target, options);
  }
  qint64 total = 0, r;
  foreach (const PfFragment f, _fragments) {
    r = f.writePf(target, options);
    if (r < 0)
      return -1;
    total += r;
  }
  return total;
}

qint64 PfNodeData::writeRawContent(QIODevice *target, PfOptions options) const {
  if (isArray())
    return _array.writePf(target, options);
  qint64 total = 0, r;
  foreach (const PfFragment f, _fragments) {
    r = f.writeRaw(target, options);
    if (r < 0)
      return -1;
    total += r;
  }
  return total;
}

qint64 PfNodeData::writeXmlUsingBase64Content(
    QIODevice *target, PfOptions options) const {
  if (isArray()) {
    if (options.shouldTranslateArrayIntoTree()) {
      PfNode tmp;
      _array.convertToChildrenTree(&tmp);
      qint64 total = 0, r;
      foreach (PfNode child, tmp.children()) {
        r = child.writeFlatXml(target, options);
        if (r == -1)
          return -1;
        total += r;
      }
      return total;
    } else
      return _array.writeTrTd(target, true, options);
  }
  qint64 total = 0, r;
  foreach (const PfFragment f, _fragments) {
    r = f.writeXmlUsingBase64(target, options);
    if (r < 0)
      return -1;
    total += r;
  }
  return total;
}

QByteArray PfNodeData::contentAsByteArray() const {
  QBuffer buf;
  buf.open(QIODevice::WriteOnly);
  writeRawContent(&buf, PfOptions());
  return buf.data();
}

QString PfNodeData::PfAbstractBinaryFragmentData::takeFirstLayer(
    QString &surface) const {
  QRegExp head("^([^:]*)(:|$).*");
  if (!head.exactMatch(surface))
    return surface = QString();
  QString first = head.cap(1);
  surface.remove(0, first.size()+1);
  return first;
}

bool PfNodeData::PfAbstractBinaryFragmentData::removeSurface(
    QByteArray &data, QString surface) const {
  QString layer = takeFirstLayer(surface);
  if (layer.isEmpty() || layer == "null") {
    // do nothing
  } else if (layer == "zlib") {
    QByteArray a = qUncompress((const uchar *)data.constData()+4,
                               data.size()-4);
    if (a.isNull()) {
      qWarning("PF: cannot remove zlib surface");
      return false;
    }
    data = a;
  } else if (layer == "hex") {
    QByteArray a = QByteArray::fromHex(data);
    if (a.isNull()) {
      qWarning("PF: cannot remove hex surface");
      return false;
    }
    data = a;
  } else if (layer == "base64") {
    QByteArray a = QByteArray::fromBase64(data);
    if (a.isNull()) {
      qWarning("PF: cannot remove base64 surface");
      return false;
    }
    data = a;
  } else {
    qWarning() << "PF: cannot remove unknown surface" << layer;
    return false;
  }
  if (!surface.isEmpty())
    return removeSurface(data, surface);
  return true;
}

bool PfNodeData::PfAbstractBinaryFragmentData::applySurface(
    QByteArray &data, QString surface) const {
  QString layer = takeFirstLayer(surface);
  if (!surface.isEmpty())
    if (!applySurface(data, surface))
      return false;
  if (layer.isEmpty() || layer == "null") {
    // do nothing
  } else if (layer == "zlib") {
    QByteArray a = qCompress(data);
    if (a.isNull()) {
      qWarning("PF: cannot apply zlib surface");
      return false;
    }
    data = QByteArray("\0\0\0\0", 4).append(a);
  } else if (layer == "hex") {
    data = data.toHex();
  } else if (layer == "base64") {
    data = data.toBase64();
  } else {
    qWarning() << "PF: cannot apply unknown surface" << layer;
    return false;
  }
  return true;
}

qint64 PfNodeData::PfAbstractBinaryFragmentData::measureSurface(
    QByteArray data, QString surface) const {
  QString layer = takeFirstLayer(surface);
  if (layer.isEmpty() || layer == "null") {
    // do nothing
  } else if (layer == "zlib") {
    if (surface.isEmpty()) {
      if (data.size() > 8) {
        qint64 l;
        l = (quint64)data.at(0)
            + ((quint64)data.at(1)<<8)
            + ((quint64)data.at(2)<<16)
            + ((quint64)data.at(3)<<24)
            + ((quint64)data.at(4)<<32)
            + ((quint64)data.at(5)<<40)
            + ((quint64)data.at(6)<<48)
            + ((quint64)data.at(7)<<56);
        return l < 0 ? 0 : l;
      } else {
        qWarning("PF: cannot measure zlib surface");
        return 0;
      }
    } else
      return measureSurface(qUncompress((const uchar *)data.constData()+4,
                                        data.size()-4), surface);
  } else if (layer == "hex") {
    if (surface.isEmpty())
      return data.size()/2;
    else
      return measureSurface(QByteArray::fromHex(data), surface);
  } else if (layer == "base64") {
    if (surface.isEmpty()) {
      qint64 l = data.size()*3/4;
      if (data.at(data.size()-2) == '=') {
        --l;
        if (data.at(data.size()-3) == '=')
          --l;
      }
      return l;
    } else
      return measureSurface(QByteArray::fromBase64(data), surface);
  } else {
    qWarning() << "PF: cannot measure unknown surface" << layer;
  }
  return true;
}

void PfNodeData::PfBinaryFragmentData::setSurface(
    QString surface, bool shouldAdjustSize) {
  _surface = PfOptions::normalizeSurface(surface);
  if (shouldAdjustSize && !_surface.isNull()) {
    QByteArray data = _data;
    _size = measureSurface(data, _surface);
  }
}

void PfNodeData::PfLazyBinaryFragmentData::setSurface(
    QString surface, bool shouldAdjustSize) {
  _surface = PfOptions::normalizeSurface(surface);
  if (shouldAdjustSize && !_surface.isNull()) {
    qint64 pos = _device->pos();
    QByteArray data;
    if (!_device->seek(_offset))
      goto error;
    // LATER avoid loading in memory the full data at a whole
    data = _device->read(_length);
    if (_length != data.size()) {
error:
      qDebug() << "PfFragment::setSurface error (lazy-loaded binary fragment)"
               << _device->errorString();
      _device->seek(pos);
      _size = 0;
      return;
    }
    _device->seek(pos);
    _size = measureSurface(data, _surface);
  }
}

#include <stdlib.h>
PfNodeData::PfFragmentData::~PfFragmentData() {
  /*if (random() % 7824 == 0) {
    char *ptr = 0;
    *ptr = '0';
  }*/
}

bool PfNodeData::PfFragmentData::isText() const {
  return false;
}

QString PfNodeData::PfFragmentData::text() const {
  return QString();
}

bool PfNodeData::PfFragmentData::isEmpty() const {
  return false;
}

bool PfNodeData::PfFragmentData::isBinary() const {
  return false;
}

bool PfNodeData::PfFragmentData::isLazyBinary() const {
  return false;
}

qint64 PfNodeData::PfTextFragmentData::write(
    QIODevice *target, Format format, PfOptions options) const {
  switch (format) {
  case Raw:
    return target->write(_text.toUtf8());
  case Pf:
    return target->write(PfUtils::escape(_text, options).toUtf8());
  case XmlBase64:
    return target->write(pftoxmltext(_text).toUtf8());
  }
  return -1;
}

bool PfNodeData::PfTextFragmentData::isText() const {
  return true;
}

QString PfNodeData::PfTextFragmentData::text() const {
  return _text;
}

bool PfNodeData::PfAbstractBinaryFragmentData::isBinary() const {
  return true;
}

qint64 PfNodeData::PfBinaryFragmentData::write(
    QIODevice *target, Format format, PfOptions options) const {
  return writeDataApplyingSurface(target, format, options, _data);
}

qint64 PfNodeData::PfLazyBinaryFragmentData::write(
    QIODevice *target, Format format, PfOptions options) const {
  qint64 total = 0, pos = 0;
  if (!_device || !target)
    goto error;
  pos = _device->pos();
  if (!_surface.isEmpty()) {
    // surface cannot (yet?) be applyed to lazy-loaded fragments, therefore they
    // must be loaded in memory now
    QByteArray data;
    if (!_device->seek(_offset))
      goto error;
    data = _device->read(_length);
    if (_length != data.size())
      goto error;
    _device->seek(pos);
    return writeDataApplyingSurface(target, format, options, data);
  } else {
    // unsurfaced lazy-loaded binary
    if (!_device)
      return -1;
    qint64 r, remaining = _length;
    if (!_device->seek(_offset))
      goto error;
    if (format == Pf) {
      QString header = QString("|%1|%2\n").arg(_surface).arg(_length);
      if ((r = target->write(header.toUtf8())) < 0)
        goto error;
      total += r;
    }
    char buf[65536];
    while (remaining) {
      r = _device->read(buf, std::min(remaining, qint64(sizeof(buf))));
      if (r < 0)
        goto error;
      if (format == XmlBase64) {
        r = target->write(QByteArray(buf, r).toBase64());
      } else
        r = target->write(buf, r);
      if (r < 0)
        goto error;
      total += r;
      remaining -= r;
    }
    if (!_device->seek(pos))
      goto error;
  }
  return total;

error:
  qDebug() << "PfFragment::write() error: target device error:"
           << (target ? target->errorString() : "device is null")
           << " read (lazy) device error: "
           << (_device ? _device->errorString() : "device is null")
           << " bytes read so far: " << total;
  if (_device)
    _device->seek(pos);
  return -1;
}

bool PfNodeData::PfLazyBinaryFragmentData::isLazyBinary() const {
  return true;
}

qint64 PfNodeData::PfAbstractBinaryFragmentData::writeDataApplyingSurface(
    QIODevice *target, Format format, PfOptions options,
    QByteArray data) const {
  QString outputSurface = options.outputSurface();
  if (outputSurface.isNull() && format == Pf)
    outputSurface = _surface; // for PF, default output surface is original one
  // handling surfaces
  if ((!_surface.isNull() || !outputSurface.isNull())
      && _surface != outputSurface) {
    // decoding input surface, i.e. surface of in-memory or lazy-loaded document
    if (!removeSurface(data, _surface))
      return -1;
    // encoding output surface, i.e. surface of document being written
    if (!applySurface(data, outputSurface))
      return -1;
  }

  // writing data
  qint64 total = 0, r;
  if (format == Pf) {
    QString header = QString("|%1|%2\n").arg(outputSurface).arg(data.size());
    if ((r = target->write(header.toUtf8())) < 0)
      goto error;
    total += r;
  }
  if (format == XmlBase64) {
    QByteArray ba(data.toBase64());
    r = target->write(ba);
    if (r != ba.size())
      goto error;
  } else {
    r = target->write(data);
    if (r != data.size())
      goto error;
  }
  total += r;
  return total;

error:
  qDebug() << "PfFragment::write() error: target device error:"
           << (target ? target->errorString() : "device is null")
           << " bytes read so far: " << total << "/" << data.size();
  return -1;
}
