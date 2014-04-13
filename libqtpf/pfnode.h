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

#ifndef PFNODE_H
#define PFNODE_H

#include "libqtpf_global.h"
#include <QString>
#include <QList>
#include <QIODevice>
#include "pfcontent.h"
#include <QVariant>
#include <QSharedData>
#include "pfoptions.h"
#include <QStringList>

class PfNode;

class LIBQTPFSHARED_EXPORT PfNodeData : public QSharedData {
  friend class PfNode;

  QString _name;
  QList<PfNode> _children;
  bool _isComment;
  PfContent _content;

public:
  explicit inline PfNodeData(QString name = QString()) : _name(name),
    _isComment(false) { }

private:
  inline PfNodeData(QString name, QString content, bool isComment = false)
    : _name(name), _isComment(isComment) { _content.append(content); }
  inline bool isNull() const { return _name.isNull(); }
  inline bool isComment() const { return _isComment; }
  qint64 writePf(QIODevice *target, PfOptions options) const;
  qint64 writeFlatXml(QIODevice *target, PfOptions options) const;
  //qint64 writeCompatibleXml(QIODevice &target) const;
  //inline void buildChildrenFromArray() const;
  inline qint64 internalWritePf(QIODevice *target, QString indent,
                                PfOptions options) const;
};

class LIBQTPFSHARED_EXPORT PfNode {
  friend class PfNodeData;
  // LATER return PfNode& from currently void-returning inline methods
private:
  QSharedDataPointer<PfNodeData> d;

public:
  explicit inline PfNode(QString name = QString()) : d(new PfNodeData(name)) { }
  inline PfNode(QString name, QString content, bool isComment = false)
    : d(new PfNodeData(name, content, isComment)) { }
  inline PfNode &operator=(const PfNode &other) { d = other.d; return *this; }
  /** Build a PfNode from PF external format.
   * @return first encountered root node or PfNode() */
  static PfNode fromPf(QByteArray source, PfOptions options = PfOptions());

  // Node related methods /////////////////////////////////////////////////////

  inline QString name() const { return d->_name; }
  inline bool isNull() const { return d->isNull(); }
  inline bool isComment() const { return d->isComment(); }

  // Children related methods /////////////////////////////////////////////////

  inline const QList<PfNode> children() const { return d->_children; }
  inline void appendChild(PfNode child) { d->_children.append(child); }
  /** @return first text child by name
   * Most of the time one will use attribute() and xxxAttribute() methods rather
   * than directly calling firstTextChildByName(). */
  PfNode firstTextChildByName(QString name) const;
  /** Return a child content knowing the child name.
    * QString() if no text child exists.
    * QString("") if child exists but has no content
    * If several children have the same name the first text one is choosen.
    * The goal is to emulate XML attributes, hence the name. */
  inline QString attribute(QString name) const {
    PfNode child = firstTextChildByName(name);
    return child.isNull() ? QString() : child.contentAsString(); }
  /** Return a child content knowing the child name.
    * defaultValue if no text child exists.
    * QString("") if child exists but has no content
    * If several children have the same name the first text one is choosen.
    * The goal is to emulate XML attributes, hence the name. */
  inline QString attribute(QString name, QString defaultValue) const {
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
  qint64 longAttribute(QString name, qint64 defaultValue, bool *ok = 0) const {
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
  void setAttribute(QString name, QString content);
  /** Convenience method */
  inline void setAttribute(QString name, QVariant content) {
    setAttribute(name, content.toString()); }
  /** Convenience method (assume content is UTF-8 encoded) */
  inline void setAttribute(QString name, const char *content) {
    setAttribute(name, QString::fromUtf8(content)); }
  // LATER setAttribute() for QDateTime, QDate, QTime and QList<QString>/QSet<QString>
  void setAttribute(QString name, QList<QString> content);
  //  /** Syntaxic sugar. */
  //  inline void setAttribute(QString name, qint64 integer) {
  //    setAttribute(name, QString::number(integer));
  //  }
  //  /** Syntaxic sugar. */
  //  inline void setAttribute(QString name, double integer) {
  //    setAttribute(name, QString::number(integer));
  //  }
  /** Construct a list of all children named 'name'. */
  const QList<PfNode> childrenByName(QString name) const;
  bool hasChild(QString name) const;
  bool isLeaf() const { return d->_children.size() == 0; }
  void removeAllChildren() { d->_children.clear(); }
  void removeChildrenByName(QString name);

  // Content related methods //////////////////////////////////////////////////

  /** @return true when there is no content */
  inline bool contentIsEmpty() const { return d->_content.isEmpty(); }
  /** @return true if the content consist only of text data (no binary or
   * array) or is empty */
  inline bool contentIsText() const { return d->_content.isText(); }
  /** @return true if the content is (fully or partly) binary data, therefore
   * false when empty */
  inline bool contentIsBinary() const { return d->_content.isBinary(); }
  /** @return true if the content is an array
    */
  inline bool contentIsArray() const { return d->_content.isArray(); }
  /** @return QString() if contentIsBinary() or contentIsArray(), and
   * QString("") if contentIsEmpty() */
  inline QString contentAsString() const { return d->_content.toString(); }
  /** @return integer value if the string content is a valid C-like integer */
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
  /** Split text content into strings on whitespace (e.g. "foo bar baz",
   * "    foo  bar\nbaz" are both interpreted as the same 3 items list).
   * Whitespace can be escaped with backspaces. Actually backspace must be
   * doubled since it's already an escape character in PF syntax (e.g.
   * "foo\\ 1 bar baz" first element is "foo 1"). */
  QStringList contentAsStringList() const;
  /** @return QByteArray() if contentIsEmpty() otherwise raw content (no escape
   * for PF special characters) */
  inline QByteArray contentAsByteArray() const {
    return d->_content.toByteArray(); }
  /** @return PfArray() if not contentIsArray() */
  PfArray contentAsArray() const { return d->_content.array(); }
  /** Append text fragment to context (and remove array if any). */
  inline void appendContent(const QString text) { d->_content.append(text); }
  /** Append text fragment to context (and remove array if any). */
  inline void appendContent(const char *utf8text) {
    appendContent(QString::fromUtf8(utf8text)); }
  /** Append in-memory binary fragment to context (and remove array if any). */
  inline void appendContent(QByteArray data, QString surface = QString()) {
    d->_content.append(data, surface); }
  /** Append lazy-loaded binary fragment to context (and remove array if any) */
  inline void appendContent(QIODevice *device, qint64 length, qint64 offset,
                            QString surface = QString()) {
    d->_content.append(device, length, offset, surface); }
  /** Replace current content with text fragment. */
  inline void setContent(QString text) {
    d->_content.clear(); appendContent(text); }
  /** Replace current content with text fragment. */
  inline void setContent(const char *utf8text) {
    setContent(QString::fromUtf8(utf8text)); }
  /** Replace current content with in-memory binary fragment. */
  inline void setContent(QByteArray data) {
    d->_content.clear(); appendContent(data); }
  /** Replace current content with lazy-loaded binary fragment. */
  inline void setContent(QIODevice *device, qint64 length, qint64 offset) {
    d->_content.clear(); appendContent(device, length, offset); }
  /** Replace current content with an array. */
  inline void setContent(PfArray array) { d->_content.set(array); }
  /** Replace current content with a text content containing a space separated
   * strings list. Backspaces and spaces inside strings are escaped with
   * backslash */
  void setContent(QList<QString> strings);

  // Output methods ///////////////////////////////////////////////////////////

  /** Write the whole PfNode tree in PF file format. */
  inline qint64 writePf(QIODevice *target,
                        PfOptions options = PfOptions()) const {
    return d->writePf(target, options); }
  /** Convert the whole PfNode tree to PF in a byte array. */
  QByteArray toPf(PfOptions options = PfOptions()) const;
  /** Convert the whole PfNode tree to PF in a characters string.
    * Note that the string will be truncated to the first \0 encountered,
    * which may happen inside binary segments, if any, therefore this
    * method is only for debuging or human-readable display, not for
    * data output (which should use writePf() instead).
    */
  inline QString toString() const {
    return QString::fromUtf8(toPf(PfOptions().setShouldIndent()));
  }
  /** Convenience operator to transparently convert into a QString */
  /*inline operator QString() const {
    return QString::fromUtf8(toPf(true));
  }*/
  /** Write node and whole tree (children recursively) in flat XML format.
    * Flat XML format is a format without any attribute (every PF node is
    * written as an XML element) and with binary content converted into
    * Base64 text. Encoding is always UTF-8. */
  inline qint64 writeFlatXml(QIODevice *target,
                             PfOptions options = PfOptions()) const {
    return d->writeFlatXml(target, options); }
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
  /** Write the node content (without node structure and children tree)
    * with no escape for PF special chars and so on. */
  inline qint64 writeRawContent(QIODevice *target,
                                PfOptions options = PfOptions()) const {
    return d->_content.writeRaw(target, options); }
  /** Write the node content (without node structure and children tree)
    * in PF syntax (escaping special chars and adding binary fragment headers).
    */
  inline qint64 writeContentAsPf(QIODevice *target,
                                 PfOptions options = PfOptions()) const {
    return d->_content.writePf(target, options); }
};

#endif // PFNODE_H
