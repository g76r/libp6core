#ifndef PFPARSER_H
#define PFPARSER_H

#include "pfhandler.h"
#include "pfoptions.h"

class QIODevice;
class QByteArray;
class PfHandler;
class PfArray;

/** Parenthesis Format (PF) event-oriented (SAX-like) parser.
  * @see PfHandler
  * @see PfDomHandler
  */
class PfParser {
private:
  PfHandler *_handler;

public:
  PfParser() : _handler(0) { }
  PfParser(PfHandler *handler) : _handler(handler) { }
  void setHandler(PfHandler *handler) { _handler = handler; }
  PfHandler *handler() { return _handler; }
  // LATER provide a paused/continue mechanism as QXmlSimpleReader does
  /** Parse from a QIODevice source, such as a QFile or a QTcpSocket.
    * Support both sequential and seekable devices, but lazy-loading of
    * binary fragments will be disabled on sequential devices.
    */
  bool parse(QIODevice *source, const PfOptions options = PfOptions());
  /** Parse from a QByteArray source.
    * The current implementation does not support lazy loading (i.e. every
    * binary fragment will be copied from the original QByteArray and thus
    * will consume about twice the original memory).
    * Therefore the current implementation is only suitable for small
    * PF documents without binary fragments or if the source array is a
    * temporary array and will be deleted soon after parsing.
    */
  bool parse(QByteArray source, const PfOptions options = PfOptions());

private:
  inline bool readAndFinishBinaryFragment(QIODevice *source,
                                          bool &lazyBinaryFragments,
                                          const QString surface, qint64 l);
  inline bool finishArray(PfArray &array, QList<QString> &names);
};

#endif // PFPARSER_H
