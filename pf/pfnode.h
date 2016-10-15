/* Copyright 2012-2016 Hallowyn and others.
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

#ifndef PFNODE_H
#define PFNODE_H

#include "pffragment_p.h"
#include "pfarray.h"
#include <QString>
#include <QList>
#include <QVariant>
#include <QStringList>
#include <QBuffer>
#include <QVector>

class PfNode;

class LIBQTPFSHARED_EXPORT PfNodeData : public QSharedData {
  friend class PfNode;

  QString _name;
  QList<PfNode> _children;
  bool _isComment;
  QList<PfFragment> _fragments;
  PfArray _array;

public:
  explicit PfNodeData(QString name = QString()) : _name(name),
    _isComment(false) { }

private:
  PfNodeData(QString name, QString content, bool isComment)
    : _name(name), _isComment(isComment) {
    if (!content.isEmpty())
      _fragments.append(PfFragment(content));
  }
  PfNodeData(QString name, PfArray array)
    : _name(name), _isComment(false) {
    _array = array;
  }
  bool isComment() const { return _isComment; }
  bool isEmpty() const {
    return !_fragments.size() && _array.isNull(); }
  bool isArray() const { return !_array.isNull(); }
  bool isText() const {
    return !isArray() && !isBinary() && !isComment(); }
  bool isBinary() const {
    foreach (const PfFragment &f, _fragments)
      if (f.isBinary())
        return true;
    return false;
  }
  QString contentAsString() const {
    if (isArray())
      return QString();
    QString s("");
    foreach (const PfFragment &f, _fragments) {
      if (f.isBinary())
        return QString();
      s.append(f.text());
    }
    return s;
  }
  qint64 writePf(QIODevice *target, PfOptions options) const;
  qint64 writeFlatXml(QIODevice *target, PfOptions options) const;
  //qint64 writeCompatibleXml(QIODevice &target) const;
  //inline void buildChildrenFromArray() const;
  inline qint64 internalWritePf(QIODevice *target, QString indent,
                                PfOptions options) const;
  inline qint64 internalWritePfSubNodes(QIODevice *target, QString indent,
                                        PfOptions options) const;
  inline qint64 internalWritePfContent(QIODevice *target, QString indent,
                                       PfOptions options) const;
  /** Provide the content as a byte array.
    * If there are lazy-loaded binary fragments, they are loaded into memory,
    * in the returned QByteArray but do not keep them cached inside PfContent
    * structures, therefore the memory will be freed when the QByteArray is
    * discarded and if toByteArray() is called again, the data will be
    * loaded again. */
  QByteArray contentAsByteArray() const;
  /** Write content to target device in PF format (with escape sequences and
    * binary headers). */
  qint64 writePfContent(QIODevice *target, PfOptions options) const;
  /** Write content to target device in raw data format (no PF escape sequences
    * but actual content). */
  qint64 writeRawContent(QIODevice *target, PfOptions options) const;
  /** Write content to target device in XML format, embeding binary fragments
    * using base64 encoding. */
  qint64 writeXmlUsingBase64Content(QIODevice *target, PfOptions options) const;

};

class LIBQTPFSHARED_EXPORT PfNode {
  friend class PfNodeData;
  QSharedDataPointer<PfNodeData> d;
  static const QList<PfNode> _emptyList;

  PfNode(PfNodeData *data) : d(data) { }

public:
  /** Create a null node. */
  PfNode() { }
  PfNode(const PfNode &other) : d(other.d) { }
  /** If name is empty, the node will be null. */
  explicit PfNode(QString name)
      : d(name.isEmpty() ? 0 : new PfNodeData(name)) { }
  /** If name is empty, the node will be null. */
  PfNode(QString name, QString content)
    : d(name.isEmpty() ? 0 : new PfNodeData(name, content, false)) { }
  /** If name is empty, the node will be null. */
  PfNode(QString name, qint64 content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(QString name, quint64 content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(QString name, qint32 content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(QString name, quint32 content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(QString name, qint16 content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(QString name, quint16 content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(QString name, double content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(QString name, float content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(QString name, PfArray array)
    : d(name.isEmpty() ? 0 : new PfNodeData(name, array)) { }
  /** If name is empty, the node will be null (and children ignored). */
  PfNode(QString name, std::initializer_list<PfNode> children)
      : d(name.isEmpty() ? 0 : new PfNodeData(name)) {
    if (!name.isEmpty())
      appendChildren(children);
  }
  /** If name is empty, the node will be null (and children ignored). */
  PfNode(QString name, QString content, std::initializer_list<PfNode> children)
    : d(name.isEmpty() ? 0 : new PfNodeData(name, content, false)) {
    if (!name.isEmpty())
      appendChildren(children);
  }
  /** If name is empty, the node will be null (and children ignored). */
  PfNode(QString name, PfArray array, std::initializer_list<PfNode> children)
    : d(name.isEmpty() ? 0 : new PfNodeData(name, array)) {
    if (!name.isEmpty())
      appendChildren(children);
  }
  /** Create a comment node. */
  static PfNode createCommentNode(QString comment) {
    return PfNode(new PfNodeData(QStringLiteral("comment"), comment, true)); }
  PfNode &operator=(const PfNode &other) { d = other.d; return *this; }
  /** Build a PfNode from PF external format.
   * @return first encountered root node or PfNode() */
  static PfNode fromPf(QByteArray source, PfOptions options = PfOptions());

  // Node related methods /////////////////////////////////////////////////////

  /** A node has an empty string name if and only if the node is null. */
  QString name() const { return d ? d->_name : QString(); }
  /** Replace node name. If name is empty, the node will become null. */
  PfNode &setName(QString name) {
    if (name.isEmpty())
      d = 0;
    else {
      if (!d)
        d = new PfNodeData();
      d->_name = name;
    }
    return *this;
  }
  bool isNull() const { return !d; }
  bool isComment() const { return d && d->isComment(); }

  // Children related methods /////////////////////////////////////////////////

  const QList<PfNode> children() const {
    return d ? d->_children : _emptyList; }
  /** prepend a child to existing children (do nothing if child.isNull()) */
  inline PfNode &prependChild(PfNode child);
  /** append a child to existing children (do nothing if child.isNull()) */
  inline PfNode &appendChild(PfNode child);
  PfNode &appendChildren(std::initializer_list<PfNode> children) {
    for (const PfNode &child : children)
      appendChild(child);
    return *this; }
  PfNode &appendChildren(QList<PfNode> children) {
    for (const PfNode &child : children)
      appendChild(child);
    return *this; }
  PfNode &appendChildren(QVector<PfNode> children) {
    for (const PfNode &child : children)
      appendChild(child);
    return *this; }
  PfNode &prependCommentChild(QString comment) {
    return prependChild(createCommentNode(comment)); }
  PfNode &appendCommentChild(QString comment) {
    return appendChild(createCommentNode(comment)); }
  /** @return first text child by name
   * Most of the time one will use attribute() and xxxAttribute() methods rather
   * than directly calling firstTextChildByName(). */
  PfNode firstTextChildByName(QString name) const;
  /** Return a child content knowing the child name.
    * QString() if no text child exists.
    * QString("") if child exists but has no content
    * If several children have the same name the first text one is choosen.
    * The goal is to emulate XML attributes, hence the name. */
  QString attribute(QString name) const {
    PfNode child = firstTextChildByName(name);
    return child.isNull() ? QString() : child.contentAsString(); }
  /** Return a child content knowing the child name.
    * defaultValue if no text child exists.
    * QString("") if child exists but has no content
    * If several children have the same name the first text one is choosen.
    * The goal is to emulate XML attributes, hence the name. */
  QString attribute(QString name, QString defaultValue) const {
    PfNode child = firstTextChildByName(name);
    return child.isNull() ? defaultValue : child.contentAsString(); }
  /** Return the content (as string) of every child with a given name.
   * This is the same as attribute() with multi-valued semantics.
   * Skip children with non-text content.
   * If no text child matches the name, the list is empty. */
  QStringList stringChildrenByName(QString name) const;
  /** Return the string content of children, splited into string pairs at the
   * first whitespace, one list item per child.
   * Child whole content and both strings of the pair are trimmed.
   * Skip children with non-text content.
   * Chilren without whitespace will have the first pair item set to the whole
   * node content (which may be empty) and the second one to QString().
   * If no text child matches the name, the list is empty. */
  QList<QPair<QString,QString> > stringsPairChildrenByName(QString name) const;
  /** Return the integer content of children, splited into pairs at the
   * first whitespace, one list item per child.
   * @see stringsPairChildrenByName() */
  QList<QPair<QString, qint64> > stringLongPairChildrenByName(
      QString name) const;
  /** @see contentAsLong() */
  qint64 longAttribute(QString name, qint64 defaultValue = 0,
                       bool *ok = 0) const {
    return firstTextChildByName(name).contentAsLong(defaultValue, ok); }
  /** @see contentAsDouble() */
  double doubleAttribute(QString name, double defaultValue,
                         bool *ok = 0) const {
    return firstTextChildByName(name).contentAsDouble(defaultValue, ok); }
  // LATER contentAsDateTime()
  /** @see contentAsBool() */
  bool boolAttribute(QString name, bool defaultValue = false,
                     bool *ok = 0) const {
    return firstTextChildByName(name).contentAsBool(defaultValue, ok); }
  /** @see contentAsStringList() */
  QStringList stringListAttribute(QString name) const {
    return firstTextChildByName(name).contentAsStringList(); }
  /** Set a child named 'name' with 'content' content and remove any other
    * child named 'name'. */
  PfNode &setAttribute(QString name, QString content);
  /** Convenience method */
  PfNode &setAttribute(QString name, QVariant content) {
    return setAttribute(name, content.toString()); }
  /** Convenience method (assume content is UTF-8 encoded) */
  PfNode &setAttribute(QString name, const char *content) {
    return setAttribute(name, QString::fromUtf8(content)); }
  // LATER setAttribute() for QDateTime, QDate, QTime and QStringList/QSet<QString>
  /** Set a child named 'name' with 'content' content and remove any other child
   * named 'name'. The QStringList is formated as a space separated value list
   * in a way that it can be parsed back by contentAsStringList() (i.e. using
   * backslash escapement for whitespace and backslashes).
   * @see contentAsStringList()
   */
  PfNode &setAttribute(QString name, QStringList content);
  /** Construct a list of all children named 'name'. */
  const QList<PfNode> childrenByName(QString name) const;
  bool hasChild(QString name) const;
  /** This PfNode has no children. Null nodes are leaves */
  inline bool isLeaf() const;
  inline PfNode &removeAllChildren();
  PfNode &removeChildrenByName(QString name);

  // Content related methods //////////////////////////////////////////////////

  /** @return true when there is no content (neither text or binary fragment or
   * array content) */
  bool isEmpty() const { return !d || d->isEmpty(); }
  /** @return true if the content is an array */
  bool isArray() const { return d && d->isArray(); }
  /** @return true if the content consist only of text data (no binary no array)
   * or is empty or the node is null, false for comment nodes */
  bool isText() const { return !d || d->isText(); }
  /** @return true if the content is (fully or partly) binary data, therefore
   * false when empty */
  bool isBinary() const { return d && d->isBinary(); }
  /** @return QString() if isBinary() or isArray() or isNull(), and QString("")
   * if isText() even if isEmpty() */
  QString contentAsString() const {
    return d ? d->contentAsString() : QString(); }
  /** @return integer value if the string content T a valid C-like integer */
  qint64 contentAsLong(qint64 defaultValue = 0, bool *ok = 0) const;
  /** @return decimal value if the string content is a valid E notation number
   * the implementation does not fully support the PF specications since it
   * uses QString::toDouble() which relies on the default locale
   * (QLocale::setDefault()) to define the separators (especially comma versus
   * period) */
  double contentAsDouble(double defaultValue = 0.0, bool *ok = 0) const;
  /** @return bool value if the child string content is a valid boolean
   * "true" regardless of case and any non null integer are regarded as true
   * "false" regardless of case and 0 are regarded as false
   * any other text is regarded as invalid */
  bool contentAsBool(bool defaultValue = false, bool *ok = 0) const;
  /** Split text content into strings on whitespace (e.g. "foo bar baz" and
   * "    foo  bar\nbaz" are both interpreted as the same 3 items list).
   * Whitespace can be escaped with backspaces. Actually backspace must be
   * doubled since it's already an escape character in PF syntax (e.g.
   * "foo\\ 1 bar baz" first element is "foo 1"). */
  QStringList contentAsStringList() const;
  /** Split text content into two strings on first non-leading whitespace.
   * e.g. "foo bar baz" and "    foo  bar baz" are both interpreted as the same
   * 2 items list: { "foo", "bar baz" }.
   * Whitespace cannot be escaped. */
  QStringList contentAsTwoStringsList() const;
  /** @return QByteArray() if isEmpty() otherwise raw content (no escape
   * for PF special characters) */
  QByteArray contentAsByteArray() const {
    return d ? d->contentAsByteArray() : QByteArray(); }
  /** @return PfArray() if not isArray() */
  PfArray contentAsArray() const { return d ? d->_array : PfArray(); }
  /** Append text fragment to context (and remove array if any). */
  PfNode &appendContent(const QString text) {
    if (!d)
      d = new PfNodeData();
    d->_array.clear();
    if (!text.isEmpty()) {
      // merge fragments if previous one exists and is text
      if (!d->_fragments.isEmpty()) {
        PfFragment &last = d->_fragments.last();
        if (last.isText()) {
          last = PfFragment(last.text()+' '+text);
          return *this;
        }
      }
      // otherwise append new fragment
      d->_fragments.append(PfFragment(text));
    }
    return *this;
  }
  /** Append text fragment to context (and remove array if any). */
  PfNode &appendContent(const char *utf8text) {
    return appendContent(QString::fromUtf8(utf8text)); }
  /** Append in-memory binary fragment to context (and remove array if any). */
  PfNode &appendContent(QByteArray data, QString surface = QString()) {
    if (!d)
      d = new PfNodeData();
    d->_array.clear();
    // Merging fragments if previous is in-memory binary is probably a bad idea
    // because it would prevent Qt's implicit sharing to work.
    if (!data.isEmpty())
      d->_fragments.append(PfFragment(data, surface));
    return *this;
  }
  /** Append lazy-loaded binary fragment to context (and remove array if any) */
  PfNode &appendContent(QIODevice *device, qint64 length, qint64 offset,
                        QString surface = QString()) {
    if (!d)
      d = new PfNodeData();
    d->_array.clear();
    if (device && length > 0)
      d->_fragments
          .append(PfFragment(device, length, offset, surface));
    return *this;
  }
  /** Replace current content with text fragment. */
  PfNode &setContent(QString text) {
    clearContent(); appendContent(text); return *this; }
  /** Replace current content with text fragment. */
  PfNode &setContent(const char *utf8text) {
    setContent(QString::fromUtf8(utf8text)); return *this; }
  /** Replace current content with in-memory binary fragment. */
  PfNode &setContent(QByteArray data) {
    clearContent(); appendContent(data); return *this; }
  /** Replace current content with lazy-loaded binary fragment. */
  PfNode &setContent(QIODevice *device, qint64 length, qint64 offset) {
    clearContent(); appendContent(device, length, offset); return *this; }
  /** Replace current content with an array. */
  PfNode &setContent(PfArray array) {
    if (!d)
      d = new PfNodeData();
    d->_fragments.clear();
    d->_array = array;
    return *this;
  }
  /** Replace current content with a text content containing a space separated
   * strings list. Backspaces and spaces inside strings are escaped with
   * backslash */
  PfNode &setContent(QStringList strings);
  /** Remove current content and make the node content empty (and thus text). */
  PfNode &clearContent() {
    if (d) {
      d->_array.clear();
      d->_fragments.clear();
    }
    return *this;
  }

  // Output methods ///////////////////////////////////////////////////////////

  /** Write the whole PfNode tree in PF file format. */
  qint64 writePf(QIODevice *target, PfOptions options = PfOptions()) const {
    return d ? d->writePf(target, options) : 0; }
  /** Convert the whole PfNode tree to PF in a byte array. */
  QByteArray toPf(PfOptions options = PfOptions()) const;
  /** Convert the whole PfNode tree to PF in a characters string.
    * Note that the string will be truncated to the first \0 encountered,
    * which may happen inside binary segments, if any, therefore this
    * method is only for debuging or human-readable display, not for
    * data output (which should use writePf() instead).
    */
  QString toString() const {
    return QString::fromUtf8(toPf(PfOptions().setShouldIndent()));
  }
  /** Write node and whole tree (children recursively) in flat XML format.
    * Flat XML format is a format without any attribute (every PF node is
    * written as an XML element) and with binary content converted into
    * Base64 text. Encoding is always UTF-8. */
  qint64 writeFlatXml(QIODevice *target,
                      PfOptions options = PfOptions()) const {
    return d ? d->writeFlatXml(target, options) : 0; }
  // LATER test, debug and uncomment method
  /* Write node and whole tree (children recursively) in compatible XML format.
    * Compatible XML format is a format where every subnode with no subnode
    * and with a unique name among its siblings and with a small, non binary,
    * content is written twice: once as an attribute and once as a subelement.
    * Small means < 256 characters (not bytes).
    * Binary fragments are lost. Maybe later they will be written with an XML
    * escape mean such as entities or cdata, or with a non-XML mean such as
    * base64 content.
    * Encoding is always UTF-8.
    */
  //qint64 writeCompatibleXml(QIODevice *target) const;
};

Q_DECLARE_METATYPE(PfNode)
Q_DECLARE_TYPEINFO(PfNode, Q_MOVABLE_TYPE);

// following methods are those that should be declared after
// Q_DECLARE_TYPEINFO(PfNode, Q_MOVABLE_TYPE) because they call QList methods
// that depends on its internal memory layout, and 'inline' declaration does not
// guarantee that the method will be inlined by the compiler

inline bool PfNode::isLeaf() const {
  return !d || d->_children.size() == 0;
}

inline PfNode &PfNode::removeAllChildren() {
  if (d)
    d->_children.clear();
  return *this;
}

inline PfNode &PfNode::prependChild(PfNode child) {
  if (!child.isNull()) {
    if (!d)
      d = new PfNodeData();
    d->_children.prepend(child);
  }
  return *this;
}

inline PfNode &PfNode::appendChild(PfNode child) {
  if (!child.isNull()) {
    if (!d)
      d = new PfNodeData();
    d->_children.append(child);
  }
  return *this;
}

#endif // PFNODE_H
