#ifndef PFNODE_H
#define PFNODE_H

#include <QString>
#include <QList>
#include <QIODevice>
#include "pfcontent.h"
#include <QVariant>
#include <QSharedData>
#include "pfoptions.h"

class PfNode;

class PfNodeData : public QSharedData {
  friend class PfNode;
private:
  QString _name;
  QList<PfNode> _children;
  bool _isComment;
  PfContent _content;

public:
  explicit inline PfNodeData(const QString &name = QString()) : _name(name),
    _isComment(false) { staticInit(); }
  inline PfNodeData(const QString &name, const QString &content,
                    bool isComment = false) : _name(name),
    _isComment(isComment) { staticInit(); _content.append(content); }
  inline PfNodeData(const PfNodeData &other) : QSharedData(),
    _name(other._name), _children(other._children),
    _isComment(other._isComment), _content(other._content) { }
  inline bool isNull() const { return _name.isNull(); }
  inline bool isComment() const { return _isComment; }
  qint64 writePf(QIODevice *target, const PfOptions options) const;
  qint64 writeFlatXml(QIODevice *target, const PfOptions options) const;
  //qint64 writeCompatibleXml(QIODevice &target) const;

private:
  //inline void buildChildrenFromArray() const;
  inline qint64 internalWritePf(QIODevice *target, QString indent,
                                const PfOptions options) const;
  static void staticInit();
};

class PfNode {
  friend class PfNodeData;
private:
  QSharedDataPointer<PfNodeData> d;

public:
  explicit inline PfNode(QString name = QString()) : d(new PfNodeData(name)) { }
  inline PfNode(QString name, QString content, bool isComment = false)
    : d(new PfNodeData(name, content, isComment)) { }

  // Node related methods /////////////////////////////////////////////////////

  inline QString name() const { return d->_name; }
  inline bool isNull() const { return d->isNull(); }
  inline bool isComment() const { return d->isComment(); }

  // Children related methods /////////////////////////////////////////////////

  inline const QList<PfNode> children() const { return d->_children; }
  inline void appendChild(PfNode child) { d->_children.append(child); }
  /** Return a child content knowing the child name.
    * QString() if child does not exist or the child content is binary.
    * QString("") if child exists but has no content
    * If several children has the same name the first one is choosen.
    * The goal is to emulate XML attributes, hence the name.
    */
  QString attribute(QString name) const;
  /** Return a child content knowing the child name.
    * defaultValue if child does not exist or the child content is binary.
    * QString("") if child exists but has no content
    * If several children has the same name the first one is choosen.
    * The goal is to emulate XML attributes, hence the name.
    */
  QString attribute(QString name, QString defaultValue) const;
  /** Set a child named 'name' with 'content' content and remove any other
    * child named 'name'.
    */
  void setAttribute(QString name, QString content);
  /** Convenience method
    */
  inline void setAttribute(QString name, QVariant content) {
    setAttribute(name, content.toString());
  }
  /** Disambiguation method (assume content is UTF-8 encoded)
    */
  inline void setAttribute(QString name, const char *content) {
    setAttribute(name, QString::fromUtf8(content));
  }
  /** Construct a list of all children named 'name'.
    */
  const QList<PfNode> childrenByName(QString name) const;
  bool hasChild(QString name) const;
  bool isLeaf() const { return d->_children.size() == 0; }
  void removeAllChildren() { d->_children.clear(); }
  void removeChildrenByName(const QString name);

  // Content related methods //////////////////////////////////////////////////

  /** @return true when there is no content
    */
  inline bool contentIsEmpty() const { return d->_content.isEmpty(); }
  /** @return true if the content consist only of text data (no binary or array) or is empty
    */
  inline bool contentIsText() const { return d->_content.isText(); }
  /** @return true if the content is (fully or partly) binary data, therefore false when empty
    */
  inline bool contentIsBinary() const { return d->_content.isBinary(); }
  /** @return true if the content is an array
    */
  inline bool contentIsArray() const { return d->_content.isArray(); }
  /** @return QString() if contentIsBinary() or contentIsArray(), and QString("") if contentIsEmpty()
    */
  inline QString contentAsString() const { return d->_content.toString(); }
  /** @return QByteArray() if contentIsEmpty() otherwise raw content (no escape for PF special characters)
    */
  inline QByteArray contentAsByteArray() const { return d->_content.toByteArray(); }
  /** @return PfArray() if not contentIsArray()
    */
  PfArray contentAsArray() const { return d->_content.array(); }
  /** Append text fragment to context (and remove array if any).
    */
  inline void appendContent(const QString text) { d->_content.append(text); }
  /** Append text fragment to context (and remove array if any).
    */
  inline void appendContent(const char *utf8text) {
    appendContent(QString::fromUtf8(utf8text)); }
  /** Append in-memory binary fragment to context (and remove array if any).
    */
  inline void appendContent(const QByteArray data,
                            const QString surface = QString()) {
    d->_content.append(data, surface); }
  /** Append lazy-loaded binary fragment to context (and remove array if any).
    */
  inline void appendContent(QIODevice *device, qint64 length, qint64 offset,
                            const QString surface = QString()) {
    d->_content.append(device, length, offset, surface); }
  /** Replace current content with text fragment.
    */
  inline void setContent(const QString text) {
    d->_content.clear(); appendContent(text); }
  /** Replace current content with text fragment.
    */
  inline void setContent(const char *utf8text) {
    setContent(QString::fromUtf8(utf8text)); }
  /** Replace current content with in-memory binary fragment.
    */
  inline void setContent(const QByteArray data) {
    d->_content.clear(); appendContent(data); }
  /** Replace current content with lazy-loaded binary fragment.
    */
  inline void setContent(QIODevice *device, qint64 length, qint64 offset) {
    d->_content.clear(); appendContent(device, length, offset); }
  /** Replace current content with an array.
    */
  inline void setContent(const PfArray array) { d->_content.set(array); }

  // Output methods ///////////////////////////////////////////////////////////

  /** Write the whole PfNode tree in PF file format.
    */
  inline qint64 writePf(QIODevice *target,
                        const PfOptions options = PfOptions()) const {
    return d->writePf(target, options); }
  /** Convert the whole PfNode tree to PF in a byte array.
    */
  QByteArray toPf(const PfOptions options = PfOptions()) const;
  /** Convert the whole PfNode tree to PF in a characters string.
    * Note that the string will be truncated to the first \0 encountered,
    * which may happen inside binary segments, if any, therefore this
    * method is only for debuging or human-readable display, not for
    * data output (which should use writePf() instead).
    */
  inline QString toString() const {
    return QString::fromUtf8(toPf(PfOptions().setShouldIndent()));
  }
  /** Convenience operator to transparently convert into a QString
    */
  /*inline operator QString() const {
    return QString::fromUtf8(toPf(true));
  }*/
  /** Write node and whole tree (children recursively) in flat XML format.
    * Flat XML format is a format without any attribute (every PF node is
    * written as an XML element) and with binary content converted into
    * Base64 text. Encoding is always UTF-8.
    */  
  inline qint64 writeFlatXml(QIODevice *target,
                             const PfOptions options = PfOptions()) const {
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
    * with no escape for PF special chars and so on.
    */
  inline qint64 writeRawContent(QIODevice *target,
                                const PfOptions options = PfOptions()) const {
    return d->_content.writeRaw(target, options); }
  /** Write the node content (without node structure and children tree)
    * in PF syntax (escaping special chars and adding binary fragment headers).
    */
  inline qint64 writeContentAsPf(QIODevice *target,
                                 const PfOptions options = PfOptions()) const {
    return d->_content.writePf(target, options); }
};

#endif // PFNODE_H
