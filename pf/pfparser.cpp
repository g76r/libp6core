#include "pfparser.h"
#include <QObject>
#include <QtDebug>
#include "pfinternals.h"
#include <QBuffer>
#include "util/ioutils.h"
#include "pfarray.h"

enum State { TopLevel, Name, Content, Comment, Quote, BinarySurfaceOrLength,
             BinaryLength, ArrayHeader, ArrayBody, Escape, EscapeHex };

static const char hexdigits[] = {
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

bool PfParser::parse(QIODevice *source, const PfOptions options) {
  bool lazyBinaryFragments = options.shouldLazyLoadBinaryFragments();
  if (!_handler) {
    qWarning() << "PfParser::parse called before setting a handler";
    return false;
  }
  _handler->setErrorString(tr("unknown handler error"));
  int line = 1, column = 0, arrayColumn = 0;
  uchar c, quote = 0;
  ushort escaped = 0;
  char digit;
  State state = TopLevel, nextState = TopLevel;
  QByteArray content, comment, surface;
  QList<QString> names;
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
  while (source->waitForReadyRead(30000), source->getChar((char*)&c)) {
    ++column;
    switch(state) {
    case TopLevel:
      if (c == '(') {
        state = Name;
      } else if (pfisnewline(c)) {
        ++line;
        column = 0;
      } else if (pfisspace(c)) {
      } else if (c == '#') {
        state = Comment;
        nextState = TopLevel;
      } else {
        _handler->setErrorString(tr("unexpected character '%1'")
                                 .arg(pfquotechar(c)));
        goto error;
      }
      break;
    case Name:
      if (pfisendofname(c) && content.isEmpty()) {
        _handler->setErrorString(tr("anonymous node"));
        goto error;
      } else if (c == '(') {
        names.append(QString::fromUtf8(content));
        content.clear();
        if (!_handler->startNode(names)) {
          _handler->setErrorString(tr("cannot handle start of node"));
          goto error;
        }
      } else if (c == ')') {
        names.append(QString::fromUtf8(content));
        content.clear();
        if (!_handler->startNode(names) || !_handler->endNode(names)) {
          _handler->setErrorString(tr("cannot handle end of node"));
          goto error;
        }
        names.removeLast();
        state = names.size() ? Content : TopLevel;
      } else if (pfisspace(c)) {
        if (pfisnewline(c)) {
          ++line;
          column = 0;
        }
        names.append(QString::fromUtf8(content));
        content.clear();
        if (!_handler->startNode(names)) {
          _handler->setErrorString(tr("cannot handle start of node"));
          goto error;
        }
        state = Content;
      } else if (c == '#') {
        names.append(QString::fromUtf8(content));
        content.clear();
        if (!_handler->startNode(names)) {
          _handler->setErrorString(tr("cannot handle start of node"));
          goto error;
        }
        state = Comment;
        nextState = Content;
      } else if (c == '|') {
        names.append(QString::fromUtf8(content));
        content.clear();
        if (!_handler->startNode(names)) {
          _handler->setErrorString(tr("cannot handle start of node"));
          goto error;
        }
        state = BinarySurfaceOrLength;
      } else if (pfisquote(c)) {
        quote = c;
        state = Quote;
        nextState = Name;
      } else if (c == '\\') {
        state = Escape;
        nextState = Name;
      } else if (pfisspecial(c)) {
        _handler->setErrorString(tr("unexpected character '%1'")
                                 .arg(pfquotechar(c)));
        goto error;
      } else {
        content.append(c);
      }
      break;
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
        }
        state = Name;
      } else if (c == ')') {
        if (content.size()) {
          if (!_handler->text(QString::fromUtf8(content))) {
            _handler->setErrorString(tr("cannot handle text fragment"));
            goto error;
          }
          content.clear();
        }
        if (!_handler->endNode(names)) {
          _handler->setErrorString(tr("cannot handle end of node"));
          goto error;
        }
        names.removeLast();
        state = names.size() ? Content : TopLevel;
      } else if (pfisspace(c)) {
        if (pfisnewline(c)) {
          ++line;
          column = 0;
        }
        if (content.size()) {
          if (!_handler->text(QString::fromUtf8(content))) {
            _handler->setErrorString(tr("cannot handle text fragment"));
            goto error;
          }
          content.clear();
        }
      } else if (c == '#') {
        if (content.size()) {
          if (!_handler->text(QString::fromUtf8(content))) {
            _handler->setErrorString(tr("cannot handle text fragment"));
            goto error;
          }
          content.clear();
        }
        state = Comment;
        nextState = Content;
      } else if (c == '|') {
        if (content.size()) {
          if (!_handler->text(QString::fromUtf8(content))) {
            _handler->setErrorString(tr("cannot handle text fragment"));
            goto error;
          }
          content.clear();
        }
        state = BinarySurfaceOrLength;
      } else if (pfisquote(c)) {
        quote = c;
        state = Quote;
        nextState = Content;
      } else if (c == '\\') {
        state = Escape;
        nextState = Content;
      } else if (pfisspecial(c)) {
        _handler->setErrorString(tr("unexpected character '%1'")
                                 .arg(pfquotechar(c)));
        goto error;
      } else
        content.append(c);
      break;
      case Comment:
      if (c == '\n') {
        if (!options.shouldIgnoreComment()) {
          if (!_handler->comment(comment)) {
            _handler->setErrorString(tr("cannot handle comment"));
            goto error;
          }
          comment.clear();
          ++line;
          column = 0;
        }
        state = nextState;
      } else {
        if (!options.shouldIgnoreComment())
          comment.append(c);
        ++column;
      }
      break;
    case Quote:
      if (c == quote) {
        state = nextState;
      } else {
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
        if (!readAndFinishBinaryFragment(source, lazyBinaryFragments, "", l))
          goto error;
        content.clear();
        state = Content;
      } else if (c == '|') {
        surface = content;
        content.clear();
        state = BinaryLength;
      } else if (std::isdigit(c) || std::islower(c) || std::isupper(c)
                 || c == ':') {
        content.append(c);
      } else {
        _handler->setErrorString(tr("unexpected character '%1'")
                                 .arg(pfquotechar(c)));
        goto error;
      }
      break;
    case BinaryLength:
      if (c == '\n') {
        if (content.size() == 0) {
          _handler->setErrorString(tr("binary fragment without length"));
          goto error;
        }
        if (!readAndFinishBinaryFragment(source, lazyBinaryFragments, surface,
                                         content.toLongLong()))
          goto error;
        content.clear();
        state = Content;
      } else if (std::isdigit(c)) {
        content.append(c);
      } else {
        _handler->setErrorString(tr("unexpected character '%1'")
                                 .arg(pfquotechar(c)));
        goto error;
      }
      break;
    case ArrayHeader:
      if (c == ';') {
        if (!content.isEmpty()) {
          array.appendHeader(QString::fromUtf8(content));
          content.clear();
        } else
          array.appendHeader(QString::number(arrayColumn));
        ++arrayColumn;
      } else if (c == ')') {
        content.clear();
        if (!finishArray(array, names))
          goto error;
        state = names.size() ? Content : TopLevel;
      } else if (pfisnewline(c)) {
        if (!content.isEmpty()) {
          array.appendHeader(QString::fromUtf8(content));
          content.clear();
        } else
          array.appendHeader(QString::number(arrayColumn));
        ++line;
        column = 0;
        state = ArrayBody;
      } else if (c == '#') {
        if (!content.isEmpty()) {
          array.appendHeader(QString::fromUtf8(content));
          content.clear();
        } else
          array.appendHeader(QString::number(arrayColumn));
        ++column;
        state = Comment;
        nextState = ArrayBody;
      } else if (pfisspace(c)) {
        // ignore
      } else if (pfisquote(c)) {
        quote = c;
        state = Quote;
        nextState = ArrayHeader;
      } else if (c == '\\') {
        state = Escape;
        nextState = ArrayHeader;
      } else if (pfisspecial(c)) {
        _handler->setErrorString(tr("unexpected character '%1'")
                                 .arg(pfquotechar(c)));
        goto error;
      } else {
        content.append(c);
      }
      break;
    case ArrayBody:
      if (c == ';') {
        array.appendCell(QString::fromUtf8(content));
        content.clear();
      } else if (c == ')') {
        if (content.size())
          array.appendCell((QString::fromUtf8(content)));
        array.removeLastRowIfEmpty();
        content.clear();
        if (!finishArray(array, names))
          goto error;
        state = names.size() ? Content : TopLevel;
      } else if (pfisnewline(c)) {
        array.appendCell(QString::fromUtf8(content));
        content.clear();
        array.appendRow();
        ++line;
        column = 0;
      } else if (c == '#') {
        if (content.size())
          array.appendCell((QString::fromUtf8(content)));
        content.clear();
        ++column;
        state = Comment;
        nextState = ArrayBody;
      } else if (pfisspace(c)) {
        // ignore
      } else if (pfisquote(c)) {
        quote = c;
        state = Quote;
        nextState = ArrayBody;
      } else if (c == '\\') {
        state = Escape;
        nextState = ArrayBody;
      } else if (pfisspecial(c)) {
        _handler->setErrorString(tr("unexpected character '%1'")
                                 .arg(pfquotechar(c)));
        goto error;
      } else {
        content.append(c);
      }
      break;
    case Escape:
      if (c == '\n') {
        column = 0;
        ++line;
      } else
        ++column;
      if (c == 'n')
        c = '\n';
      else if (c == 'r')
        c = '\r';
      else if (c == 't')
        c = '\t';
      else if (c == '0')
        c = 0;
      /*else if (c == 'v')
        c = '\v';
      else if (c == 'b')
        c = '\b';
      else if (c == 'f')
        c = '\f';
      else if (c == 'a')
        c = '\a';*/
      else if (c == 'x') {
        state = EscapeHex;
        quote = 4;
        escaped = 0;
        break;
      } else if (c == 'u') {
        state = EscapeHex;
        quote = 12;
        escaped = 0;
        break;
      }
      content.append(c);
      state = nextState;
      break;
    case EscapeHex:
      digit = hexdigits[c];
      if (digit < 0) {
        _handler->setErrorString("bad hexadecimal digit in escape sequence");
        goto error;
      }
      if (quote) {
        escaped |= digit << quote;
        quote -= 4;
      } else {
        if (escaped > 0x7f)
          content.append(QString(QChar(escaped|digit)).toUtf8());
        else
          content.append(QChar(escaped|digit));
        state = nextState;
      }
      break;
    }
  }
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

bool PfParser::parse(QByteArray source, const PfOptions options) {
  QBuffer buf(&source);
  if (!buf.open(QBuffer::ReadOnly))
    return false; // unlikely to occur
  return parse(&buf, options);
}

bool PfParser::readAndFinishBinaryFragment(QIODevice *source,
                                           bool &lazyBinaryFragments,
                                           const QString surface, qint64 l) {
  if (l <= 0)
    return true;
  if (lazyBinaryFragments && source->isSequential()) {
    qDebug() << "lazyBinaryFragments ignored because source is "
                "sequential (= not seekable)";
    lazyBinaryFragments = false;
  }
  if (lazyBinaryFragments) {
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
    IOUtils::copy(buf, source, l);
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

bool PfParser::finishArray(PfArray &array, QList<QString> &names) {
  if (!(_handler->array(array))) {
    array.clear();
    _handler->setErrorString(tr("cannot handle array fragment"));
    return false;
  }
  if (!_handler->endNode(names)) {
    _handler->setErrorString(tr("cannot handle end of node"));
    return false;
  }
  names.removeLast();
  return true;
}
