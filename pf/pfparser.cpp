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
#include "pfparser.h"
#include "util/utf8utils.h"
#include<forward_list>
#include <QBuffer>

using enum PfOptions::RootParsingPolicy;

PfAbstractParser::~PfAbstractParser() {
}

namespace {

enum State {
  Toplevel, Comment, WaitForName, Name, WaitForFragment, Text, Wrappings,
  EndMarker, HereText, HereBinary,
};

} // anonynous ns

#define SKIP_WHITESPACE \
  if (!escaped && !quoted && PfNode::is_pf_whitespace(c)) \
  continue;
#define HANDLE_QUOTES \
  if (!escaped && !quoted && (c == '"' || c == '\'')) { \
  quoted = c; \
  continue; \
  } \
  if (!escaped && quoted && c == quoted) { \
  quoted = 0; \
  continue; \
  }
#define ERROR(err) \
  [[unlikely]] return (err)+(line ? " on line "_u8+Utf8String::number(line) \
    +" column "_u8+Utf8String::number(column)+" byte "_u8 \
    +Utf8String::number(pos) : ""_u8)
#define ON_TEXT \
  content.clean(); \
  if (!content.isEmpty()) \
  if (auto err = on_text(content); !!err) \
  ERROR(err);

Utf8String PfAbstractParser::parse(
    const Utf8String &input, const PfOptions &options) {
  auto copy{input};
  QBuffer buf(&copy);
  buf.open(QIODevice::ReadOnly);
  return parse(&buf, options.with_io_timeout(0)
               .with_defer_binary_loading(false));
}

Utf8String PfAbstractParser::parse(
    QIODevice *input, const PfOptions &original_options) {
  PfOptions options = original_options;
  if (input->isSequential()) {
    // can't use deferred loading on e.g. network sockets
    options._defer_binary_loading = false;
  } else {
    // waiting for bytes is useless on seekable devices
    options._io_timeout_ms = 0;
  }
  pos = 0;
  line = 1;
  column = 0;
  if (auto err = on_document_begin(options); !!err)
    ERROR(err);
  State state = Toplevel, next_state = Toplevel;
  char c = 0, quoted = 0;
  Utf8String content, wrappings, endmarker;
  std::forward_list<Utf8String> names;
  bool had_already_seen_a_root_node = false;
  while (true) {
    int escaped = 0;
    bool on_newline = (c == '\n'); // previous char was a \n
read_escaped_char:
    if (auto width = p6::read_byte(input, &c, options._io_timeout_ms);
        width <= 0 || c == 0) {
      if (state == Toplevel || names.empty()) {
        if (c == 0) { // c == 0 at eof and on regular '\0', stop on both
          if (pos == 0)
            ERROR("unexpected empty file"_u8);
          goto end_of_document;
        }
        ERROR(input->errorString() | "read error"_u8);
      }
      ERROR("unexpected end of file"_u8);
    }
    ++pos;
    if (line) { // otherwise we met some data scrambling lines & columns
      if (on_newline) {
        column = 1;
        ++line;
      } else if (!Utf8String::is_utf8_continuation_byte(c)) {
        ++column;
      }
    }
    if (!escaped && c == '\\' && quoted != '\'' && state != Comment) {
      escaped = 1;
      goto read_escaped_char;
    }
    if (escaped == 1) {
      switch (c) {
        case 'a':
          c = '\a';
          break;
        case 'b':
          c = '\b';
          break;
        case 'e':
          c = '\x1b';
          break;
        case 'f':
          c = '\f';
          break;
        case 'n':
          c = '\n';
          break;
        case 'r':
          c = '\r';
          break;
        case 't':
          c = '\t';
          break;
        case 'v':
          c = '\v';
          break;
        case '0':
          c = '\0';
          break;
        case 'x':
        case 'u':
        case 'U':
          // LATER support "\xnn" "\unnnn" "\Unnnnnnnn"
          qWarning() << "PfParser encountered a \\"_u8+c
                        +" escape sequence, which is not yet supported";
          break;
        default: // everything else (including backslash) is left escaped as is
          break;
      }
    }
    switch (state) {
      case Toplevel: {
          SKIP_WHITESPACE;
          HANDLE_QUOTES;
          if (!escaped && !quoted && c == '#') {
begin_of_bumping_comment:
            next_state = state;
begin_of_comment:
            state = Comment;
            content.clear();
            continue;
          }
          if (!escaped && !quoted && c == '(') {
            state = WaitForName;
            continue;
          }
          ERROR("unexpected char at toplevel: '"
                +Utf8String::cEscaped(c)+"'");
        }
      case Comment: {
          if (c == '\n') {
            if (options._with_comments) {
              content.clean();
              if (auto err = on_comment(content); !!err)
                ERROR(err);
            }
            state = next_state;
            content.clear();
            continue;
          }
          content += c;
          continue;
        }
      case WaitForName: {
          SKIP_WHITESPACE;
          HANDLE_QUOTES;
          if (!escaped && !quoted && c == '#')
            goto begin_of_bumping_comment;
          content.clear();
          state = Name;
          [[fallthrough]]; // keep c and pass it to next state
        }
      case Name: {
          HANDLE_QUOTES;
          if (!escaped && !quoted
              && (PfNode::is_pf_whitespace(c) || c == '#' || c == '('
                  || c == ')' || c == '|')) {
            if (names.empty()) {
              if (had_already_seen_a_root_node) {
                if (options._root_parsing_policy == FailAtSecondRootNode)
                  ERROR("forbidden second root node"_u8);
              } else
                had_already_seen_a_root_node = true;
            }
            content.clean();
            names.push_front(content);
            if (auto err = on_node_begin(names); !!err)
              ERROR(err);
            state = WaitForFragment; // for whitespace
          }
          if (!escaped && !quoted && c == '#') {
            next_state = WaitForFragment;
            goto begin_of_comment;
          }
          if (!escaped && !quoted && c == '(') {
begin_of_subnode:
            content.clear();
            state = Name;
            continue;
          }
          if (!escaped && !quoted && c == ')') {
end_of_node:
            if (auto err = on_node_end(names); !!err)
              ERROR(err);
            names.pop_front();
            if (options._root_parsing_policy == StopAfterFirstRootNode
                && names.empty())
              goto end_of_document;
            state = names.empty() ? Toplevel : WaitForFragment;
            continue;
          }
          if (!escaped && !quoted && c == '|') {
begin_of_wrappings:
            wrappings.clear();
            state = Wrappings;
            continue;
          }
          content += c;
          continue;
        }
      case WaitForFragment: {
          SKIP_WHITESPACE;
          HANDLE_QUOTES;
          content.clear();
          state = Text;
          [[fallthrough]]; // keep c and pass it to next state
        }
      case Text: {
          HANDLE_QUOTES;
          if (!escaped && !quoted && PfNode::is_pf_whitespace(c)) {
            ON_TEXT;
            state = WaitForFragment;
            continue;
          }
          if (!escaped && !quoted && c == '#') {
            ON_TEXT;
            goto begin_of_bumping_comment;
          }
          if (!escaped && !quoted && c == '(') {
            ON_TEXT;
            goto begin_of_subnode;
          }
          if (!escaped && !quoted && c == ')') {
            ON_TEXT;
            goto end_of_node;
          }
          if (!escaped && !quoted && c == '|')
            goto begin_of_wrappings;
          content += c;
          continue;
        }
      case Wrappings: {
          if (escaped)
            ERROR("backslash not allowed in wrappings");
          if (PfNode::is_pf_reserved_char(c) && c != '|')
            ERROR("character not allowed in wrappings: '"
                  +Utf8String::cEscaped(c)+"'");
          if (c == '|') {
            endmarker.clear();
            state = EndMarker;
            continue;
          }
          wrappings += c;
          continue;
        }
      case EndMarker: {
          if (escaped)
            ERROR("escape not allowed in end marker");
          if (PfNode::is_pf_reserved_char(c) && c != '\n')
            ERROR("character not allowed in end marker: '"
                  +Utf8String::cEscaped(c)+"'");
          if (c == '\n') {
            wrappings.clean();
            endmarker.clean();
            if (endmarker.isEmpty())
              ERROR("invalid empty end marker");
            bool ok;
            auto len = endmarker.toLongLong<false,false>(&ok, 10, 0);
            if (ok) { // end marker is a valid base 10 integer
              wrappings = PfNode::normalized_wrappings(wrappings);
              if (wrappings.isEmpty() && options._defer_binary_loading
                  && options._deferred_loading_min_size <= len) {
                if (!input->seek(input->pos()+len))
                  // in many cases seek won't return false and the error will
                  // only occur at next read so a unexpected end of file will
                  // be issued instead of teh following error
                  ERROR("not enough bytes for binary fragment: expected "_u8
                        +Utf8String::number(len));
                pos += len;
                line = 0; // any further line & column numbers are wrong
                if (auto err = on_deferred_binary(
                      input, input->pos()-len, len,
                      options._should_cache_deferred_loading); !!err)
                  ERROR(err);
              } else {
                content.clear();
                while (input->bytesAvailable() < len) {
                  // LATER manage _io_timeout_ms on total wait time
                  // now it's applied on each iteration, which is wrong
                  if (options._io_timeout_ms
                      && !input->waitForReadyRead(options._io_timeout_ms))
                    break;
                }
                content = input->read(len);
                if (content.size() != len)
                  ERROR("i/o timed out or not enough bytes, expected "_u8
                        +Utf8String::number(len)+" got "_u8
                        +Utf8String::number(content.size()));
                PfNode::unwrap_binary(&content, wrappings, options);
                pos += len;
                line = 0; // any further line & column numbers are wrong
                if (content.size() > 0)
                  if (auto err = on_loaded_binary(content, wrappings); !!err)
                    ERROR(err);
              }
              state = WaitForFragment;
              continue;
            } else {
              if (wrappings.isEmpty()) {
                state = HereText;
                content.clear();
                continue;
              } else {
                state = HereBinary;
                content.clear();
                continue;
              }
            }
          }
          endmarker += c;
          continue;
        }
      case HereBinary: {
          content += c;
          if (content.endsWith(endmarker)) {
            content.chop(endmarker.size());
            wrappings = PfNode::normalized_wrappings(wrappings);
            if (wrappings.isEmpty() && options._defer_binary_loading
                && options._deferred_loading_min_size <= content.size()) {
              if (auto err = on_deferred_binary(
                    input, input->pos()-content.size(), content.size(),
                    options._should_cache_deferred_loading); !!err)
                ERROR(err);
            } else {
              PfNode::unwrap_binary(&content, wrappings, options);
              if (auto err = on_loaded_binary(content, wrappings); !!err)
                ERROR(err);
            }
            state = WaitForFragment;
          }
          continue;
        }
      case HereText: {
          content += c;
          if (content.endsWith(endmarker)) {
            content.chop(endmarker.size());
            ON_TEXT;
            state = WaitForFragment;
          }
          continue;
        }
    }
  }
end_of_document:
  if (auto err = on_document_end(options); !!err)
    ERROR(err);
  return {};
}

Utf8String PfAbstractParser::on_document_begin(const PfOptions &) {
  return {};
}

Utf8String PfAbstractParser::on_node_begin(std::forward_list<Utf8String> &) {
  return {};
}

Utf8String PfAbstractParser::on_text(const Utf8String &) {
  return {};
}

Utf8String PfAbstractParser::on_loaded_binary(
    const QByteArray &, const Utf8String &) {
  return {};
}

Utf8String PfAbstractParser::on_deferred_binary(
    QIODevice*, qsizetype, qsizetype, bool) {
  return {};
}

Utf8String PfAbstractParser::on_comment(const Utf8String &) {
  return {};
}

Utf8String PfAbstractParser::on_node_end(std::forward_list<Utf8String> &) {
  return {};
}

Utf8String PfAbstractParser::on_document_end(const PfOptions &) {
  return {};
}

PfParser::~PfParser() {
  qDeleteAll(_nodes);
}

void PfParser::clear() {
  _root = {"$root"};
  qDeleteAll(_nodes);
  _nodes.clear();
}

Utf8String PfParser::on_document_begin(const PfOptions &) {
  clear();
  return {};
}

Utf8String PfParser::on_node_begin(std::forward_list<Utf8String> &names) {
  auto node = new PfNode(names.front());
  if (line)
    node->set_pos(line, column);
  _nodes.push_front(node);
  return {};
}

Utf8String PfParser::on_text(const Utf8String &text) {
  auto item = _nodes.front();
  if (!item) // should never happen
    [[unlikely]] return "PfItemBuilder::on_text() called without "
                        "PfItemBuilder::on_node_begin()";
  item->append_text_fragment(text);
  return {};
}

Utf8String PfParser::on_loaded_binary(
    const QByteArray &unwrapped_payload, const Utf8String &wrappings) {
  auto item = _nodes.front();
  if (!item) // should never happen
    [[unlikely]] return "PfItemBuilder::on_loaded_binary() called without "
                        "PfItemBuilder::on_node_begin()";
  item->append_loaded_binary_fragment(unwrapped_payload, wrappings);
  return {};
}

Utf8String PfParser::on_deferred_binary(
    QIODevice *file, qsizetype pos, qsizetype len, bool should_cache) {
  auto item = _nodes.front();
  if (!item) // should never happen
    [[unlikely]] return "PfItemBuilder::on_deferred_binary() called without "
                        "PfItemBuilder::on_node_begin()";
  item->append_deferred_binary_fragment(file, pos, len, should_cache);
  return {};
}

Utf8String PfParser::on_comment(const Utf8String &comment) {
  auto item = _nodes.front();
  if (item)
    item->append_comment_fragment(comment);
  else
    _root.append_comment_fragment(comment);
  return {};
}

Utf8String PfParser::on_node_end(std::forward_list<Utf8String> &) {
  auto node = _nodes.front();
  if (!node)
    [[unlikely]] return "PfItemBuilder::on_node_end() called without "
                        "PfItemBuilder::on_node_begin()";
  _nodes.pop_front();
  if (_nodes.empty())
    _root.append_child(std::move(*node));
  else
    _nodes.front()->append_child(std::move(*node));
  delete node;
  return {};
}

Utf8String PfParser::on_document_end(const PfOptions &) {
  if (!_nodes.empty()) // should never happen
    [[unlikely]] return "PfItemBuilder::on_document_end with unterminated node";
  return {};
}
