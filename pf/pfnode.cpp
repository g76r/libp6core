/* Copyright 2012-2025 Hallowyn, Gregoire Barbier and others.
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
#include "pfnode.h"
#include <QBuffer>
#include <QPointer>

PfNode PfNode::_empty;

struct PfNode::Fragment::DeferredBinaryPayload {
  QPointer<QIODevice> _file;
  qsizetype _pos = 0, _len = 0;
  quint8 _should_cache:1 = 0;
  QByteArray _cache = {};
};

struct PfNode::PfWriter : public QIODevice {
  QIODevice *_device;
  char _last = 0;
  qint64 _written = 0;

  inline PfWriter(QIODevice *device) : QIODevice(0), _device(device) {
    setOpenMode(device->openMode());
  }
  inline auto errString() const { return _device->errorString(); }
  inline char last() const { return _last; }
  inline qint64 written() const { return _written; }
  qint64 readData(char*, qint64) override {
    qWarning() << "PfWriter::readData called";
    return -1;// should never happen
  }
  qint64 writeData(const char *data, qint64 len) override {
    auto r = _device->write(data, len);
    if (r != len)
      return -1;
    if (r > 0) {
      _last = data[r-1];
      _written += r;
    }
    return r;
  }
  using QIODevice::write;
  inline qint64 write(char c) {
    return write(&c, 1);
  }
};

namespace {

using Enwrapper = std::function<void(QByteArray *data, const PfOptions &options)>;
using Unwrapper = std::function<void(QByteArray *data, const PfOptions &options)>;
struct Wrapper {
  Enwrapper enwrap;
  Unwrapper unwrap;
};

template<int n = 80>
static inline QByteArray split_every_n_chars(const QByteArray &input) {
  auto s = input.constData(), end = s+input.size(), end1 = s+input.size()/n*n;
  QByteArray output;
  for (; s < end1; s += n)
    output.append(s, n).append('\n');
  if (end == end1)
    output.chop(1);
  else
    output.append(s, end-end1);
  return output;
}

static QMap<Utf8String,Wrapper> _wrappers {
  {"", { {}, {}, } },
  {"null", { {}, {}, } },
  {"hex", {
      [](QByteArray *data, const PfOptions &options) STATIC_LAMBDA {
        if (options._indent_size)
          *data = split_every_n_chars(data->toHex());
        else
          *data = data->toHex();
      },
      [](QByteArray *data, const PfOptions &) STATIC_LAMBDA {
        *data = QByteArray::fromHex(*data);
      },
    } },
  {"base64", {
      [](QByteArray *data, const PfOptions &options) STATIC_LAMBDA {
        if (options._indent_size)
          *data = split_every_n_chars(data->toBase64());
        else
          *data = data->toBase64();
      },
      [](QByteArray *data, const PfOptions &) STATIC_LAMBDA {
        *data = QByteArray::fromBase64(*data);
      },
    } },
  {"zlib", {
      [](QByteArray *data, const PfOptions &) STATIC_LAMBDA {
        *data = qCompress(*data).mid(4);
      },
      [](QByteArray *data, const PfOptions &) STATIC_LAMBDA {
        *data = qUncompress("\0\0\0\0"_ba+*data);
      },
    } },
};

} // anonymous ns

void PfNode::unwrap_binary(QByteArray *wrapped_data, Utf8String &wrappings,
                           const PfOptions &options) {
  for (auto wrapping: wrappings.split(':', Qt::SkipEmptyParts)) {
    auto wrapper = _wrappers.value(wrapping);
    if (wrapper.unwrap)
      wrapper.unwrap(wrapped_data, options);
  }
}

void PfNode::enwrap_binary(QByteArray *unwrapped_data, Utf8String &wrappings,
                           const PfOptions &options) {
  for (auto wrapping: wrappings.split(':', Qt::SkipEmptyParts)
       | std::views::reverse) {
    auto wrapper = _wrappers.value(wrapping);
    if (wrapper.enwrap)
      wrapper.enwrap(unwrapped_data, options);
  }
}

bool PfNode::is_registered_wrapping(const Utf8String &wrapping) {
  return _wrappers.contains(wrapping);
}

Utf8String PfNode::normalized_wrappings(const Utf8String &wrappings) {
  auto list = wrappings.split(':', Qt::KeepEmptyParts);
  list.removeIf([](const QByteArray &wrapping) STATIC_LAMBDA {
    if (wrapping.isEmpty() || wrapping == "null"_ba)
      return true;
    if (!is_registered_wrapping(wrapping)) {
      qWarning() << "unknown wrapping" << wrapping;
      return true;
    }
    return false;
  });
  return list.join(':');
}

Utf8String PfNode::as_pf(const PfOptions &options) const {
  QBuffer buf;
  buf.open(QIODevice::WriteOnly);
  auto w = write_pf(&buf, options);
  if (w < 0)
    return {};
  return buf.data();
}

qint64 PfNode::write_pf(QIODevice *target, const PfOptions &options) const {
  PfWriter writer(target);
  auto r = write_pf(0, &writer, options);
  return r >= 0 ? writer.written() : -1;
}

#define WRITE(data) { if (writer->write(data) == -1) { return -1; } }

static inline Utf8String find_availlable_endmarker(const Utf8String &text) {
  static Utf8String tmpl = "EOF";
  Utf8String endmarker = tmpl;
  for (int i = 0; text.contains(endmarker); ++i)
    endmarker = tmpl+Utf8String::number(i, 36);
  return endmarker;
}

qint64 PfNode::write_pf(size_t depth, PfNode::PfWriter *writer,
                        const PfOptions &options) const {
  Utf8String indent_this = indentation_string(depth, options),
      indent_next = indentation_string(depth+1, options);
  bool inline_node = true;
  WRITE(indent_this);
  WRITE('(');
  WRITE(_name);
  auto list = fragments_as_list();
  switch (options._fragments_reordering) {
    using enum PfOptions::FragmentsReordering;
    case PayloadFirst:
      std::stable_sort(list.begin(), list.end(),
                       [](const Fragment *x, const Fragment *y){
        return x->has_payload() && !y->has_payload();
      });
      break;
    case ChildrenFirst:
      std::stable_sort(list.begin(), list.end(),
                       [](const Fragment *x, const Fragment *y){
        return x->type() == Child && y->type() != Child;
      });
      break;
    case NoReordering:
      ;
  }
  for (const Fragment *last_written = 0; auto f: list) {
    switch (f->type()) {
      using enum Fragment::FragmentType;
      case Text: {
          if (options._indent_size && writer->last() == '\n') {
            WRITE(indent_next);
          } else if (!last_written || last_written->type() == Text)
            WRITE(' ');
          auto text = f->text();
          if (options._heretext_trigger_size >= 0 &&
              text.size() >= options._heretext_trigger_size) {
            auto endmarker = find_availlable_endmarker(text);
            WRITE('|');
            WRITE('|');
            WRITE(endmarker);
            WRITE('\n');
            WRITE(text);
            WRITE(endmarker);
            if (options._indent_size)
              WRITE('\n');
          } else
            WRITE(PfNode::escaped_text(text));
          break;
        }
      case Child: {
          auto child = f->child();
          Q_ASSERT(child);
          if (options._indent_size && writer->last() != '\n')
            WRITE('\n');
          auto r = child->write_pf(depth+1, writer, options);
          if (r < 0)
            return -1;
          inline_node = false; // LATER but if it's inline itself
          break;
        }
      case Comment: {
          if (!options._with_comments)
            goto fragment_skipped;
          if (options._indent_size) {
            if (writer->last() != '\n') // always \n before comment
              WRITE('\n');
            WRITE(indent_next);
          }
          WRITE('#');
          WRITE(f->comment());
          WRITE('\n');
          inline_node = false;
          break;
        }
      case DeferredBinary:
      case LoadedBinary: {
          auto data = f->unwrapped_data();
          auto wrappings = PfNode::normalized_wrappings(f->wrappings());
          if (!options._allow_bare_binary && wrappings.isEmpty())
            wrappings = "base64"_u8;
          PfNode::enwrap_binary(&data, wrappings, options);
          if (options._indent_size && writer->last() == '\n')
            WRITE(indent_next);
          WRITE('|');
          WRITE(wrappings);
          WRITE('|');
          WRITE(Utf8String::number(data.size()));
          WRITE('\n');
          WRITE(data);
          if (options._indent_size && writer->last() == '\n')
            WRITE('\n');
          inline_node = false;
          break;
        }
    }
    last_written = f;
fragment_skipped:;
  }
  if (options._indent_size && !inline_node) {
    // closes parenthesis on next line excepted for inline nodes
    if (writer->last() != '\n')
      WRITE('\n');
    WRITE(indent_this);
  }
  WRITE(')');
  if (options._indent_size)
    WRITE('\n');
  return 0;
}

void PfNode::Fragment::create_deferred_binary(
    const PfNode::Fragment::DeferredBinaryPayload &other) {
  Q_ASSERT(_type == DeferredBinary);
  _nontext._deferredbinary = new PfNode::Fragment::DeferredBinaryPayload(other);
}

void PfNode::Fragment::create_deferred_binary(
    QIODevice *file, qsizetype pos, qsizetype len, bool should_cache) {
  Q_ASSERT(_type == DeferredBinary);
  _nontext._deferredbinary =
      new PfNode::Fragment::DeferredBinaryPayload{file, pos, len, should_cache};
}

void PfNode::Fragment::destroy_deferred_binary() {
  Q_ASSERT(_type == DeferredBinary);
  Q_ASSERT(_nontext._deferredbinary);
  delete _nontext._deferredbinary;
  _nontext._deferredbinary = 0;
}

QByteArray PfNode::Fragment::retrieve_deferred_binary() const {
  static_assert(sizeof(DeferredBinaryPayload) == 64);
  if (_type != DeferredBinary || !_nontext._deferredbinary) {
    qWarning() << "PfNode::Fragment::retrieve_deferred_binary_content() "
                  "called on bad object" << this;
    return {};
  }
  if (_nontext._deferredbinary->_len == 0
      || !_nontext._deferredbinary->_cache.isEmpty())
    return _nontext._deferredbinary->_cache;
  QIODevice *file = _nontext._deferredbinary->_file.data();
  if (!file) {
    qDebug() << "PF deferred binary fragment can't be read because file is no "
                "longer open" << this;
    return {};
  }
  if (!file->seek(_nontext._deferredbinary->_pos)) {
    qDebug() << "PF deferred binary fragment can't seek" << this << "error:"
             << file->errorString();
    return {};
  }
  auto payload = file->read(_nontext._deferredbinary->_len);
  if (payload.size() != _nontext._deferredbinary->_len)
    qDebug() << "PF deferred binary fragment can't read full payload" << this
             << ": read" << payload.size() << "instead of"
             << _nontext._deferredbinary->_len
             << "error:" << file->errorString();
  else if (_nontext._deferredbinary->_should_cache)
    _nontext._deferredbinary->_cache = payload;
  return payload;
}

qsizetype PfNode::Fragment::size_of_deferred_binary() const {
  if (_type != DeferredBinary || !_nontext._deferredbinary) {
    qWarning() << "PfNode::Fragment::size_of_deferred_binary_content() "
                  "called on bad object" << this;
    return 0;
  }
  return _nontext._deferredbinary->_len;
}

Utf8String PfNode::position() const {
  if (!_line)
    return "unknown position"_u8;
  return "line: "_u8 + Utf8String::number(_line)
      + " column: "_u8 + Utf8String::number(_column);
}
