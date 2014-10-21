/* Copyright 2012-2014 Hallowyn and others.
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
  if ((r = _content.writeXmlUsingBase64(target, options)) < 0)
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
    QList<QString> lines = _content.toString().split("\n");
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
    if ((r = target->write(PfUtils::escape(_name, true).toUtf8())) < 0)
      return -1;
    total += r;
    // subnodes & content
    if (options.shouldWriteContentBeforeSubnodes() && !_content.isArray()) {
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
      if ((r = target->write(INDENTATION_EOL_STRING)) < 0)
        return -1;
      total += r;
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
      if (!indent.isNull()) {
        if ((r = target->write(INDENTATION_EOL_STRING)) < 0)
          return -1;
        total += r;
      }
      if ((r = _children.at(i).d->internalWritePf(target, indent, options)) < 0)
        return -1;
      total += r;
    }
    if (!indent.isNull())
      indent.chop(sizeof INDENTATION_STRING - 1);
  }
  return total;
}

qint64 PfNodeData::internalWritePfContent(QIODevice *target, QString indent,
                                      PfOptions options) const {
  qint64 total = 0, r;
  if (_content.isArray()) {
    if ((r = target->write("\n")) < 0)
      return -1;
    total += r;
    // array content
    if ((r = _content.writePf(target, options)) < 0)
      return -1;
    total += r;
    if (!indent.isNull()) {
      if ((r = target->write(indent.toUtf8())) < 0)
        return -1;
      total += r;
    }
  } else if (!_content.isEmpty()){
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
    if ((r = _content.writePf(target, options)) < 0)
      return -1;
    total += r;
  }
  return total;
}

const QList<PfNode> PfNode::childrenByName(QString name) const {
  QList<PfNode> list;
  foreach (PfNode child, children())
    if (child.d->_name == name)
      list.append(child);
  return list;
}

bool PfNode::hasChild(QString name) const {
  foreach (PfNode child, children())
    if (child.d->_name == name)
      return true;
  return false;
}

PfNode PfNode::firstTextChildByName(QString name) const {
  foreach (PfNode child, children())
    if (child.d->_name == name && child.contentIsText())
        return child;
  return PfNode();
}

QStringList PfNode::stringChildrenByName(QString name) const {
  QStringList sl;
  foreach (PfNode child, children())
    if (child.d->_name == name && child.contentIsText())
        sl.append(child.contentAsString());
  return sl;
}

QList<QPair<QString,QString> > PfNode::stringsPairChildrenByName(
    QString name) const {
  QList<QPair<QString,QString> > l;
  foreach (PfNode child, children())
    if (child.d->_name == name && child.contentIsText()) {
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
  foreach (PfNode child, children())
    if (child.d->_name == name && child.contentIsText()) {
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
  qint64 v = d->_content.toString().trimmed().toLongLong(&myok, 0);
  if (ok)
    *ok = myok;
  return myok ? v : defaultValue;
}

double PfNode::contentAsDouble(double defaultValue, bool *ok) const {
  bool myok;
  double v = d->_content.toString().trimmed().toDouble(&myok);
  if (ok)
    *ok = myok;
  return myok ? v : defaultValue;
}

bool PfNode::contentAsBool(bool defaultValue, bool *ok) const {
  QString s = d->_content.toString().trimmed();
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
  QString v = d->_content.toString(), s;
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
  d->_children.append(PfNode(name, content));
}

void PfNode::setAttribute(QString name, QList<QString> content) {
  removeChildrenByName(name);
  PfNode child(name);
  child.setContent(content);
  appendChild(child);
}

void PfNode::setContent(QList<QString> strings) {
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
