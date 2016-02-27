/* Copyright 2016 Hallowyn and others.
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
#include "pffragment_p.h"
#include <QtDebug>
#include "pfutils.h"
#include "pfinternals_p.h"

QString PfAbstractBinaryFragmentData::takeFirstLayer(
    QString &surface) const {
  QRegExp head("^([^:]*)(:|$).*");
  if (!head.exactMatch(surface))
    return surface = QString();
  QString first = head.cap(1);
  surface.remove(0, first.size()+1);
  return first;
}

bool PfAbstractBinaryFragmentData::removeSurface(
    QByteArray &data, QString surface) const {
  QString layer = takeFirstLayer(surface);
  if (layer.isEmpty() || layer == "null") {
    // do nothing
  } else if (layer == "zlib") {
    QByteArray a = qUncompress((const uchar *)data.constData()+4,
                               data.size()-4);
    if (a.isNull()) {
      qWarning("PF: cannot remove zlib surface");
      return false;
    }
    data = a;
  } else if (layer == "hex") {
    QByteArray a = QByteArray::fromHex(data);
    if (a.isNull()) {
      qWarning("PF: cannot remove hex surface");
      return false;
    }
    data = a;
  } else if (layer == "base64") {
    QByteArray a = QByteArray::fromBase64(data);
    if (a.isNull()) {
      qWarning("PF: cannot remove base64 surface");
      return false;
    }
    data = a;
  } else {
    qWarning() << "PF: cannot remove unknown surface" << layer;
    return false;
  }
  if (!surface.isEmpty())
    return removeSurface(data, surface);
  return true;
}

bool PfAbstractBinaryFragmentData::applySurface(
    QByteArray &data, QString surface) const {
  QString layer = takeFirstLayer(surface);
  if (!surface.isEmpty())
    if (!applySurface(data, surface))
      return false;
  if (layer.isEmpty() || layer == "null") {
    // do nothing
  } else if (layer == "zlib") {
    QByteArray a = qCompress(data);
    if (a.isNull()) {
      qWarning("PF: cannot apply zlib surface");
      return false;
    }
    data = QByteArray("\0\0\0\0", 4).append(a);
  } else if (layer == "hex") {
    data = data.toHex();
  } else if (layer == "base64") {
    data = data.toBase64();
  } else {
    qWarning() << "PF: cannot apply unknown surface" << layer;
    return false;
  }
  return true;
}

qint64 PfAbstractBinaryFragmentData::measureSurface(
    QByteArray data, QString surface) const {
  QString layer = takeFirstLayer(surface);
  if (layer.isEmpty() || layer == "null") {
    // do nothing
  } else if (layer == "zlib") {
    if (surface.isEmpty()) {
      if (data.size() > 8) {
        qint64 l;
        l = (quint64)data.at(0)
            + ((quint64)data.at(1)<<8)
            + ((quint64)data.at(2)<<16)
            + ((quint64)data.at(3)<<24)
            + ((quint64)data.at(4)<<32)
            + ((quint64)data.at(5)<<40)
            + ((quint64)data.at(6)<<48)
            + ((quint64)data.at(7)<<56);
        return l < 0 ? 0 : l;
      } else {
        qWarning("PF: cannot measure zlib surface");
        return 0;
      }
    } else
      return measureSurface(qUncompress((const uchar *)data.constData()+4,
                                        data.size()-4), surface);
  } else if (layer == "hex") {
    if (surface.isEmpty())
      return data.size()/2;
    else
      return measureSurface(QByteArray::fromHex(data), surface);
  } else if (layer == "base64") {
    if (surface.isEmpty()) {
      qint64 l = data.size()*3/4;
      if (data.at(data.size()-2) == '=') {
        --l;
        if (data.at(data.size()-3) == '=')
          --l;
      }
      return l;
    } else
      return measureSurface(QByteArray::fromBase64(data), surface);
  } else {
    qWarning() << "PF: cannot measure unknown surface" << layer;
  }
  return true;
}

void PfBinaryFragmentData::setSurface(
    QString surface, bool shouldAdjustSize) {
  _surface = PfOptions::normalizeSurface(surface);
  if (shouldAdjustSize && !_surface.isNull()) {
    QByteArray data = _data;
    _size = measureSurface(data, _surface);
  }
}

void PfLazyBinaryFragmentData::setSurface(
    QString surface, bool shouldAdjustSize) {
  _surface = PfOptions::normalizeSurface(surface);
  if (shouldAdjustSize && !_surface.isNull()) {
    qint64 pos = _device->pos();
    QByteArray data;
    if (!_device->seek(_offset))
      goto error;
    // LATER avoid loading in memory the full data at a whole
    data = _device->read(_length);
    if (_length != data.size()) {
error:
      qDebug() << "PfFragment::setSurface error (lazy-loaded binary fragment)"
               << _device->errorString();
      _device->seek(pos);
      _size = 0;
      return;
    }
    _device->seek(pos);
    _size = measureSurface(data, _surface);
  }
}

PfFragmentData::~PfFragmentData() {
}

bool PfFragmentData::isText() const {
  return false;
}

QString PfFragmentData::text() const {
  return QString();
}

bool PfFragmentData::isEmpty() const {
  return false;
}

bool PfFragmentData::isBinary() const {
  return false;
}

bool PfFragmentData::isLazyBinary() const {
  return false;
}

qint64 PfTextFragmentData::write(
    QIODevice *target, Format format, PfOptions options) const {
  switch (format) {
  case Raw:
    return target->write(_text.toUtf8());
  case Pf:
    return target->write(PfUtils::escape(_text, options).toUtf8());
  case XmlBase64:
    return target->write(pftoxmltext(_text).toUtf8());
  }
  return -1;
}

bool PfTextFragmentData::isText() const {
  return true;
}

QString PfTextFragmentData::text() const {
  return _text;
}

bool PfAbstractBinaryFragmentData::isBinary() const {
  return true;
}

qint64 PfBinaryFragmentData::write(
    QIODevice *target, Format format, PfOptions options) const {
  return writeDataApplyingSurface(target, format, options, _data);
}

qint64 PfLazyBinaryFragmentData::write(
    QIODevice *target, Format format, PfOptions options) const {
  qint64 total = 0, pos = 0;
  if (!_device || !target)
    goto error;
  pos = _device->pos();
  if (!_surface.isEmpty()) {
    // surface cannot (yet?) be applyed to lazy-loaded fragments, therefore they
    // must be loaded in memory now
    QByteArray data;
    if (!_device->seek(_offset))
      goto error;
    data = _device->read(_length);
    if (_length != data.size())
      goto error;
    _device->seek(pos);
    return writeDataApplyingSurface(target, format, options, data);
  } else {
    // unsurfaced lazy-loaded binary
    if (!_device)
      return -1;
    qint64 r, remaining = _length;
    if (!_device->seek(_offset))
      goto error;
    if (format == Pf) {
      QString header = QString("|%1|%2\n").arg(_surface).arg(_length);
      if ((r = target->write(header.toUtf8())) < 0)
        goto error;
      total += r;
    }
    char buf[65536];
    while (remaining) {
      r = _device->read(buf, std::min(remaining, qint64(sizeof(buf))));
      if (r < 0)
        goto error;
      if (format == XmlBase64) {
        r = target->write(QByteArray(buf, r).toBase64());
      } else
        r = target->write(buf, r);
      if (r < 0)
        goto error;
      total += r;
      remaining -= r;
    }
    if (!_device->seek(pos))
      goto error;
  }
  return total;

error:
  qDebug() << "PfFragment::write() error: target device error:"
           << (target ? target->errorString() : "device is null")
           << " read (lazy) device error: "
           << (_device ? _device->errorString() : "device is null")
           << " bytes read so far: " << total;
  if (_device)
    _device->seek(pos);
  return -1;
}

bool PfLazyBinaryFragmentData::isLazyBinary() const {
  return true;
}

qint64 PfAbstractBinaryFragmentData::writeDataApplyingSurface(
    QIODevice *target, Format format, PfOptions options,
    QByteArray data) const {
  QString outputSurface = options.outputSurface();
  if (outputSurface.isNull() && format == Pf)
    outputSurface = _surface; // for PF, default output surface is original one
  // handling surfaces
  if ((!_surface.isNull() || !outputSurface.isNull())
      && _surface != outputSurface) {
    // decoding input surface, i.e. surface of in-memory or lazy-loaded document
    if (!removeSurface(data, _surface))
      return -1;
    // encoding output surface, i.e. surface of document being written
    if (!applySurface(data, outputSurface))
      return -1;
  }

  // writing data
  qint64 total = 0, r;
  if (format == Pf) {
    QString header = QString("|%1|%2\n").arg(outputSurface).arg(data.size());
    if ((r = target->write(header.toUtf8())) < 0)
      goto error;
    total += r;
  }
  if (format == XmlBase64) {
    QByteArray ba(data.toBase64());
    r = target->write(ba);
    if (r != ba.size())
      goto error;
  } else {
    r = target->write(data);
    if (r != data.size())
      goto error;
  }
  total += r;
  return total;

error:
  qDebug() << "PfFragment::write() error: target device error:"
           << (target ? target->errorString() : "device is null")
           << " bytes read so far: " << total << "/" << data.size();
  return -1;
}
