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

#ifndef PFDOMHANDLER_H
#define PFDOMHANDLER_H

#include "pfhandler.h"
#include "pfnode.h"

/** Handler for loading the whole PF document into memory (except lazy-loaded
  * binary fragments).
  * This class is usefull for manipulating PF content without bothering with
  * event-oriented parsing: all the data is loaded into memory (excepted binary
  * fragments if they are lazy-loaded) and can be manipuled as PfNode trees. */
class LIBP6CORESHARED_EXPORT PfDomHandler : public PfHandler {
protected:
  QList<PfNode> _path;
  QList<PfNode> _roots;

public:
  // LATER add option to enable (or disable) loading of arrays as children
  /** @param ignoreComments if set, won't receive comment() calls */
  PfDomHandler();
  ~PfDomHandler();
  bool startDocument(const PfOptions &options) override;
  bool startNode(const QStringList &names) override;
  bool text(const QString &text) override;
  bool binary(QIODevice *device, qint64 length, qint64 offset,
              const QString &surface) override;
  bool binary(const QByteArray &data, const QString &surface) override;
  bool array(const PfArray &array) override;
  bool endNode(const QStringList &names) override;
  bool comment(const QString &content) override;
  bool endDocument() override;
  QList<PfNode> roots() const { return _roots; }
  void clear() { _path.clear(); _roots.clear(); }
};

#endif // PFDOMHANDLER_H
