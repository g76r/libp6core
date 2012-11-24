#ifndef PFCONTENT_H
#define PFCONTENT_H

#include <QIODevice>
#include <QList>
#include <QBuffer>
#include <QString>
#include <QVariant>
#include <QSharedData>
#include "pfarray.h"

/** Internal class for Qt's implicit sharing idiom.
  * @see PfFragment
  */
class PfFragmentData : public QSharedData {
  friend class PfFragment;
private:
  enum Format {Raw, Pf, XmlBase64 };

  QString _text;
  qint64 _size;
  mutable QIODevice *_device;
  qint64 _length;
  qint64 _offset;
  QByteArray _data;
  QString _surface;

  explicit inline PfFragmentData(const QString &text = QString())
    : _text(text.isNull() ? "" : text), _size(_text.toUtf8().size()),
      _device(0), _length(0), _offset(0) { }
  inline PfFragmentData(QIODevice *device, qint64 length, qint64 offset,
                        const QString surface) : _size(0), _device(device),
    _length(length), _offset(offset) { setSurface(surface, true); }
  inline PfFragmentData(const QByteArray data, const QString surface)
    : _size(0), _device(0), _length(data.length()), _offset(0), _data(data) {
    setSurface(surface, true); }
  inline PfFragmentData(const PfFragmentData &other) : QSharedData(),
    _text(other._text), _size(other._size), _device(other._device),
    _length(other._length), _offset(other._offset), _data(other._data),
    _surface(other._surface) { }
  qint64 write(QIODevice *target, Format format, const PfOptions options) const;
  inline bool isText() const { return !_text.isNull(); }
  inline bool isEmpty() const { return isText() && _text.isEmpty(); }
  inline bool isBinary() const { return !isText(); }
  inline bool isLazyBinary() const { return isBinary() && _data.isNull(); }
  void setSurface(QString surface, bool shouldAdjustSize);
  inline qint64 measureSurface(QByteArray data, QString surface) const;
  inline bool removeSurface(QByteArray &data, QString surface) const;
  inline bool applySurface(QByteArray &data, QString surface) const;
  inline static QString takeFirstLayer(QString &surface);
};

/** Fragment of PF node content, this class is only for internal use of
  * implementation, mainly PfContent. It should not be used directly by
  * application code.
  */
class PfFragment {
private:
  QSharedDataPointer<PfFragmentData> d;

public:
  inline PfFragment() : d(new PfFragmentData()) { }
  explicit inline PfFragment(const QString &text)
    : d(new PfFragmentData(text)) { }
  inline PfFragment(QIODevice *device, qint64 length, qint64 offset,
                    const QString surface)
    : d(new PfFragmentData(device, length, offset, surface)) { }
  inline PfFragment(const QByteArray data, const QString surface)
    : d(new PfFragmentData(data, surface)) { }
  inline PfFragment(const PfFragment &other) : d(other.d) { }
  /* a fragment is either text or binary; a binary fragment can be lazy or not.
   * there is no difference between a null or empty fragment
   */
  inline bool isEmpty() const { return d->isEmpty(); }
  inline bool isText() const { return d->isText(); }
  inline bool isBinary() const { return d->isBinary(); }
  inline bool isLazyBinary() const { return d->isLazyBinary(); }
  /** binary size (for text: size of text in UTF-8)
    */
  inline qint64 size() const { return d->_size; }
  /** .isNull() if binary fragment
    */
  inline const QString text() const { return d->_text; }
  /** Write content as PF-escaped string or binary with header.
    */
  inline qint64 writePf(QIODevice *target,
                        const PfOptions options) const {
    return d->write(target, PfFragmentData::Pf, options);
  }
  /** Write actual content in unescaped format.
    */
  inline qint64 writeRaw(QIODevice *target, const PfOptions options) const {
    return d->write(target, PfFragmentData::Raw, options);
  }
  /** Write content as XML string, using base64 encoding for binary fragments
    */
  inline qint64 writeXmlUsingBase64(QIODevice *target,
                                    const PfOptions options) const {
    return d->write(target, PfFragmentData::XmlBase64, options);
  }
};

/** Internal class for Qt's implicit sharing idiom.
  * @see PfContent
  */
class PfContentData : public QSharedData {
  friend class PfContent;
private:
  QList<PfFragment> _fragments;
  PfArray _array;

public:
  inline PfContentData() { }
  inline PfContentData(const PfContentData &other) : QSharedData(),
    _fragments(other._fragments), _array(other._array) { }
};

/** Content of a PF node.
  * @see PfNode
  */
class PfContent {
  QSharedDataPointer<PfContentData> d;

public:
  inline PfContent() : d(new PfContentData) { }
  inline PfContent(const PfContent &other) : d(other.d) { }
  /** @return true if contains no fragment (neither text nor binary) and no array
    */
  inline bool isEmpty() const { return d->_fragments.size() == 0 && !isArray(); }
  inline bool isArray() const { return !d->_array.isNull(); }
  /** @return true if the content consist only of text data (no binary no array) or is empty
    */
  inline bool isText() const { return !isArray() && !isBinary(); }
  /** @return true if the content is (fully or partly) binary data, false for empty content
    */
  inline bool isBinary() const {
    foreach (const PfFragment f, d->_fragments)
      if (f.isBinary())
        return true;
    return false;
  }
  /** @return QString() if contains binary or array data, and QString("") if
    * isEmpty()
    */
  inline const QString toString() const {
    if (isArray())
      return QString();
    QString s("");
    foreach (const PfFragment f, d->_fragments) {
      if (f.isBinary())
        return QString();
      s.append(f.text());
    }
    return s;
  }
  /** @return null array if !isArray()
    */
  inline const PfArray array() const {
    return d->_array;
  }
  /** Provide the content as a byte array.
    * If there are lazy-loaded binary fragments, they are loaded into memory,
    * in the returned QByteArray but do not keep them cached inside PfContent
    * structures, therefore the memory will be freed when the QByteArray is
    * discarded and if toByteArray() is called again, the data will be
    * loaded again.
    */
  QByteArray toByteArray() const;
  /** Append text content (and remove array if any).
    */
  inline void append(QString text) {
    d->_array.clear();
    // LATER merge fragments if previous one is text
    d->_fragments.append(PfFragment(text));
  }
  /** Append lazy-loaded binary content (and remove array if any).
    */
  inline void append(QIODevice *device, qint64 length, qint64 offset,
                     const QString surface = QString()) {
    d->_array.clear();
    d->_fragments.append(PfFragment(device, length, offset, surface));
  }
  /** Append in-memory binary content (and remove array if any).
    */
  inline void append(const QByteArray data, const QString surface = QString()) {
    d->_array.clear();
    // Merging fragments if previous is in-memory binary is probably a bad idea
    // because it would prevent Qt's implicite sharing to work.
    d->_fragments.append(PfFragment(data, surface));
  }
  /** Replace current content with array.
    */
  inline void set(PfArray array) {
    d->_fragments.clear();
    d->_array = array;
  }
  inline void clear() {
    d->_array.clear();
    d->_fragments.clear();
  }
  /** Write content to target device in PF format (with escape sequences and
    * binary headers).
    */
  qint64 writePf(QIODevice *target, const PfOptions options) const;
  /** Write content to target device in raw data format (no PF escape sequences
    * but actual content).
    */
  qint64 writeRaw(QIODevice *target, const PfOptions options) const;
  /** Write content to target device in XML format, embeding binary fragments
    * using base64 encoding.
    */
  qint64 writeXmlUsingBase64(QIODevice *target, const PfOptions options) const;
};

#endif // PFCONTENT_H
