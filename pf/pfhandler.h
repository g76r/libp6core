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

#ifndef PFHANDLER_H
#define PFHANDLER_H

#include "libp6core_global.h"
#include "pfparser.h"
#include "pfarray.h"

class QIODevice;
class QByteArray;

/** Handler for event-oriented (SAX-like) PF parser.
  * @see PfParser
  * @see PfDomHandler */
class LIBP6CORESHARED_EXPORT PfHandler {
  QString _errorString;
  PfOptions _options;
  int _errorLine, _errorColumn;
  bool _errorOccured;

public:
  /** @param ignoreComments should the parser call comment() or not */
  PfHandler();
  virtual ~PfHandler();
  QString errorString() const { return _errorString; }
  void setErrorString(const QString &string) { _errorString = string; }
  int errorLine() const { return _errorLine; }
  int errorColumn() const { return _errorColumn; }
  bool errorOccured() const { return _errorOccured; }
  /** Event method called once at document begining.
    * @return must return false iff an error occur (and optionaly set
    * errorString before) */
  virtual bool startDocument(const PfOptions &options);
  /** Event method called each time a node is encountered, before any content
    * events (text() and binary()) and subnodes events.
    * @param names names of nodes path to current node, last item of 'names' is
    *        current node name; first is root node name; always at less 1 name
    *        in the list
    * @return must return false iff an error occur (and optionaly set
    * errorString before) */
  virtual bool startNode(const QStringList &names);
  /** Event method called each time a text fragment is encountered.
    * @return must return false iff an error occur (and optionaly set
    * errorString before) */
  virtual bool text(const QString &text);
  /** Event method called each time a binary fragment is encountered, if the
    * parser decided that the fragment should be lazy loaded.
    * Note that the handler can decided to load the data immediately, by reading
    * the content of device.
    * The default implementation loads the data into memory and then calls next
    * method, therefore you must reimplement this method to have lazy loading
    * working.
    * @return must return false iff an error occur (and optionaly set
    * errorString before) */
  virtual bool binary(QIODevice *device, qint64 length, qint64 offset,
                      const QString &surface);
  /** Event method called each time a binary fragment is encountered, if the
    * parser decided to load the fragment immediately.
    * @return must return false iff an error occur (and optionaly set
    * errorString before) */
  virtual bool binary(const QByteArray &data, const QString &surface);
  /** Event method called each time an array is encountered.
    * @return must return false iff an error occur (and optionaly set
    * errorString before) */
  virtual bool array(const PfArray &array);
  /** Event method called at end of node parsing, after any content
    * events (text() and binary()) and subnodes events.
    * @param names names of nodes path to current node, same as with startNode()
    * @return must return false iff an error occur (and optionaly set
    * errorString before) */
  virtual bool endNode(const QStringList &names);
  /** Event method called each time a comment is encountered, but if
    * ignoreComments is set to true.
    * @return must return false iff an error occur (and optionaly set
    * errorString before) */
  virtual bool comment(const QString &content);
  /** Event method called once at document ending.
    * @return must return false iff an error occur (and optionaly set
    * errorString before) */
  virtual bool endDocument();
  /** Event method called on error, including when one of the other event method
    * return false.
    * Error message is availlable through errorString(). */
  virtual void error(int line, int column);

protected:
  PfOptions options() const { return _options; }
};

#endif // PFHANDLER_H
