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

#ifndef PFNODE_H
#define PFNODE_H

#include "pffragment_p.h"
#include "pfarray.h"
#include "util/utf8stringlist.h"
#include <QVariant>
#include <QBuffer>
#if QT_VERSION < 0x060000
#include <QVector>
#endif

class PfNode;

class LIBP6CORESHARED_EXPORT PfNodeData : public QSharedData {
  friend class PfNode;

  QString _name;
  QList<PfNode> _children;
  bool _isComment;
  QList<PfFragment> _fragments;
  PfArray _array;

public:
  explicit PfNodeData(const QString &name = QString())
    : _name(name), _isComment(false) { }

private:
  PfNodeData(const QString &name, const QString &content, bool isComment)
    : _name(name), _isComment(isComment) {
    if (!content.isEmpty())
      _fragments.append(PfFragment(content));
  }
  PfNodeData(const QString &name, const PfArray &array)
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
  qint64 writePf(QIODevice *target, const PfOptions &options) const;
  qint64 writeFlatXml(QIODevice *target, const PfOptions &options) const;
  //qint64 writeCompatibleXml(QIODevice &target) const;
  //inline void buildChildrenFromArray() const;
  inline qint64 internalWritePf(
      QIODevice *target, QString indent, const PfOptions &options) const;
  inline qint64 internalWritePfSubNodes(
      QIODevice *target, QString indent, const PfOptions &options) const;
  inline qint64 internalWritePfContent(
      QIODevice *target, const QString &indent, const PfOptions &options) const;
  /** Provide the content as a byte array.
    * If there are lazy-loaded binary fragments, they are loaded into memory,
    * in the returned QByteArray but do not keep them cached inside PfContent
    * structures, therefore the memory will be freed when the QByteArray is
    * discarded and if toByteArray() is called again, the data will be
    * loaded again. */
  QByteArray contentAsByteArray() const;
  /** Write content to target device in PF format (with escape sequences and
    * binary headers). */
  qint64 writePfContent(QIODevice *target, const PfOptions &options) const;
  /** Write content to target device in raw data format (no PF escape sequences
    * but actual content). */
  qint64 writeRawContent(QIODevice *target, const PfOptions &options) const;
  /** Write content to target device in XML format, embeding binary fragments
    * using base64 encoding. */
  qint64 writeXmlUsingBase64Content(
      QIODevice *target, const PfOptions &options) const;

};

class LIBP6CORESHARED_EXPORT PfNode {
  friend class PfNodeData;
  QSharedDataPointer<PfNodeData> d;
  static const QList<PfNode> _emptyList;

  PfNode(PfNodeData *data) : d(data) { }

public:
  /** Create a null node. */
  PfNode() { }
  PfNode(const PfNode &other) : d(other.d) { }
  /** If name is empty, the node will be null. */
  explicit PfNode(const QString &name)
      : d(name.isEmpty() ? 0 : new PfNodeData(name)) { }
  /** If name is empty, the node will be null. */
  PfNode(const QString &name, const QString &content)
    : d(name.isEmpty() ? 0 : new PfNodeData(name, content, false)) { }
  /** If name is empty, the node will be null. */
  PfNode(const QString &name, qint64 content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(const QString &name, quint64 content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(const QString &name, qint32 content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(const QString &name, quint32 content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(const QString &name, qint16 content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(const QString &name, quint16 content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(const QString &name, double content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number(content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(const QString &name, float content)
    : d(name.isEmpty()
        ? 0 : new PfNodeData(name, QString::number((double)content), false)) { }
  /** If name is empty, the node will be null. */
  PfNode(const QString &name, const PfArray &array)
    : d(name.isEmpty() ? 0 : new PfNodeData(name, array)) { }
  /** If name is empty, the node will be null (and children ignored). */
  PfNode(const QString &name, std::initializer_list<PfNode> children)
      : d(name.isEmpty() ? 0 : new PfNodeData(name)) {
    if (!name.isEmpty())
      appendChildren(children);
  }
  /** If name is empty, the node will be null (and children ignored). */
  PfNode(const QString &name, const QString &content,
         std::initializer_list<PfNode> children)
    : d(name.isEmpty() ? 0 : new PfNodeData(name, content, false)) {
    if (!name.isEmpty())
      appendChildren(children);
  }
  /** If name is empty, the node will be null (and children ignored). */
  PfNode(const QString &name, const PfArray &array,
         std::initializer_list<PfNode> children)
    : d(name.isEmpty() ? 0 : new PfNodeData(name, array)) {
    if (!name.isEmpty())
      appendChildren(children);
  }
  /** Create a comment node. */
  static PfNode createCommentNode(const QString &comment) {
    return PfNode(new PfNodeData(QStringLiteral("comment"), comment, true)); }
  PfNode &operator=(const PfNode &other) { d = other.d; return *this; }
  /** Build a PfNode from PF external format.
   * @return first encountered root node or PfNode() */
  static PfNode fromPf(QByteArray source, PfOptions options = PfOptions());

  // Node related methods /////////////////////////////////////////////////////

  /** A node has an empty string name if and only if the node is null. */
  QString name() const { return d ? d->_name : QString(); }
  Utf8String utf8Name() const { return name(); }
  /** Replace node name. If name is empty, the node will become null. */
  PfNode &setName(const QString &name) {
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
  inline PfNode &prependChild(const PfNode &child);
  /** append a child to existing children (do nothing if child.isNull()) */
  inline PfNode &appendChild(const PfNode &child);
  PfNode &appendChildren(std::initializer_list<PfNode> children) {
    for (const PfNode &child : children)
      appendChild(child);
    return *this; }
  PfNode &appendChildren(QList<PfNode> children) {
    for (const PfNode &child : children)
      appendChild(child);
    return *this; }
#if QT_VERSION < 0x060000
  PfNode &appendChildren(QVector<PfNode> children) {
    for (const PfNode &child : children)
      appendChild(child);
    return *this; }
#endif
  PfNode &prependCommentChild(const QString &comment) {
    return prependChild(createCommentNode(comment)); }
  PfNode &appendCommentChild(const QString &comment) {
    return appendChild(createCommentNode(comment)); }
  /** @return first text child by name
   * Most of the time one will use attribute() and xxxAttribute() methods rather
   * than directly calling firstTextChildByName(). */
  PfNode firstTextChildByName(const QString &name) const;
  /** Return a child content knowing the child name.
    * QString() if no text child exists.
    * QString("") if child exists but has no content
    * If several children have the same name the first text one is choosen.
    * The goal is to emulate XML attributes, hence the name. */
  QString attribute(const QString &name) const {
    PfNode child = firstTextChildByName(name);
    return child.isNull() ? QString() : child.contentAsString(); }
  Utf8String utf8Attribute(const QString &name) const {
    PfNode child = firstTextChildByName(name);
    return child.isNull() ? Utf8String{} : child.contentAsUtf8(); }
  /** Return a child content knowing the child name.
    * defaultValue if no text child exists.
    * QString("") if child exists but has no content
    * If several children have the same name the first text one is choosen.
    * The goal is to emulate XML attributes, hence the name. */
  QString attribute(const QString &name, const QString &defaultValue) const {
    PfNode child = firstTextChildByName(name);
    return child.isNull() ? defaultValue : child.contentAsString(); }
  Utf8String utf8Attribute(
      const Utf8String &name, const Utf8String &defaultValue) const {
    PfNode child = firstTextChildByName(name);
    return child.isNull() ? defaultValue : child.contentAsUtf8(); }
  /** Return the content (as string) of every child with a given name.
   * This is the same as attribute() with multi-valued semantics.
   * Skip children with non-text content.
   * If no text child matches the name, the list is empty. */
  QStringList stringChildrenByName(const QString &name) const;
  /** Return the string content of children, splited into string pairs at the
   * first whitespace, one list item per child.
   * Child whole content and both strings of the pair are trimmed.
   * Skip children with non-text content.
   * Chilren without whitespace will have the first pair item set to the whole
   * node content (which may be empty) and the second one to QString().
   * If no text child matches the name, the list is empty. */
  QList<QPair<QString,QString>> stringsPairChildrenByName(
      const QString &name) const;
  /** Return the integer content of children, splited into pairs at the
   * first whitespace, one list item per child.
   * @see stringsPairChildrenByName() */
  QList<QPair<QString, qint64>> stringLongPairChildrenByName(
      const QString &name) const;
  /** @see contentAsLong() */
  qint64 longAttribute(const QString &name, qint64 defaultValue = 0,
                       bool *ok = 0) const {
    return firstTextChildByName(name).contentAsLong(defaultValue, ok); }
  /** @see contentAsDouble() */
  double doubleAttribute(const QString &name, double defaultValue,
                         bool *ok = 0) const {
    return firstTextChildByName(name).contentAsDouble(defaultValue, ok); }
  // LATER contentAsDateTime()
  /** @see contentAsBool() */
  bool boolAttribute(const QString &name, bool defaultValue = false,
                     bool *ok = 0) const {
    return firstTextChildByName(name).contentAsBool(defaultValue, ok); }
  /** @see contentAsStringList() */
  QStringList stringListAttribute(const QString &name) const {
    return firstTextChildByName(name).contentAsStringList(); }
  /** Set a child named 'name' with 'content' content and remove any other
    * child named 'name'. */
  PfNode &setAttribute(const QString &name, const QString &content);
  /** Convenience method */
  PfNode &setAttribute(const QString &name, const QVariant &content) {
    return setAttribute(name, content.toString()); }
  /** Convenience method (assume content is UTF-8 encoded) */
  PfNode &setAttribute(const QString &name, const char *content) {
    return setAttribute(name, QString::fromUtf8(content)); }
  /** Convenience method (assume content is UTF-8 encoded) */
  PfNode &setAttribute(const QString &name, const Utf8String &utf8) {
    return setAttribute(name, utf8); }
  // LATER setAttribute() for QDateTime, QDate, QTime and QStringList/QSet<QString>
  /** Set a child named 'name' with 'content' content and remove any other child
   * named 'name'. The QStringList is formated as a space separated value list
   * in a way that it can be parsed back by contentAsStringList() (i.e. using
   * backslash escapement for whitespace and backslashes).
   * @see contentAsStringList()
   */
  PfNode &setAttribute(const QString &name, const QStringList &content);
  /** Construct a list of all children named 'name'. */
  QList<PfNode> childrenByName(const QString &name) const;
  QList<PfNode> childrenByName(const QStringList &names) const;
  /** Construct a list of all children of children named 'name'. */
  QList<PfNode> grandChildrenByChildrenName(const QString &name) const;
  QList<PfNode> grandChildrenByChildrenName(const QStringList &names) const;
  bool hasChild(const QString &name) const;
  /** This PfNode has no children. Null nodes are leaves */
  inline bool isLeaf() const;
  inline PfNode &removeAllChildren();
  PfNode &removeChildrenByName(const QString &name);

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
  Utf8String contentAsUtf8() const { return contentAsString(); }
  /** @return integer value if the string content is a valid integer
   * C-like prefixes are supported and both kmb and kMGTP suffixes are supported
   * surrounding whitespace is trimmed
   * e.g. 0x1f means 15, 12k means 12000, 12b and 12G mean 12000000000.
   * however mixing them is not supported e.g. 0x1fG isn't. */
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
  Utf8StringList contentAsUtf8List() const;
  /** Split text content into two strings on first non-leading whitespace.
   * e.g. "foo bar baz" and "    foo  bar baz" are both interpreted as the same
   * 2 items list: { "foo", "bar baz" }.
   * List may contain only 1 or 0 element, depending on node content.
   * Whitespace cannot be escaped. */
  QStringList contentAsTwoStringsList() const;
  /** @return QByteArray() if isEmpty() otherwise raw content (no escape
   * for PF special characters) */
  QByteArray contentAsByteArray() const {
    return d ? d->contentAsByteArray() : QByteArray(); }
  /** @return PfArray() if not isArray() */
  PfArray contentAsArray() const { return d ? d->_array : PfArray(); }
  /** Append text fragment to context (and remove array if any). */
  PfNode &appendContent(const QString &text) {
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
  PfNode &appendContent(QByteArray data, const QString &surface = QString()) {
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
                        const QString &surface = QString()) {
    if (!d)
      d = new PfNodeData();
    d->_array.clear();
    if (device && length > 0)
      d->_fragments
          .append(PfFragment(device, length, offset, surface));
    return *this;
  }
  /** Replace current content with text fragment. */
  PfNode &setContent(const QString &text) {
    clearContent(); appendContent(text); return *this; }
  /** Replace current content with text fragment. */
  PfNode &setContent(const char *utf8text) {
    setContent(QString::fromUtf8(utf8text)); return *this; }
  /** Replace current content with in-memory binary fragment. */
  PfNode &setContent(const QByteArray &data) {
    clearContent(); appendContent(data); return *this; }
  /** Replace current content with lazy-loaded binary fragment. */
  PfNode &setContent(QIODevice *device, qint64 length, qint64 offset) {
    clearContent(); appendContent(device, length, offset); return *this; }
  /** Replace current content with an array. */
  PfNode &setContent(const PfArray &array) {
    if (!d)
      d = new PfNodeData();
    d->_fragments.clear();
    d->_array = array;
    return *this;
  }
  /** Replace current content with a text content containing a space separated
   * strings list. Backspaces and spaces inside strings are escaped with
   * backslash */
  PfNode &setContent(const QStringList &strings);
  /** Remove current content and make the node content empty (and thus text). */
  PfNode &clearContent() {
    if (d) {
      d->_array.clear();
      d->_fragments.clear();
    }
    return *this;
  }
  /** Compares contentAsString() */
  bool operator<(const PfNode &other) const {
    return contentAsString() < other.contentAsString();
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
                      const PfOptions &options = PfOptions()) const {
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

inline PfNode &PfNode::prependChild(const PfNode &child) {
  if (!child.isNull()) {
    if (!d)
      d = new PfNodeData();
    d->_children.prepend(child);
  }
  return *this;
}

inline PfNode &PfNode::appendChild(const PfNode &child) {
  if (!child.isNull()) {
    if (!d)
      d = new PfNodeData();
    d->_children.append(child);
  }
  return *this;
}

#endif // PFNODE_H
