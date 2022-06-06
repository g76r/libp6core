/* Copyright 2012-2022 Hallowyn and others.
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

#ifndef PFHANDLER_H
#define PFHANDLER_H

#include "libqtpf_global.h"
#include "pfparser.h"
#include "pfarray.h"

class QIODevice;
class QByteArray;

/** Handler for event-oriented (SAX-like) PF parser.
  * @see PfParser
  * @see PfDomHandler */
class LIBQTPFSHARED_EXPORT PfHandler {
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
