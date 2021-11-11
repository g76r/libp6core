/* Copyright 2016-2017 Hallowyn and others.
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
#ifndef PFFRAGMENT_P_H
#define PFFRAGMENT_P_H

#include "libqtpf_global.h"
#include <QSharedData>
#include <QIODevice>
#include "pfoptions.h"

/** Internal class for Qt's implicit sharing idiom.
  * @see PfFragment */
class LIBQTPFSHARED_EXPORT PfFragmentData : public QSharedData {
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

class LIBQTPFSHARED_EXPORT PfTextFragmentData : public PfFragmentData {
  friend class PfFragment;
  QString _text;
  explicit PfTextFragmentData(const QString &text) : _text(text) { }
  qint64 write(QIODevice *target, Format format,
               const PfOptions &options) const override;
  bool isText() const override;
  QString text() const override;
};

class LIBQTPFSHARED_EXPORT PfAbstractBinaryFragmentData
    : public PfFragmentData {
  friend class PfFragment;
protected:
  QString _surface;
  qint64 _size; // real data size, surface removed
  bool isBinary() const override;

  PfAbstractBinaryFragmentData() : _size(0) { }
  inline qint64 writeDataApplyingSurface(
      QIODevice *target, Format format, const PfOptions &options,
      QByteArray data) const;
  inline static QString takeFirstLayer(QString &surface);
  inline bool removeSurface(QByteArray &data, QString surface) const;
  inline bool applySurface(QByteArray &data, QString surface) const;
  inline qint64 measureSurface(const QByteArray &data, QString surface) const;
};

class LIBQTPFSHARED_EXPORT PfBinaryFragmentData
    : public PfAbstractBinaryFragmentData {
  friend class PfFragment;
  QByteArray _data;
  PfBinaryFragmentData(const QByteArray &data, const QString &surface)
    : _data(data) { setSurface(surface, true); }
  void setSurface(const QString &surface, bool shouldAdjustSize);
  qint64 write(QIODevice *target, Format format,
               const PfOptions &options) const override;
};

class LIBQTPFSHARED_EXPORT PfLazyBinaryFragmentData
    : public PfAbstractBinaryFragmentData {
  friend class PfFragment;
  mutable QIODevice *_device; // TODO manage ownership of the device
  qint64 _length; // raw data length on device, surface applyied
  qint64 _offset;
  PfLazyBinaryFragmentData(
      QIODevice *device, qint64 length, qint64 offset, const QString &surface)
    : _device(device), _length(length), _offset(offset) {
    setSurface(surface, true); }
  void setSurface(const QString &surface, bool shouldAdjustSize);
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
class LIBQTPFSHARED_EXPORT PfFragment {
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
             const  QString &surface)
    : d(new PfLazyBinaryFragmentData(device, length, offset, surface)) { }
  PfFragment(const QByteArray &data, const QString &surface)
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
