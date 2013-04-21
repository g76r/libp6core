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

#include "pfnode.h"
#include "pfinternals.h"
#include <QtDebug>
#include <QAtomicInt>
#include <QStringList>

#define INDENTATION_STRING "  "
#define INDENTATION_STRING_LENGTH 2

static int staticInit() {
  qRegisterMetaType<PfNode>("PfNode");
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

static QRegExp whitespace("\\s");

qint64 PfNodeData::writePf(QIODevice *target, const PfOptions options) const {
  if (options.shouldIndent())
    return internalWritePf(target, "", options);
  else
    return internalWritePf(target, QString(), options);
}

qint64 PfNodeData::writeFlatXml(QIODevice *target,
                                const PfOptions options) const  {
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
                                   const PfOptions options) const {
  // LATER indent
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
    if ((r = target->write(pfescape(_name).toUtf8())) < 0)
      return -1;
    total += r;
    if (_content.isArray()) {
      if ((r = target->write("\n")) < 0)
        return -1;
      total += r;
    } else {
      if (indent.isNull()) {
        // no indentation but single space when content and no child
        if (_children.size() == 0 && !_content.isEmpty()) {
          if ((r = target->write(" ")) < 0)
            return -1;
          total += r;
        }
      } else {
        // indentation
        if (_children.size() > 0) {
          if ((r = target->write("\n")) < 0)
            return -1;
          total += r;
        } else if (!_content.isEmpty() && !_content.isArray()) {
          if ((r = target->write(" ")) < 0)
            return -1;
          total += r;
        }
      }
    }
    // subnodes
    if(_children.size()) {
      if (!indent.isNull())
        indent.append(INDENTATION_STRING);
      for (int i = 0; i < _children.size(); ++i) {
        if ((r = _children.at(i).d->internalWritePf(target, indent,
                                                    options)) < 0)
          return -1;
        total += r;
      }
      if (!indent.isNull())
        indent.chop(INDENTATION_STRING_LENGTH);
    }
    // content
    if (_content.isArray()) {
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
      // regular content
      if (!indent.isNull() && _children.size() > 0) {
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
    // closing parenthesis
    if (!indent.isNull() && _children.size()) {
      if ((r = target->write(indent.toUtf8())) < 0)
        return -1;
      total += r;
    }
    if ((r = target->write(")")) < 0)
      return -1;
    total += r;
    if (!indent.isNull()) {
      if ((r = target->write("\n")) < 0)
        return -1;
      total += r;
    }
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

QString PfNode::attribute(QString name, QString defaultValue) const {
  foreach (PfNode child, children())
    if (child.d->_name == name && child.contentIsText())
        return child.contentAsString();
  return defaultValue;
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
      QString s = child.contentAsString().trimmed();
      int i = s.indexOf(whitespace);
      if (i >= 0)
        l.append(QPair<QString,QString>(s.left(i), s.mid(i).trimmed()));
      else
        l.append(QPair<QString,QString>(s, QString()));
    }
  return l;
}

qint64 PfNode::longAttribute(QString name, qint64 defaultValue) const {
  bool ok;
  qint64 i = attribute(name).toLongLong(&ok, 0);
  return ok ? i : defaultValue;
}

double PfNode::doubleAttribute(QString name, double defaultValue) const {
  bool ok;
  double f = attribute(name).toDouble(&ok);
  return ok ? f : defaultValue;
}

void PfNode::setAttribute(QString name, QString content) {
  removeChildrenByName(name);
  d->_children.append(PfNode(name, content));
}

QByteArray PfNode::toPf(const PfOptions options) const {
  QByteArray ba;
  QBuffer b(&ba);
  b.open(QIODevice::WriteOnly);
  writePf(&b, options);
  return ba;
}

void PfNode::removeChildrenByName(const QString name) {
  for (int i = 0; i < d->_children.size(); ) {
    PfNode child = d->_children.at(i);
    if (child.name() == name)
      d->_children.removeAt(i);
    else
      ++i;
  }
}
