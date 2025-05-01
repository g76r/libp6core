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
#ifndef PFPARSER_H
#define PFPARSER_H

#include "pf/pfnode.h"
#include <forward_list>

/** Base class for PF parser: parses data but do nothing with it.
 *  @see PfParser */
struct LIBP6CORESHARED_EXPORT PfAbstractParser {
protected:
  qsizetype pos = 0, line = 1, column = 1;

public:
  inline PfAbstractParser() {}
  virtual ~PfAbstractParser();
  Utf8String parse(QIODevice *input, const PfOptions &options = {});
  Utf8String parse(const Utf8String &input, const PfOptions &options = {});
  virtual Utf8String on_document_begin(const PfOptions &options);
  /** Event method called each time a node is encountered, before any content
    * events (text() and binary()) and subnodes events.
    * @param names names of nodes path to current node, last item of 'names' is
    *        current node name; first is root node name; always at less 1 name
    *        in the list
    * @return must return false iff an error occur (and optionaly set
    * errorString before) */
  virtual Utf8String on_node_begin(std::forward_list<Utf8String> &names);
  virtual Utf8String on_text(const Utf8String &text);
  virtual Utf8String on_loaded_binary(const QByteArray &unwrapped_payload,
                                      const Utf8String &wrappings);
  virtual Utf8String on_deferred_binary(QIODevice *file, qsizetype pos,
                                        qsizetype len, bool should_cache);
  virtual Utf8String on_comment(const Utf8String &comment);
  /** Event method called at end of node parsing, after any content
    * events (text() and binary()) and subnodes events.
    * @param names names of nodes path to current node, same as with startNode()
    * @return must return false iff an error occur (and optionaly set
    * errorString before) */
  virtual Utf8String on_node_end(std::forward_list<Utf8String> &names);
  virtual Utf8String on_document_end(const PfOptions &options);
};

/** Build a PfNode hierarchy out of PF data. */
struct LIBP6CORESHARED_EXPORT PfParser : PfAbstractParser {
  PfNode _root;
  std::list<PfNode*> _items;

public:
  ~PfParser();
  inline PfNode &root() { return _root; }
  void clear();
  Utf8String on_document_begin(const PfOptions &options) override;
  Utf8String on_node_begin(std::forward_list<Utf8String> &names) override;
  Utf8String on_text(const Utf8String &text) override;
  Utf8String on_loaded_binary(const QByteArray &unwrapped_payload,
                              const Utf8String &wrappings) override;
  Utf8String on_deferred_binary(QIODevice *file, qsizetype pos,
                                qsizetype len, bool should_cache) override;
  Utf8String on_comment(const Utf8String &comment) override;
  Utf8String on_node_end(std::forward_list<Utf8String> &names) override;
  Utf8String on_document_end(const PfOptions &options) override;
};

#endif // PFPARSER_H
