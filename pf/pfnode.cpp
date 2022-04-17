/* Copyright 2012-2022 Hallowyn and others.
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
#include "pfinternals_p.h"
#include <QtDebug>
#include <QAtomicInt>
#include <QStringList>
#include "pfparser.h"
#include "pfdomhandler.h"
#include <QRegularExpression>

#define INDENTATION_EOL_STRING "\n"
#define INDENTATION_STRING "  "

const QList<PfNode> PfNode::_emptyList;

static int staticInit() {
  qMetaTypeId<PfNode>();
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

static QRegularExpression _whitespace("\\s");
static QRegularExpression _leadingwhitespace("\\A\\s+");

qint64 PfNodeData::writePf(QIODevice *target, const PfOptions &options) const {
  if (options.shouldIndent())
    return internalWritePf(target, "", options);
  return internalWritePf(target, QString(), options);
}

qint64 PfNodeData::writeFlatXml(QIODevice *target,
                                const PfOptions &options) const  {
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
  for (auto child: _children) {
    if ((r = child.writeFlatXml(target, options)) < 0)
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

qint64 PfNodeData::internalWritePf(
    QIODevice *target, QString indent, const PfOptions &options) const {
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
    if (!indent.isNull() && !_children.isEmpty()) {
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

qint64 PfNodeData::internalWritePfSubNodes(
    QIODevice *target, QString indent, const PfOptions &options) const {
  qint64 total = 0, r;
  if(!_children.isEmpty()) {
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

qint64 PfNodeData::internalWritePfContent(
    QIODevice *target, const QString &indent, const PfOptions &options) const {
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
    if (options.shouldWriteContentBeforeSubnodes() || _children.isEmpty()) {
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

QList<PfNode> PfNode::childrenByName(const QString &name) const {
  QList<PfNode> list;
  if (!name.isEmpty())
    for (auto child: children())
      if (!child.isNull() && child.d->_name == name)
        list.append(child);
  return list;
}

QList<PfNode> PfNode::childrenByName(const QStringList &names) const {
  QList<PfNode> list;
  if (!names.isEmpty())
    for (auto child: children())
      if (!child.isNull() && names.contains(child.d->_name))
        list.append(child);
  return list;
}

QList<PfNode> PfNode::grandChildrenByChildrenName(const QString &name) const {
  QList<PfNode> list;
  if (!name.isEmpty())
    for (auto child: children())
      if (!child.isNull() && child.d->_name == name)
        list.append(child.children());
  return list;
}

QList<PfNode> PfNode::grandChildrenByChildrenName(
    const QStringList &names) const {
  QList<PfNode> list;
  if (!names.isEmpty())
    for (auto child: children())
      if (!child.isNull() && names.contains(child.d->_name))
        list.append(child.children());
  return list;
}

bool PfNode::hasChild(const QString &name) const {
  if (!name.isEmpty())
    for (auto child: children())
      if (!child.isNull() && child.d->_name == name)
        return true;
  return false;
}

PfNode PfNode::firstTextChildByName(const QString &name) const {
  if (!name.isEmpty())
    for (auto child: children())
      if (!child.isNull() && child.d->_name == name && child.isText())
        return child;
  return PfNode();
}

QStringList PfNode::stringChildrenByName(const QString &name) const {
  QStringList sl;
  if (!name.isEmpty())
    for (auto child: children())
      if (!child.isNull() && child.d->_name == name && child.isText())
        sl.append(child.contentAsString());
  return sl;
}

QList<QPair<QString,QString> > PfNode::stringsPairChildrenByName(
    const QString &name) const {
  QList<QPair<QString,QString> > l;
  if (!name.isEmpty())
    for (auto child: children())
      if (!child.isNull() && child.d->_name == name && child.isText()) {
        QString s = child.contentAsString().remove(_leadingwhitespace);
        qsizetype i = s.indexOf(_whitespace);
        if (i >= 0)
          l.append(QPair<QString,QString>(s.left(i), s.mid(i+1)));
        else
          l.append(QPair<QString,QString>(s, QString()));
      }
  return l;
}

QList<QPair<QString, qint64>> PfNode::stringLongPairChildrenByName(
    const QString &name) const {
  QList<QPair<QString,qint64>> l;
  if (!name.isEmpty())
    for (auto child: children())
      if (!child.isNull() && child.d->_name == name && child.isText()) {
        QString s = child.contentAsString().remove(_leadingwhitespace);
        qsizetype i = s.indexOf(_whitespace);
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
  auto s = contentAsString().trimmed();
  auto len = s.size();
  if (len >= 2 && s.at(1) == 'x') {
    switch(s.at(len-1).toLatin1()) {
      case 'k':
        s = s.left(len-1)+"000";
        break;
      case 'M':
      case 'm':
        s = s.left(len-1)+"000000";
        break;
      case 'G':
      case 'b':
        s = s.left(len-1)+"000000000";
        break;
      case 'T':
        s = s.left(len-1)+"000000000000";
        break;
      case 'P':
        s = s.left(len-1)+"000000000000000";
        break;
    }
  }
  qint64 v = s.toLongLong(&myok, 0);
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

static QRegularExpression _twoStringsListRegexp("\\A\\s*(\\S+)\\s*(.*)\\z");

QStringList PfNode::contentAsTwoStringsList() const {
  return _twoStringsListRegexp.match(contentAsString()).capturedTexts();
}

PfNode &PfNode::setAttribute(const QString &name, const QString &content) {
  removeChildrenByName(name);
  appendChild(PfNode(name, content));
  return *this;
}

PfNode &PfNode::setAttribute(const QString &name, const QStringList &content) {
  removeChildrenByName(name);
  PfNode child(name);
  child.setContent(content);
  appendChild(child);
  return *this;
}

PfNode &PfNode::setContent(const QStringList &strings) {
  QString v;
  foreach(QString s, strings) {
    s.replace('\\', "\\\\").replace(' ', "\\ ").replace('\t', "\\\t")
        .replace('\r', "\\\r").replace('\n', "\\\n");
    v.append(s).append(' ');
  }
  if (!v.isEmpty())
    v.chop(1);
  return setContent(v);
}

QByteArray PfNode::toPf(PfOptions options) const {
  QByteArray ba;
  QBuffer b(&ba);
  b.open(QIODevice::WriteOnly);
  writePf(&b, options);
  return ba;
}

PfNode &PfNode::removeChildrenByName(const QString &name) {
  if (d)
    for (int i = 0; i < d->_children.size(); ) {
      PfNode child = d->_children.at(i);
      if (child.name() == name)
        d->_children.removeAt(i);
      else
        ++i;
    }
  return *this;
}

PfNode PfNode::fromPf(QByteArray source, PfOptions options) {
  PfDomHandler h;
  PfParser p(&h);
  if (p.parse(source, options)) {
    if (!h.roots().isEmpty())
      return h.roots().at(0);
  }
  return PfNode();
}

qint64 PfNodeData::writePfContent(
    QIODevice *target, const PfOptions &options) const {
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
    }
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

qint64 PfNodeData::writeRawContent(
    QIODevice *target, const PfOptions &options) const {
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
    QIODevice *target, const PfOptions &options) const {
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
    }
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
