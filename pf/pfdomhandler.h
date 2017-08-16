/* Copyright 2012-2017 Hallowyn and others.
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

#ifndef PFDOMHANDLER_H
#define PFDOMHANDLER_H

#include "libqtpf_global.h"
#include "pfhandler.h"
#include "pfnode.h"
#include <QList>

/** Handler for loading the whole PF document into memory (except lazy-loaded
  * binary fragments).
  * This class is usefull for manipulating PF content without bothering with
  * event-oriented parsing: all the data is loaded into memory (excepted binary
  * fragments if they are lazy-loaded) and can be manipuled as PfNode trees. */
class LIBQTPFSHARED_EXPORT PfDomHandler : public PfHandler {
protected:
  QList<PfNode> _path;
  QList<PfNode> _roots;

public:
  // LATER add option to enable (or disable) loading of arrays as children
  /** @param ignoreComments if set, won't receive comment() calls */
  PfDomHandler();
  ~PfDomHandler();
  bool startDocument(const PfOptions &options) override;
  bool startNode(const QVector<QString> &names) override;
  bool text(const QString &text) override;
  bool binary(QIODevice *device, qint64 length, qint64 offset,
              const QString &surface) override;
  bool binary(const QByteArray &data, const QString &surface) override;
  bool array(const PfArray &array) override;
  bool endNode(const QVector<QString> &names) override;
  bool comment(const QString &content) override;
  bool endDocument() override;
  QList<PfNode> roots() const { return _roots; }
  void clear() { _path.clear(); _roots.clear(); }
};

#endif // PFDOMHANDLER_H
