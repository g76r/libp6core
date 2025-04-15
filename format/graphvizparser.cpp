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
#include "graphvizparser.h"
#include "util/utf8utils.h"
#include <QBuffer>

GraphvizParser::GraphvizParser() {
  _rootcontext(&_rootdefault);
  _graphcontext(&_graphdefault)(&_rootdefault);
  _nodecontext(&_nodedefault)(&_rootdefault);
  _edgecontext(&_edgedefault)(&_rootdefault);
}

GraphvizParser::~GraphvizParser() {
}

namespace {

inline bool expect_char(QIODevice *source, char expected) {
  char c = 0;
  do {
    // qDebug() << "GraphvizParser::expect" << (int)c << source->bytesAvailable()
    //          << source->isReadable() << source->isOpen()
    //          << source->waitForReadyRead(-1);
    if (!source->bytesAvailable() && !source->waitForReadyRead(-1))
      return false;
    if (source->read(&c, 1) != 1)
      return false;
  } while (c != expected);
  return true;
}

enum State {
  Toplevel,
  Name1,
  WaitForDash,
  WaitForName2,
  Name2,
  WaitForList,
  WaitForKey,
  Key,
  WaitForEqual,
  WaitForValue,
  Value,
};

} // anonymous ns

Utf8String GraphvizParser::parse(QIODevice *input) {
  // qDebug() << "GraphvizParser::parse" << input;
  if (!expect_char(input, '{')) //  everything before first { is a graph header
    return "can't find starting {";
  if (_on_beg_g)
    _on_beg_g({}, &_rootcontext);
  State state = Toplevel;
  char32_t c = 0, quote = 0;
  Utf8String name1, name2, key, value;
  ParamSet params;
  while (true) {
    bool escaped = false;
read_escaped_char:
    if (auto r = p6::read_utf8<true>(input, &c); r <= 0 || c == 0) {
      if (state == Toplevel || name1.isEmpty()) {
        if (r >= 0)
          goto end_of_graph;
        return input->errorString();
      }
      goto end_of_node_or_edge; // will set state to Toplevel
    }
    if (!escaped && c == '\\') {
      escaped = true;
      goto read_escaped_char;
    }
    // qDebug() << "  iteration" << Utf8String::encode_utf8(c) << state << (int)quote;
    switch (state) {
      case Toplevel: {
          if (!escaped && (Utf8String::is_unicode_whitespace(c) || c == ';'))
            continue;
          if (!escaped && c == '}')
            goto end_of_graph; // ignore any garbage after }
          state = Name1;
          [[fallthrough]]; // keep c and pass it to next state
        }
      case Name1: {
          if (!escaped && !quote && (c == '"' || c == '\'')) {
            quote = c;
            continue;
          }
          if (!escaped && !quote && c == '[') {
            quote = 0;
            state = WaitForKey;
            continue;
          }
          if (!escaped && (c == quote || (!quote && (c == ' ' || c == '\t')))) {
            quote = 0;
            state = WaitForDash;
            continue;
          }
          if (!escaped && !quote && c == '-') {
            quote = 0;
            state = WaitForDash;
            goto waitfordash_dash;
          }
          name1 += c;
          continue;
        }
      case WaitForDash: {
          if (!escaped && (c == ' ' || c == '\t'))
            continue;
          if (!escaped && c == '-') {
waitfordash_dash:
              if (!p6::read_utf8<true>(input, &c))
                return input->errorString();
              if (c != '-' && c != '>')
                return "-- or -> expected between node names of an edge";
              state = WaitForName2;
              continue;
          }
          if (!escaped && (c == '-' || c == ';' || c == '\n' || c == '\r'))
            goto end_of_node_or_edge;
          if (!escaped && c == '[') {
              state = WaitForKey;
              continue;
          }
          state = Name2;
          [[fallthrough]]; // keep c and pass it to next state
        }
      case WaitForName2: {
          if (!escaped && (c == ' ' || c == '\t'))
            continue;
          state = Name2;
          [[fallthrough]]; // keep c and pass it to next state
        }
      case Name2: {
          if (!escaped && !quote && (c == '"' || c == '\'')) {
            quote = c;
            continue;
          }
          if (!escaped && (c == quote || (!quote && (c == ' ' || c == '\t')))) {
            quote = 0;
            state = WaitForList;
            continue;
          }
          if (!escaped && !quote && c == '[') {
            quote = 0;
            state = WaitForKey;
            continue;
          }
          name2 += c;
          continue;
        }
      case WaitForList: {
          if (!escaped && (c == ' ' || c == '\t'))
            continue;
          if (!escaped && (c == ';' || c == '\r' || c == '\n'))
            goto end_of_node_or_edge;
          if (!escaped && c == '[') {
            state = WaitForKey;
            continue;
          }
          return "garbage character before [: "_u8+c;
        }
      case WaitForKey: {
          if (!escaped && (c == ' ' || c == '\t' || c == ',' || c == '\r'
                           || c == '\n'))
            continue;
          if (!escaped && c == ']') {
end_of_node_or_edge:
              // qDebug() << "GraphvizParser::end_of_node_or_edge" << name1
              //          << name2 << params;
              if (name1.isEmpty()) // should never happen
                return "internal parser error at end_of_node_or_edge";
              if (name1 == "node"_u8) {
                _nodedefault.insert(params);
              } else if (name1 == "edge"_u8) {
                _edgedefault.insert(params);
              } else if (name1 == "graph"_u8) {
                _graphdefault.insert(params);
              } else if (name2.isEmpty()) {
                if (_on_node) {
                  _nodecontext.prepend(&params);
                  _on_node(name1, &_nodecontext, &_graphcontext);
                  _nodecontext.pop_front();
                }
              } else {
                if (_on_edge) {
                  _edgecontext.prepend(&params);
                  _on_edge(name1, name2, &_edgecontext, &_graphcontext);
                  _edgecontext.pop_front();
                }
              }
              name1.clear();
              name2.clear();
              params.clear();
              state = Toplevel;
              continue;
          }
          state = Key;
          [[fallthrough]]; // keep c and pass it to next state
        }
      case Key: {
          if (!escaped && !quote && (c == '"' || c == '\'')) {
            // actually graphivz disallow simple quotes for keys (but allows
            // them for values and names)
            quote = c;
            continue;
          }
          if (!escaped && !quote && c == '=') {
            state = WaitForValue;
            continue;
          }
          if (!escaped && (c == quote || (!quote && (c == ' ' || c == '\t')))) {
            quote = 0;
            state = WaitForEqual;
            continue;
          }
          key += c;
          continue;
        }
      case WaitForEqual: {
          if (!escaped && (c == ' ' || c == '\t'))
            continue;
          if (!escaped && c == '=') {
              state = WaitForValue;
              continue;
          }
          return "garbage character before =: "_u8+c;
        }
      case WaitForValue: {
          if (!escaped && (c == ' ' || c == '\t'))
            continue;
          state = Value;
          [[fallthrough]]; // keep c and pass it to next state
        }
      case Value: {
          if (!escaped && !quote && (c == '"' || c == '\'')) {
            quote = c;
            continue;
          }
          if (!escaped && (c == quote
                           || (!quote && (c == ',' || c == ' ' || c == '\t'
                                          || c == ']')))) {
            params.insert(key, value);
            key.clear();
            value.clear();
            quote = 0;
            if (c == ']')
              goto end_of_node_or_edge;
            state = WaitForKey;
            continue;
          }
          value += c;
          continue;
        }
    }
  }
end_of_graph:
  // qDebug() << "GraphvizParser::end_of_graph" << name1
  //          << name2 << params;
  if (_on_end_g)
    _on_end_g({}, &_rootcontext);
  return {};
}

Utf8String GraphvizParser::parse(Utf8String input) {
  QBuffer buf(&input);
  if (!buf.open(QBuffer::ReadOnly))
    return "cannot open QBuffer for reading :-/"; // unlikely to occur
  return parse(&buf);
}
