#include "pfcontent.h"
#include "pfinternals.h"
#include "util/ioutils.h"
#include <QtDebug>
#include <QRegExp>
#include "pfnode.h"

inline QString PfFragmentData::takeFirstLayer(QString &surface) {
  QRegExp head("^([^:]*)(:|$).*");
  if (!head.exactMatch(surface))
    return surface = QString();
  QString first = head.cap(1);
  surface.remove(0, first.size()+1);
  return first;
}

inline bool PfFragmentData::removeSurface(QByteArray &data,
                                          QString surface) const {
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

inline bool PfFragmentData::applySurface(QByteArray &data,
                                         QString surface) const {
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

inline qint64 PfFragmentData::measureSurface(QByteArray data,
                                           QString surface) const {
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

void PfFragmentData::setSurface(QString surface, bool shouldAdjustSize) {
  _surface = PfOptions::normalizeSurface(surface);
  if (shouldAdjustSize && !_surface.isNull()) {
    QByteArray data = _data;
    if (data.isNull()) {
      qint64 pos = _device->pos();
      if (!_device->seek(_offset))
        goto error;
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
    }
    _size = measureSurface(data, _surface);
  }
}

qint64 PfFragmentData::write(QIODevice *target, Format format,
                             const PfOptions options) const {
  if (isEmpty())
    return 0;
  if (isText()) {
    switch (format) {
    case Raw:
      return target->write(_text.toUtf8());
    case Pf:
      return target->write(pfescape(_text).toUtf8());
    case XmlBase64:
      return target->write(pftoxmltext(_text).toUtf8());
    }
    return -1;
  }
  QString surface = options.outputSurface();
  if (surface.isNull())
    surface = _surface;
  QByteArray data = _data;
  qint64 total = 0, pos = _device ? _device->pos() : 0;
  QString step = "begin(surface)";
  // handling surfaces
  if (!_surface.isNull() || !surface.isNull()) {
    // surface cannot (yet?) be applyed to lazy-loaded fragments, therefore they
    // must be loaded in memory now
    if (data.isNull()) {
      if (!_device->seek(_offset))
        goto error;
      step = "seeked(surface)";
      data = _device->read(_length);
      if (_length != data.size())
        goto error;
      _device->seek(pos);
    }
    step = "read(surface)";
    // decoding input surface, i.e. surface of in-memory or lazy-loaded document
    if (!removeSurface(data, _surface))
      return -1;
    // for PF, encoding output surface, i.e. surface of document being written
    if (format == Pf && !applySurface(data, surface))
      return -1;
  }
  if (data.isNull()){
    // unsurfaced lazy-loaded binary
    step = "begin(lazy)";
    if (!_device)
      return -1;
    qint64 r, remaining = _length;
    if (!_device->seek(_offset))
      goto error;
    step = "seeked(lazy)";
    if (format == Pf) {
      QString header = QString("|%1|%2\n").arg(_surface).arg(_length);
      if ((r = target->write(header.toUtf8())) < 0)
        goto error;
      total += r;
    }
    step = "headerwritten(lazy)";
    char buf[65536];
    while (remaining) {
      r = _device->read(buf, std::min(remaining, qint64(sizeof(buf))));
      if (r < 0)
        goto error;
      step = "read(lazy)";
      if (format == XmlBase64) {
        r = target->write(QByteArray(buf, r).toBase64());
      } else
        r = target->write(buf, r);
      if (r < 0)
        goto error;
      step = "written(lazy)";
      total += r;
      remaining -= r;
    }
    step = "datawritten(lazy)";
    if (!_device->seek(pos))
      goto error;
  } else {
    // in-memory binary or surfaced lazy-loaded binary
    step = "begin(in-memory)";
    qint64 total = 0, r;
    if (format == Pf) {
      QString header = QString("|%1|%2\n").arg(surface).arg(data.size());
      if ((r = target->write(header.toUtf8())) < 0)
        goto error;
      total += r;
    }
    step = "headerwritten(in-memory)";
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
    step = "datawritten(in-memory)";
  }
  return total;
error:
  qDebug() << "PfFragment::write error" << step << total
           << (_device ? _device->errorString() : "")
           << target->errorString();
  if (_device)
    _device->seek(pos);
  return -1;
}

qint64 PfContent::writePf(QIODevice *target, const PfOptions options) const {
  if (!d->_array.isNull()) {
    if (options.shouldTranslateArrayIntoTree()) {
      PfNode tmp;
      d->_array.convertToChildrenTree(&tmp);
      qint64 total = 0, r;
      foreach (PfNode child, tmp.children()) {
        r = child.writePf(target, options);
        if (r == -1)
          return -1;
        total += r;
      }
      return total;
    } else
      return d->_array.writePf(target, options);
  }
  qint64 total = 0, r;
  foreach (const PfFragment f, d->_fragments) {
    r = f.writePf(target, options);
    if (r < 0)
      return -1;
    total += r;
  }
  return total;
}

qint64 PfContent::writeRaw(QIODevice *target, const PfOptions options) const {
  if (!d->_array.isNull())
    return d->_array.writePf(target, options);
  qint64 total = 0, r;
  foreach (const PfFragment f, d->_fragments) {
    r = f.writeRaw(target, options);
    if (r < 0)
      return -1;
    total += r;
  }
  return total;
}

qint64 PfContent::writeXmlUsingBase64(QIODevice *target,
                                      const PfOptions options) const {
  if (!d->_array.isNull()) {
    if (options.shouldTranslateArrayIntoTree()) {
      PfNode tmp;
      d->_array.convertToChildrenTree(&tmp);
      qint64 total = 0, r;
      foreach (PfNode child, tmp.children()) {
        r = child.writeFlatXml(target, options);
        if (r == -1)
          return -1;
        total += r;
      }
      return total;
    } else
      return d->_array.writeTrTd(target, true, options);
  }
  qint64 total = 0, r;
  foreach (const PfFragment f, d->_fragments) {
    r = f.writeXmlUsingBase64(target, options);
    if (r < 0)
      return -1;
    total += r;
  }
  return total;
}

QByteArray PfContent::toByteArray() const {
  QBuffer buf;
  buf.open(QIODevice::WriteOnly);
  writeRaw(&buf, PfOptions());
  return buf.data();
}
