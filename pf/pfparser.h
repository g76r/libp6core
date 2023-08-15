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

#ifndef PFPARSER_H
#define PFPARSER_H

#include "pfoptions.h"

class QIODevice;
class QByteArray;
class PfHandler;
class PfArray;

/** Parenthesis Format (PF) event-oriented (SAX-like) parser.
  * @see PfHandler
  * @see PfDomHandler */
class LIBP6CORESHARED_EXPORT PfParser {
  PfHandler *_handler;

public:
  PfParser() : _handler(0) { }
  PfParser(PfHandler *handler) : _handler(handler) { }
  void setHandler(PfHandler *handler) { _handler = handler; }
  PfHandler *handler() { return _handler; }
  // LATER provide a paused/continue mechanism as QXmlSimpleReader does
  /** Parse from a QIODevice source, such as a QFile or a QTcpSocket.
    * Support both sequential and seekable devices, but lazy-loading of
    * binary fragments will be disabled on sequential devices. */
  bool parse(QIODevice *source, const PfOptions &options = PfOptions());
  /** Parse from a QByteArray source.
    * The current implementation does not support lazy loading (i.e. every
    * binary fragment will be copied from the original QByteArray and thus
    * will consume about twice the original memory).
    * Therefore the current implementation is only suitable for small
    * PF documents without binary fragments or if the source array is a
    * temporary array and will be deleted soon after parsing. */
  bool parse(QByteArray source, const PfOptions &options = PfOptions());
  /** Open a file and call parse(QIODevice*,PfOptions). */
  bool parse(const QString &pathOrUrl, const PfOptions &options = PfOptions());

private:
  inline bool readAndFinishBinaryFragment(
      QIODevice *source, bool *lazyBinaryFragments, const QString &surface,
      qint64 l, const PfOptions &options);
};

#endif // PFPARSER_H
