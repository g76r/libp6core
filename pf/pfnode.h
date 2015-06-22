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

#ifndef PFNODE_H
#define PFNODE_H

#include "libqtpf_global.h"
#include <QString>
#include <QList>
#include <QIODevice>
#include <QVariant>
#include <QSharedData>
#include "pfoptions.h"
#include <QStringList>
#include <QBuffer>
#include "pfarray.h"

class PfNode;

class LIBQTPFSHARED_EXPORT PfNodeData : public QSharedData {
  friend class PfNode;

  class PfFragment;

  /** Internal class for Qt's implicit sharing idiom.
    * @see PfFragment */
  class LIBQTPFSHARED_EXPORT PfFragmentData : public QSharedData {
  public:
    enum Format {Raw, Pf, XmlBase64 };
    virtual ~PfFragmentData();
    virtual qint64 write(
        QIODevice *target, Format format, PfOptions options) const = 0;
    virtual bool isText() const;
    virtual QString text() const;
    virtual bool isEmpty() const;
    virtual bool isBinary() const;
    virtual bool isLazyBinary() const;
  };

  class LIBQTPFSHARED_EXPORT PfTextFragmentData : public PfFragmentData {
  public:
    QString _text;
    explicit inline PfTextFragmentData(QString text) : _text(text) { }
    qint64 write(QIODevice *target, Format format, PfOptions options) const;
    bool isText() const;
    QString text() const;
  };

  class LIBQTPFSHARED_EXPORT PfAbstractBinaryFragmentData
      : public PfFragmentData {
  public:
    QString _surface;
    qint64 _size; // real data size, surface removed
    bool isBinary() const;

  protected:
    PfAbstractBinaryFragmentData() : _size(0) { }
    inline qint64 writeDataApplyingSurface(
        QIODevice *target, Format format, PfOptions options,
        QByteArray data) const;
    inline QString takeFirstLayer(QString &surface) const;
    inline bool removeSurface(QByteArray &data, QString surface) const;
    inline bool applySurface(QByteArray &data, QString surface) const;
    inline qint64 measureSurface(QByteArray data, QString surface) const;
  };

  class LIBQTPFSHARED_EXPORT PfBinaryFragmentData
      : public PfAbstractBinaryFragmentData {
  public:
    QByteArray _data;
    inline PfBinaryFragmentData(QByteArray data, QString surface)
      : _data(data) { setSurface(surface, true); }
    void setSurface(QString surface, bool shouldAdjustSize);
    qint64 write(QIODevice *target, Format format, PfOptions options) const;
  };

  class LIBQTPFSHARED_EXPORT PfLazyBinaryFragmentData
      : public PfAbstractBinaryFragmentData {
  public:
    mutable QIODevice *_device; // TODO manage ownership of the device
    qint64 _length; // raw data length on device, surface applyied
    qint64 _offset;
    inline PfLazyBinaryFragmentData(
        QIODevice *device, qint64 length, qint64 offset, QString surface)
      : _device(device), _length(length), _offset(offset) {
      setSurface(surface, true); }
    void setSurface(QString surface, bool shouldAdjustSize);
    qint64 write(QIODevice *target, Format format, PfOptions options) const;
    bool isLazyBinary() const;
  };

  /** Fragment of PF node content, this class is only for internal use of
    * implementation, mainly PfContent. It should not be used directly by
    * application code.
    *
    * A fragment is either text or binary or array.
    * A binary fragment can be lazy or not.
    * There is no difference between a null or empty fragment.
    * An empty fragment is a text fragment.
    */
  class LIBQTPFSHARED_EXPORT PfFragment {
  private:
    QSharedDataPointer<PfFragmentData> d;

  public:
    inline PfFragment() { }
    explicit inline PfFragment(QString text)
      : d(new PfTextFragmentData(text)) { }
    inline PfFragment(QIODevice *device, qint64 length, qint64 offset,
                      QString surface)
      : d(new PfLazyBinaryFragmentData(device, length, offset, surface)) { }
    inline PfFragment(QByteArray data, QString surface)
      : d(new PfBinaryFragmentData(data, surface)) { }
    inline PfFragment(const PfFragment &other) : d(other.d) { }
    PfFragment &operator =(const PfFragment &other) { d = other.d; return *this; }
    inline bool isEmpty() const { return d ? d->isEmpty() : true; }
    inline bool isText() const { return d ? d->isText() : true; }
    inline bool isBinary() const { return d ? d->isBinary() : false; }
    inline bool isLazyBinary() const { return d ? d->isLazyBinary() : false; }
    /** binary size (for text: size of text in UTF-8) */
    // inline qint64 size() const { return d ? d->_size : 0; }
    /** .isNull() if binary fragment */
    inline QString text() const { return d ? d->text() : QString(); }
    /** Write content as PF-escaped string or binary with header. */
    inline qint64 writePf(QIODevice *target, PfOptions options) const {
      return d ? d->write(target, PfFragmentData::Pf, options) : 0;
    }
    /** Write actual content in unescaped format. */
    inline qint64 writeRaw(QIODevice *target, PfOptions options) const {
      return d ? d->write(target, PfFragmentData::Raw, options) : 0;
    }
    /** Write content as XML string, using base64 encoding for binary fragments */
    inline qint64 writeXmlUsingBase64(QIODevice *target,
                                      PfOptions options) const {
      return d ? d->write(target, PfFragmentData::XmlBase64, options) : 0;
    }
  };

  QString _name;
  QList<PfNode> _children;
  bool _isComment;
  QList<PfFragment> _fragments;
  PfArray _array;

public:
  explicit inline PfNodeData(QString name = QString()) : _name(name),
    _isComment(false) { }

private:
  inline PfNodeData(QString name, QString content, bool isComment)
    : _name(name), _isComment(isComment) {
    if (!content.isEmpty())
      _fragments.append(PfFragment(content));
  }
  inline bool isComment() const { return _isComment; }
  inline bool isEmpty() const {
    return !_fragments.size() && _array.isNull(); }
  inline bool isArray() const { return !_array.isNull(); }
  inline bool isText() const {
    return !isArray() && !isBinary() && !isComment(); }
  inline bool isBinary() const {
    foreach (const PfFragment &f, _fragments)
      if (f.isBinary())
        return true;
    return false;
  }
  inline QString contentAsString() const {
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
  // LATER return PfNode& from currently void-returning inline methods
private:
  QSharedDataPointer<PfNodeData> d;

  inline PfNode(PfNodeData *data) : d(data) { }

public:
  /** Create a null node. */
  inline PfNode() { }
  inline PfNode(const PfNode &other) : d(other.d) { }
  /** If name is empty, the node will be null. */
  explicit inline PfNode(QString name)
      : d(name.isEmpty() ? 0 : new PfNodeData(name)) { }
  /** If name is empty, the node will be null. */
  inline PfNode(QString name, QString content)
    : d(name.isEmpty() ? 0 : new PfNodeData(name, content, false)) { }
  /** Create a comment node. */
  static inline PfNode createCommentNode(QString comment) {
    return PfNode(new PfNodeData("comment", comment, true)); }
  inline PfNode &operator=(const PfNode &other) { d = other.d; return *this; }
  /** Build a PfNode from PF external format.
   * @return first encountered root node or PfNode() */
  static PfNode fromPf(QByteArray source, PfOptions options = PfOptions());

  // Node related methods /////////////////////////////////////////////////////

  /** A node has an empty string name if and only if the node is null. */
  inline QString name() const { return d ? d->_name : QString(); }
  /** Replace node name. If name is empty, the node will become null. */
  inline void setName(QString name) {
    if (name.isEmpty())
      d = 0;
    else {
      if (!d)
        d = new PfNodeData();
      d->_name = name;
    }
  }
  inline bool isNull() const { return !d; }
  inline bool isComment() const { return d && d->isComment(); }

  // Children related methods /////////////////////////////////////////////////

  inline const QList<PfNode> children() const {
    return d ? d->_children : QList<PfNode>(); }
  /** prepend a child to existing children (do nothing if child.isNull()) */
  inline void prependChild(PfNode child) {
    if (!child.isNull()) {
      if (!d)
        d = new PfNodeData();
      d->_children.prepend(child);
    }
  }
  /** append a child to existing children (do nothing if child.isNull()) */
  inline void appendChild(PfNode child) {
    if (!child.isNull()) {
      if (!d)
        d = new PfNodeData();
      d->_children.append(child);
    }
  }
  inline void prependCommentChild(QString comment) {
    prependChild(createCommentNode(comment));
  }
  inline void appendCommentChild(QString comment) {
    appendChild(createCommentNode(comment));
  }
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
  // LATER setAttribute() for QDateTime, QDate, QTime and QStringList/QSet<QString>
  // TODO document behaviour
  void setAttribute(QString name, QStringList content);
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
  /** This PfNode has no children. Null nodes are leaves */
  bool isLeaf() const { return !d || d->_children.size() == 0; }
  void removeAllChildren() { if (d) d->_children.clear(); }
  void removeChildrenByName(QString name);

  // Content related methods //////////////////////////////////////////////////

  /** @return true when there is no content (neither text or binary fragment or
   * array content) */
  inline bool isEmpty() const { return !d || d->isEmpty(); }
  /** @return true if the content is an array */
  inline bool isArray() const { return d && d->isArray(); }
  /** @return true if the content consist only of text data (no binary no array)
   * or is empty or the node is null, false for comment nodes */
  inline bool isText() const { return !d || d->isText(); }
  /** @return true if the content is (fully or partly) binary data, therefore
   * false when empty */
  inline bool isBinary() const { return d && d->isBinary(); }
  /** @return QString() if isBinary() or isArray() or isNull(), and QString("")
   * if isText() even if isEmpty() */
  inline QString contentAsString() const {
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
  /** Split text content into strings on whitespace (e.g. "foo bar baz",
   * "    foo  bar\nbaz" are both interpreted as the same 3 items list).
   * Whitespace can be escaped with backspaces. Actually backspace must be
   * doubled since it's already an escape character in PF syntax (e.g.
   * "foo\\ 1 bar baz" first element is "foo 1"). */
  QStringList contentAsStringList() const;
  /** @return QByteArray() if isEmpty() otherwise raw content (no escape
   * for PF special characters) */
  inline QByteArray contentAsByteArray() const {
    return d ? d->contentAsByteArray() : QByteArray(); }
  /** @return PfArray() if not isArray() */
  PfArray contentAsArray() const { return d ? d->_array : PfArray(); }
  /** Append text fragment to context (and remove array if any). */
  inline void appendContent(const QString text) {
    if (!d)
      d = new PfNodeData();
    d->_array.clear();
    // LATER merge fragments if previous one is text
    if (!text.isEmpty())
      d->_fragments.append(PfNodeData::PfFragment(text));
  }
  /** Append text fragment to context (and remove array if any). */
  inline void appendContent(const char *utf8text) {
    appendContent(QString::fromUtf8(utf8text)); }
  /** Append in-memory binary fragment to context (and remove array if any). */
  inline void appendContent(QByteArray data, QString surface = QString()) {
    if (!d)
      d = new PfNodeData();
    d->_array.clear();
    // Merging fragments if previous is in-memory binary is probably a bad idea
    // because it would prevent Qt's implicite sharing to work.
    if (!data.isEmpty())
      d->_fragments.append(PfNodeData::PfFragment(data, surface));
  }
  /** Append lazy-loaded binary fragment to context (and remove array if any) */
  inline void appendContent(QIODevice *device, qint64 length, qint64 offset,
                            QString surface = QString()) {
    if (!d)
      d = new PfNodeData();
    d->_array.clear();
    if (device && length > 0)
      d->_fragments
          .append(PfNodeData::PfFragment(device, length, offset, surface));
  }
  /** Replace current content with text fragment. */
  inline void setContent(QString text) {
    clearContent();
    appendContent(text); }
  /** Replace current content with text fragment. */
  inline void setContent(const char *utf8text) {
    setContent(QString::fromUtf8(utf8text)); }
  /** Replace current content with in-memory binary fragment. */
  inline void setContent(QByteArray data) {
    clearContent();
    appendContent(data); }
  /** Replace current content with lazy-loaded binary fragment. */
  inline void setContent(QIODevice *device, qint64 length, qint64 offset) {
    clearContent();
    appendContent(device, length, offset); }
  /** Replace current content with an array. */
  inline void setContent(PfArray array) {
    if (!d)
      d = new PfNodeData();
    d->_fragments.clear();
    d->_array = array; }
  /** Replace current content with a text content containing a space separated
   * strings list. Backspaces and spaces inside strings are escaped with
   * backslash */
  void setContent(QStringList strings);
  /** Remove current content and make the node content empty (and thus text). */
  inline void clearContent() {
    if (d) {
      d->_array.clear();
      d->_fragments.clear();
    }
  }

  // Output methods ///////////////////////////////////////////////////////////

  /** Write the whole PfNode tree in PF file format. */
  inline qint64 writePf(QIODevice *target,
                        PfOptions options = PfOptions()) const {
    return d ? d->writePf(target, options) : 0; }
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
  /** Write the node content (without node structure and children tree)
    * with no escape for PF special chars and so on. */
  inline qint64 writeRawContent(QIODevice *target,
                                PfOptions options = PfOptions()) const {
    return d ? d->writeRawContent(target, options) : 0; }
  /** Write the node content (without node structure and children tree)
    * in PF syntax (escaping special chars and adding binary fragment headers).
    */
  inline qint64 writeContentAsPf(QIODevice *target,
                                 PfOptions options = PfOptions()) const {
    return d ? d->writePfContent(target, options) : 0; }
};

#endif // PFNODE_H
