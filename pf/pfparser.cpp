/* Copyright 2012-2024 Hallowyn, Gregoire Barbier and others.
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

#include "pfparser.h"
#include "pfinternals_p.h"
#include "pfarray.h"
#include "pfhandler.h"
#include <QBuffer>
#include <QFile>
#include <QObject>
#include <QtDebug>

#define tr(x) QObject::tr(x)

namespace {

struct Node {
  QString _name;
  bool _hasContent;
  Node(QString name = QString()) : _name(name), _hasContent(false) { }
};

// LATER would be nice but conflicts w/ unnamed namespace:
// Q_DECLARE_TYPEINFO(Node, Q_RELOCATABLE_TYPE);

enum State { TopLevel, Name, Content, SpaceInContent, Comment, Quote,
             BinarySurfaceOrLength, BinaryLength, ArrayHeader, ArrayBody,
             Escape, EscapeHex };

} // unnamed namespace

static const qint8 hexdigits[] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

static inline QStringList names(QList<Node> nodes) {
  QStringList names(nodes.size());
  for (const Node &node : nodes)
    names.append(node._name);
  return names;
}

static inline bool finishArray(PfHandler *handler, PfArray *array,
                               QList<Node> *nodes) {
  if (!(handler->array(*array))) {
    array->clear();
    handler->setErrorString(tr("cannot handle array fragment"));
    return false;
  }
  if (!handler->endNode(names(*nodes))) {
    handler->setErrorString(tr("cannot handle end of node"));
    return false;
  }
  nodes->removeLast();
  return true;
}

// LATER make read and write timeout parameters
bool PfParser::parse(QIODevice *source, const PfOptions &options) {
  bool lazyBinaryFragments = options.shouldLazyLoadBinaryFragments();
  if (!_handler) {
    qWarning() << "PfParser::parse called before setting a handler";
    return false;
  }
  _handler->setErrorString(tr("unknown handler error"));
  int line = 1, column = 0, arrayColumn = 0;
  char c = 0, quote = 0, escapeshift = 0;
  quint16 escaped = 0;
  qint8 digit;
  State state = TopLevel; // current state
  State quotedState = TopLevel; // saved state for quotes and comments
  State escapedState = TopLevel; // saved state for escapes
  QByteArray content, comment, surface;
  QList<Node> nodes;
  bool firstNode = true;
  PfArray array;
  if (!source->isOpen() && !source->open(QIODevice::ReadOnly)) {
    _handler->setErrorString(tr("cannot open document : %1")
                             .arg(source->errorString()));
    goto error;
  }
  if (!_handler->startDocument(options)) {
    _handler->setErrorString(tr("cannot handle start of document"));
    goto error;
  }
  while (source->bytesAvailable()
         || source->waitForReadyRead(options.readTimeout()),
         source->getChar(&c)) {
    ++column;
    switch(state) {
    case TopLevel:
      if (c == '(') {
        state = Name;
      } else if (c == '\n') {
        ++line;
        column = 0;
      } else if (pfisspace(c)) {
      } else if (c == '#') {
        state = Comment;
        quotedState = TopLevel;
      } else {
        _handler->setErrorString(tr("unexpected character '%1' "
                                    "(in TopLevel state)")
                                 .arg(pfquotechar(c)));
        goto error;
      }
      break;
    case Name:
      if (pfisendofname(c) && content.isEmpty()) {
        _handler->setErrorString(tr("anonymous node"));
        goto error;
      } else if (c == '(') {
        nodes.append(QString::fromUtf8(content));
        content.clear();
        if (!_handler->startNode(names(nodes))) {
          _handler->setErrorString(tr("cannot handle start of node"));
          goto error;
        }
      } else if (c == ')') {
        nodes.append(QString::fromUtf8(content));
        content.clear();
        auto names = ::names(nodes);
        if (!_handler->startNode(names) || !_handler->endNode(names)) {
          _handler->setErrorString(tr("cannot handle end of node"));
          goto error;
        }
        nodes.removeLast();
        state = nodes.isEmpty() ? TopLevel : Content;
        if (nodes.isEmpty()) {
          switch (options.rootNodesParsingPolicy()) {
          case StopAfterFirstRootNode:
            if (firstNode)
              goto stop_parsing;
            break;
          case FailAtSecondRootNode:
            if (!firstNode) {
              _handler->setErrorString(tr("only one root node is allowed "
                                          "(by option)"));
              goto error;
            }
            break;
          case ParseEveryRootNode:
            ;
          }
        }
      } else if (pfisspace(c)) {
        if (c == '\n') {
          ++line;
          column = 0;
        }
        nodes.append(QString::fromUtf8(content));
        content.clear();
        if (!_handler->startNode(names(nodes))) {
          _handler->setErrorString(tr("cannot handle start of node"));
          goto error;
        }
        state = Content;
      } else if (c == '#') {
        nodes.append(QString::fromUtf8(content));
        content.clear();
        if (!_handler->startNode(names(nodes))) {
          _handler->setErrorString(tr("cannot handle start of node"));
          goto error;
        }
        state = Comment;
        quotedState = Content;
      } else if (c == '|') {
        nodes.append(QString::fromUtf8(content));
        content.clear();
        if (!_handler->startNode(names(nodes))) {
          _handler->setErrorString(tr("cannot handle start of node"));
          goto error;
        }
        state = BinarySurfaceOrLength;
      } else if (c == ';') {
        nodes.append(QString::fromUtf8(content));
        array.clear();
        content.clear();
        if (!_handler->startNode(names(nodes))) {
          _handler->setErrorString(tr("cannot handle start of node"));
          goto error;
        }
        array.appendHeader("0");
        arrayColumn = 1;
        state = ArrayHeader;
      } else if (pfisquote(c)) {
        quote = c;
        state = Quote;
        quotedState = Name;
      } else if (c == '\\') {
        state = Escape;
        escapedState = Name;
      } else if (pfisspecial(c)) {
        _handler->setErrorString(tr("unexpected character '%1' (in Name state)")
                                 .arg(pfquotechar(c)));
        goto error;
      } else {
        content.append(c);
      }
      break;
    case SpaceInContent:
      if (pfisspace(c)) {
        if (c == '\n') {
          ++line;
          column = 0;
        } else {
          ++column;
        }
        break;
      }
      // otherwise process as Content by falling into Content: label
    Q_FALLTHROUGH();
    case Content:
      if (c == ';') {
        // LATER warn if an array node has text or binary content
        array.clear();
        if (!content.isEmpty()) {
          array.appendHeader(content);
          content.clear();
        } else
          array.appendHeader("0");
        arrayColumn = 1;
        state = ArrayHeader;
      } else if (c == '(') {
        if (content.size()) {
          if (!_handler->text(QString::fromUtf8(content))) {
            _handler->setErrorString(tr("cannot handle text fragment"));
            goto error;
          }
          content.clear();
          nodes.last()._hasContent = true;
        }
        state = Name;
      } else if (c == ')') {
        if (content.size()) {
          if (!_handler->text(QString::fromUtf8(content))) {
            _handler->setErrorString(tr("cannot handle text fragment"));
            goto error;
          }
          content.clear();
          nodes.last()._hasContent = true;
        }
        if (!_handler->endNode(names(nodes))) {
          _handler->setErrorString(tr("cannot handle end of node"));
          goto error;
        }
        nodes.removeLast();
        state = nodes.isEmpty() ? TopLevel : Content;
        if (nodes.isEmpty()) {
          switch (options.rootNodesParsingPolicy()) {
          case StopAfterFirstRootNode:
            if (firstNode)
              goto stop_parsing;
            break;
          case FailAtSecondRootNode:
            if (!firstNode) {
              _handler->setErrorString(tr("only one root node is allowed "
                                          "(by option)"));
              goto error;
            }
            break;
          case ParseEveryRootNode:
            ;
          }
        }
      } else if (pfisspace(c)) {
        if (c == '\n') {
          ++line;
          column = 0;
        } else {
          ++column;
        }
        state = SpaceInContent;
      } else if (c == '#') {
        if (content.size()) {
          if (!_handler->text(QString::fromUtf8(content))) {
            _handler->setErrorString(tr("cannot handle text fragment"));
            goto error;
          }
          content.clear();
          nodes.last()._hasContent = true;
        }
        state = Comment;
        quotedState = Content;
      } else if (c == '|') {
        if (content.size()) {
          if (!_handler->text(QString::fromUtf8(content))) {
            _handler->setErrorString(tr("cannot handle text fragment"));
            goto error;
          }
          content.clear();
          nodes.last()._hasContent = true;
        }
        state = BinarySurfaceOrLength;
      } else if (pfisquote(c)) {
        if (state == SpaceInContent)
          content.append(' ');
        quote = c;
        state = Quote;
        quotedState = Content;
      } else if (c == '\\') {
        if (state == SpaceInContent)
          content.append(' ');
        state = Escape;
        escapedState = Content;
      } else if (pfisspecial(c)) {
        _handler->setErrorString(tr("unexpected character '%1' "
                                    "(in Content state)")
                                 .arg(pfquotechar(c)));
        goto error;
      } else {
        if (state == SpaceInContent) {
          if (!content.isEmpty() || nodes.last()._hasContent)
            content.append(' ');
          state = Content;
        }
        content.append(c);
      }
      break;
    case Comment:
      if (c == '\n') {
        if (!options.shouldIgnoreComment()) {
          if (!_handler->comment(comment)) {
            _handler->setErrorString(tr("cannot handle comment"));
            goto error;
          }
        }
        comment.clear();
        ++line;
        column = 0;
        state = quotedState;
      } else {
        if (!options.shouldIgnoreComment())
          comment.append(c);
        ++column;
      }
      break;
    case Quote:
      if (c == quote) {
        state = quotedState;
        ++column;
      } else if (c == '\\' && quote == '"') {
        state = Escape;
        escapedState = Quote;
        ++column;
      } else {
        if (c == '\n') {
          ++line;
          column = 0;
        } else
          ++column;
        content.append(c);
      }
      break;
    case BinarySurfaceOrLength:
      if (c == '\n') {
        if (content.size() == 0) {
          _handler->setErrorString(tr("binary fragment without length"));
          goto error;
        }
        bool ok;
        qint64 l = content.toLongLong(&ok);
        if (!ok) {
          _handler->setErrorString(tr("binary fragment with incorrect length"));
          goto error;
        }
        if (!readAndFinishBinaryFragment(source, &lazyBinaryFragments, "", l,
                                         options))
          goto error;
        content.clear();
        nodes.last()._hasContent = true;
        line = 10000000; // LATER hide line numbers after first binary fragment
        column = 0;
        state = Content;
      } else {
        if (std::isspace(c)) {
          // ignore whitespace, incl. \r
        } else if (c == '|') {
          surface = content;
          content.clear();
          state = BinaryLength;
        } else if (std::isdigit(c) || std::islower(c) || std::isupper(c)
                   || c == ':') {
          content.append(c);
        } else {
          _handler->setErrorString(tr("unexpected character '%1' "
                                      "(in BinarySurfaceOrLength state)")
                                   .arg(pfquotechar(c)));
          goto error;
        }
        ++column;
      }
      break;
    case BinaryLength:
      if (c == '\n') {
        if (content.size() == 0) {
          _handler->setErrorString(tr("binary fragment without length"));
          goto error;
        }
        bool ok;
        qint64 l = content.toLongLong(&ok);
        if (!ok) {
          _handler->setErrorString(tr("binary fragment with incorrect length"));
          goto error;
        }
        if (!readAndFinishBinaryFragment(source, &lazyBinaryFragments, surface,
                                         l, options))
          goto error;
        content.clear();
        nodes.last()._hasContent = true;
        line = 10000000; // LATER hide line numbers after first binary fragment
        column = 0;
        state = Content;
      } else {
        if (std::isspace(c)) {
          // ignore whitespace, incl. \r
        } else if (std::isdigit(c)) {
          content.append(c);
        } else {
          _handler->setErrorString(tr("unexpected character '%1' "
                                      "(in BinaryLength state)")
                                   .arg(pfquotechar(c)));
          goto error;
        }
        ++column;
      }
      break;
    case ArrayHeader:
      if (c == '\n') {
        if (!content.isEmpty()) {
          array.appendHeader(QString::fromUtf8(content));
          content.clear();
        } else
          array.appendHeader(QString::number(arrayColumn));
        ++line;
        column = 0;
        state = ArrayBody;
      } else {
        if (c == ';') {
          if (!content.isEmpty()) {
            array.appendHeader(QString::fromUtf8(content));
            content.clear();
          } else
            array.appendHeader(QString::number(arrayColumn));
          ++arrayColumn;
        } else if (c == ')') {
          content.clear();
          if (!finishArray(_handler, &array, &nodes))
            goto error;
          state = nodes.isEmpty() ? TopLevel : Content;
          if (nodes.isEmpty()) {
            switch (options.rootNodesParsingPolicy()) {
            case StopAfterFirstRootNode:
              if (firstNode)
                goto stop_parsing;
              break;
            case FailAtSecondRootNode:
              if (!firstNode) {
                _handler->setErrorString(tr("only one root node is allowed "
                                            "(by option)"));
                goto error;
              }
              break;
            case ParseEveryRootNode:
              ;
            }
          }
        } else if (c == '#') {
          if (!content.isEmpty()) {
            array.appendHeader(QString::fromUtf8(content));
            content.clear();
          } else
            array.appendHeader(QString::number(arrayColumn));
          ++column;
          state = Comment;
          quotedState = ArrayBody;
        } else if (pfisspace(c)) {
          // ignore
        } else if (pfisquote(c)) {
          quote = c;
          state = Quote;
          quotedState = ArrayHeader;
        } else if (c == '\\') {
          state = Escape;
          escapedState = ArrayHeader;
        } else if (pfisspecial(c)) {
          _handler->setErrorString(tr("unexpected character '%1'"
                                      " (in ArrayHeader state)")
                                   .arg(pfquotechar(c)));
          goto error;
        } else {
          content.append(c);
        }
        ++column;
      }
      break;
    case ArrayBody:
      if (c == '\n') {
        array.appendCell(QString::fromUtf8(content));
        content.clear();
        array.appendRow();
        ++line;
        column = 0;
      } else {
        if (c == ';') {
          array.appendCell(QString::fromUtf8(content));
          content.clear();
        } else if (c == ')') {
          if (content.size())
            array.appendCell((QString::fromUtf8(content)));
          array.removeLastRowIfEmpty();
          content.clear();
          if (!finishArray(_handler, &array, &nodes))
            goto error;
          state = nodes.isEmpty() ? TopLevel : Content;
          if (nodes.isEmpty()) {
            switch (options.rootNodesParsingPolicy()) {
            case StopAfterFirstRootNode:
              if (firstNode)
                goto stop_parsing;
              break;
            case FailAtSecondRootNode:
              if (!firstNode) {
                _handler->setErrorString(tr("only one root node is allowed "
                                            "(by option)"));
                goto error;
              }
              break;
            case ParseEveryRootNode:
              ;
            }
          }
        } else if (c == '#') {
          if (content.size())
            array.appendCell((QString::fromUtf8(content)));
          content.clear();
          ++column;
          state = Comment;
          quotedState = ArrayBody;
        } else if (pfisspace(c)) {
          // ignore
        } else if (pfisquote(c)) {
          quote = c;
          state = Quote;
          quotedState = ArrayBody;
        } else if (c == '\\') {
          state = Escape;
          escapedState = ArrayBody;
        } else if (pfisspecial(c)) {
          _handler->setErrorString(tr("unexpected character '%1'"
                                      " (in ArrayBody state)")
                                   .arg(pfquotechar(c)));
          goto error;
        } else {
          content.append(c);
        }
        ++column;
      }
      break;
    case Escape:
      if (c == '\n') {
        column = 0;
        ++line;
      } else {
        if (c == 'n')
          c = '\n';
        else if (c == 'r')
          c = '\r';
        else if (c == 't')
          c = '\t';
        else if (c == '0')
          c = 0;
        else if (c == 'x') {
          state = EscapeHex;
          escapeshift = 4;
          escaped = 0;
          break;
        } else if (c == 'u') {
          state = EscapeHex;
          escapeshift = 12;
          escaped = 0;
          break;
        }
        ++column;
      }
      content.append(c);
      state = escapedState;
      break;
    case EscapeHex:
      digit = hexdigits[static_cast<unsigned char>(c)];
      if (digit < 0) {
        _handler->setErrorString("bad hexadecimal digit in escape sequence");
        goto error;
      }
      if (escapeshift) {
        escaped |= digit << escapeshift;
        escapeshift -= 4;
      } else {
        content.append(QString(QChar(escaped|digit)).toUtf8());
        state = escapedState;
      }
      ++column;
      break;
    }
  }
stop_parsing:
  if (state != TopLevel) {
    _handler->setErrorString(tr("unexpected end of document"));
    goto error;
  }
  if (!_handler->endDocument()) {
    _handler->setErrorString(tr("cannot handle end of document"));
    goto error;
  }
  return true;
error:
  _handler->error(line, column);
  return false;
}

bool PfParser::parse(QByteArray source, const PfOptions &options) {
  QBuffer buf(&source);
  if (!buf.open(QBuffer::ReadOnly))
    return false; // unlikely to occur
  return parse(&buf, options);
}

static qint64 copy(QIODevice *dest, QIODevice *src, qint64 max,
                   qint64 bufsize, int readTimeout,
                   int writeTimeout) {
  if (!dest || !src)
    return -1;
  char buf[bufsize];
  qint64 total = 0;
  while (total < max) {
    qint64 n, m;
    if (src->bytesAvailable() < 1)
      src->waitForReadyRead(readTimeout);
    n = src->read(buf, std::min(bufsize, max-total));
    if (n < 0)
      return -1;
    if (n == 0)
      break;
    m = dest->write(buf, n);
    if (m != n)
      return -1;
    if (dest->bytesToWrite() > bufsize)
      while (dest->waitForBytesWritten(writeTimeout) > bufsize);
    total += n;
  }
  return total;
}

bool PfParser::readAndFinishBinaryFragment(
    QIODevice *source, bool *lazyBinaryFragments, const QString &surface,
    qint64 l, const PfOptions &options) {
  //qDebug() << "readAndFinishBinaryFragment" << lazyBinaryFragments
  //         << surface << l;
  if (l <= 0)
    return true;
  if (*lazyBinaryFragments && source->isSequential()) {
    qDebug() << "lazyBinaryFragments ignored because source is "
                "sequential (= not seekable)";
    *lazyBinaryFragments = false;
  }
  if (*lazyBinaryFragments) {
    qint64 p = source->pos();
    if (!source->seek(p+l)) {
      _handler->setErrorString(tr("binary fragment beyond end of document"));
      return false;
    }
    if (!_handler->binary(source, l, p, surface)) {
      _handler->setErrorString(tr("cannot handle binary fragment"));
      return false;
    }
  } else {
    QByteArray data; //(source.read(l));
    QBuffer buf(&data);
    buf.open(QIODevice::WriteOnly);
    copy(&buf, source, l, 65536, options.readTimeout(), 0);
    if (data.size() != l) {
      _handler->setErrorString(tr("binary fragment beyond end of "
                                  "document (%1 bytes instead of %2)")
                               .arg(data.size()).arg(l));
      return false;
    }
    if (!_handler->binary(data, surface)) {
      _handler->setErrorString(tr("cannot handle binary fragment"));
      return false;
    }
  }
  return true;
}

bool PfParser::parse(const QString &pathOrUrl, const PfOptions &options) {
  QFile file(pathOrUrl);
  if (!_handler) {
    qWarning() << "PfParser::parse called before setting a handler";
    return false;
  }
  if (!file.open(QIODevice::ReadOnly)) {
    _handler->setErrorString(tr("cannot open file: %1").arg(pathOrUrl));
    return false;
  }
  return parse(&file, options);
}
