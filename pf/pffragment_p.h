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
#ifndef PFFRAGMENT_P_H
#define PFFRAGMENT_P_H

#include "pfoptions.h"
#include <QIODevice>

/** Internal class for Qt's implicit sharing idiom.
  * @see PfFragment */
class LIBP6CORESHARED_EXPORT PfFragmentData : public QSharedData {
  friend class PfFragment;
protected:
  enum Format {Raw, Pf, XmlBase64 };
public:
  virtual ~PfFragmentData();
  virtual qint64 write(
      QIODevice *target, Format format, const PfOptions &options) const = 0;
  virtual bool isText() const;
  virtual QString text() const;
  virtual bool isEmpty() const;
  virtual bool isBinary() const;
  virtual bool isLazyBinary() const;
};

class LIBP6CORESHARED_EXPORT PfTextFragmentData : public PfFragmentData {
  friend class PfFragment;
  QString _text;
  explicit PfTextFragmentData(const QString &text) : _text(text) { }
  qint64 write(QIODevice *target, Format format,
               const PfOptions &options) const override;
  bool isText() const override;
  QString text() const override;
};

class LIBP6CORESHARED_EXPORT PfAbstractBinaryFragmentData
    : public PfFragmentData {
  friend class PfFragment;
protected:
  Utf8String _surface;
  qint64 _size; // real data size, surface removed
  bool isBinary() const override;

  PfAbstractBinaryFragmentData() : _size(0) { }
  inline qint64 writeDataApplyingSurface(
      QIODevice *target, Format format, const PfOptions &options,
      QByteArray data) const;
  inline static Utf8String takeFirstLayer(Utf8String &surface);
  inline bool removeSurface(QByteArray &data, Utf8String surface) const;
  inline bool applySurface(QByteArray &data, Utf8String surface) const;
  inline qint64 measureSurface(const QByteArray &data, Utf8String surface) const;
};

class LIBP6CORESHARED_EXPORT PfBinaryFragmentData
    : public PfAbstractBinaryFragmentData {
  friend class PfFragment;
  QByteArray _data;
  PfBinaryFragmentData(const QByteArray &data, const Utf8String &surface)
    : _data(data) { setSurface(surface, true); }
  void setSurface(const Utf8String &surface, bool shouldAdjustSize);
  qint64 write(QIODevice *target, Format format,
               const PfOptions &options) const override;
};

class LIBP6CORESHARED_EXPORT PfLazyBinaryFragmentData
    : public PfAbstractBinaryFragmentData {
  friend class PfFragment;
  mutable QIODevice *_device; // TODO manage ownership of the device
  qint64 _length; // raw data length on device, surface applyied
  qint64 _offset;
  PfLazyBinaryFragmentData(
      QIODevice *device, qint64 length, qint64 offset,
      const Utf8String &surface)
    : _device(device), _length(length), _offset(offset) {
    setSurface(surface, true); }
  void setSurface(const Utf8String &surface, bool shouldAdjustSize);
  qint64 write(QIODevice *target, Format format,
               const PfOptions &options) const override;
  bool isLazyBinary() const override;
};

/** Fragment of PF node content, this class is only for internal use of
  * implementation. It should not be used directly by application code.
  *
  * A fragment is either text or binary or array.
  * A binary fragment can be lazy or not.
  * There is no difference between a null or empty fragment.
  * An empty fragment is a text fragment.
  */
class LIBP6CORESHARED_EXPORT PfFragment {
  friend class PfNodeData;
  friend class PfNode;
  QSharedDataPointer<PfFragmentData> d;

public:
  PfFragment() { }
  PfFragment(const PfFragment &other) : d(other.d) { }
  PfFragment &operator =(const PfFragment &other) { d = other.d; return *this; }

private:
  explicit PfFragment(const QString &text) : d(new PfTextFragmentData(text)) { }
  PfFragment(QIODevice *device, qint64 length, qint64 offset,
             const Utf8String &surface)
    : d(new PfLazyBinaryFragmentData(device, length, offset, surface)) { }
  PfFragment(const QByteArray &data, const Utf8String &surface)
    : d(new PfBinaryFragmentData(data, surface)) { }
  bool isEmpty() const { return d ? d->isEmpty() : true; }
  bool isText() const { return d ? d->isText() : true; }
  bool isBinary() const { return d ? d->isBinary() : false; }
  bool isLazyBinary() const { return d ? d->isLazyBinary() : false; }
  /** .isNull() if binary fragment */
  QString text() const { return d ? d->text() : QString(); }
  /** Write content as PF-escaped string or binary with header. */
  qint64 writePf(QIODevice *target, const PfOptions &options) const {
    return d ? d->write(target, PfFragmentData::Pf, options) : 0;
  }
  /** Write actual content in unescaped format. */
  qint64 writeRaw(QIODevice *target, const PfOptions &options) const {
    return d ? d->write(target, PfFragmentData::Raw, options) : 0;
  }
  /** Write content as XML string, using base64 encoding for binary fragments */
  qint64 writeXmlUsingBase64(QIODevice *target,
                             const PfOptions &options) const {
    return d ? d->write(target, PfFragmentData::XmlBase64, options) : 0;
  }
};

Q_DECLARE_METATYPE(PfFragment)
Q_DECLARE_TYPEINFO(PfFragment, Q_MOVABLE_TYPE);

#endif // PFFRAGMENT_P_H
