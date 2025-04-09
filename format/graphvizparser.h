/* Copyright 2025 Gregoire Barbier and others.
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
#ifndef GRAPHVIZPARSER_H
#define GRAPHVIZPARSER_H

#include "util/paramsprovidermerger.h"

/** Graphivz dot/gv format parser.
 *  Only a subset is supported, it's enough to parse output of "dot -Tdot" when
 *  there are no subgraphs but it won't handle the format in other cases.
 */
class LIBP6CORESHARED_EXPORT GraphvizParser {
public:
  using GraphCallback = std::function<void(const Utf8String &name,
  ParamsProviderMerger *graphcontext)>;
  using NodeCallback = std::function<void(const Utf8String &name,
  ParamsProviderMerger *nodecontext, ParamsProviderMerger *graphcontext)>;
  using EdgeCallback = std::function<void(const Utf8String &tail,
  const Utf8String &head, ParamsProviderMerger *edgecontext,
  ParamsProviderMerger *graphcontext)>;

private:
  ParamSet _rootdefault, _graphdefault, _nodedefault, _edgedefault;
  ParamsProviderMerger _rootcontext, _graphcontext, _nodecontext, _edgecontext;
  NodeCallback _on_node;
  GraphCallback _on_beg_g, _on_end_g;
  EdgeCallback _on_edge;

public:
  GraphvizParser();
  virtual ~GraphvizParser();
  Utf8String parse(QIODevice *input);
  Utf8String parse(Utf8String input);
  /** called at graph begin, like gvpr's BEG_G */
  inline GraphvizParser &set_begin_graph_callback(GraphCallback callback) {
    _on_beg_g = callback;
    return *this;
  }
  /** called at graph begin, like gvpr's N */
  inline GraphvizParser &set_node_callback(NodeCallback callback) {
    _on_node = callback;
    return *this;
  }
  /** called at graph begin, like gvpr's E */
  inline GraphvizParser &set_edge_callback(EdgeCallback callback) {
    _on_edge = callback;
    return *this;
  }
  /** called at graph begin, like gvpr's END_G */
  inline GraphvizParser &set_end_graph_callback(GraphCallback callback) {
    _on_end_g = callback;
    return *this;
  }
};

#endif // GRAPHVIZPARSER_H
