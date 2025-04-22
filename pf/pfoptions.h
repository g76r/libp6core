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
#ifndef PFOPTIONS_H
#define PFOPTIONS_H

#include "libp6core_global.h"

struct PfOptions {
  enum RootParsingPolicy : quint8 {
    ParseEveryRootNode = 0, StopAfterFirstRootNode, FailAtSecondRootNode,
  };

  enum FragmentsReordering : quint8 {
    NoReordering = 0,
    /// text and binary fragments should be written before children
    PayloadFirst,
    /// children should be written before payload
    ChildrenFirst,
  };

  qint32 _io_timeout_ms:32 = 10'000;
  qint32 _heretext_trigger_size:32 = 1024;
  quint32 _deferred_loading_min_size:32 = 4096;
  quint8 _indent_size:4 = 0; // also implies newlines when > 0
  quint8 _indent_with_tabs:1 = 0;
  quint8 _defer_binary_loading:1 = 0;
  quint8 _allow_bare_binary:1 = 0;
  quint8 _with_comments:1 = 0;
  quint8 _should_cache_deferred_loading:1 = 1;
  RootParsingPolicy _root_parsing_policy:2 = ParseEveryRootNode;
  FragmentsReordering _fragments_reordering: 2 = NoReordering;

  /** 0 = don't wait for bytes when parsing, -1 wait infinitely, > 0 wait that
   *  milliseconds.
   *  is ignored (as if it was 0) when the input IODevice is not sequential.
   *  default: 30'000 ms */
  inline PfOptions with_io_timeout(qint32 io_timeout_ms) const {
    PfOptions options = *this;
    options._io_timeout_ms = io_timeout_ms;
    return options;
  }
  /** decide whether a text fragment should be written as here text depending on
   *  its size or as escaped text (see PfNode::escaped_text()).
   *  -1 never write text as heretext, 0 always. default: 1024 bytes */
  inline PfOptions with_heretext_trigger_size(qint32 size = 1024) const {
    PfOptions options = *this;
    options._heretext_trigger_size = size;
    return options;
  }
  /** min size to defer loading when defer_binary_loading is set to true
   *  default: 4096 */
  inline PfOptions with_deferred_loading_min_size(quint32 size = 4096) const {
    PfOptions options = *this;
    options._deferred_loading_min_size = size;
    return options;
  }
  /** short for with_heretext_trigger_size(-1) */
  inline PfOptions without_heretext_trigger_size() const {
    return with_heretext_trigger_size(-1);
  }
  /** 0 means none, max: 15, default: none
   *  suggested values: with_indent(4) with_indent(2) with_ident(1, true) */
  inline PfOptions with_indent(unsigned size, bool use_tabs = false) const {
    PfOptions options = *this;
    options._indent_size = std::min(size, 15U);
    options._indent_with_tabs = use_tabs;
    return options;
  }
  /** default: false i.e. on parsing every binary fragment will be immediatly
   *  loaded
   *  if true, loading will be defered when possible (for binary fragments with
   *  a bytes count as end marker and no wrappings, if the input is seekable
   *  (i.e. QIODevice::isSequential() == false) size is above
   *  deferred_loading_min_size)
   *  it's probably a good idea to set defer_binary_loading along with
   *  allow_bare_binary if you want to re-read with defered loading a file that
   *  you wrote (because without allow_bare_binary every binary fragment will be
   *  written with wrappings). */
  inline PfOptions with_defer_binary_loading(
      bool defer_binary_loading = true) const {
    PfOptions options = *this;
    options._defer_binary_loading = defer_binary_loading;
    return options;
  }
  /** should deferred loading binary be kept in memory cache after their first
   *  load
   *  otherwise they are discarded and re-read each time the node binary content
   *  is requested
   *  default: true */
  inline PfOptions with_should_cache_deferred_loading(
      bool should_cache = true) const {
    PfOptions options = *this;
    options._should_cache_deferred_loading = should_cache;
    return options;
  }
  /** default: false, force a default wrapping (e.g. base64) when writing a
   *  binary fragment with empty or null wrappings */
  inline PfOptions with_allow_bare_binary(bool allow_bare_binary = true) const {
    PfOptions options = *this;
    options._allow_bare_binary = allow_bare_binary;
    return options;
  }
  /** default: false, ignore them (on parsing and on writing) */
  inline PfOptions with_comments(bool comments = true) const {
    PfOptions options = *this;
    options._with_comments = comments;
    return options;
  }
  /** default: ParseEveryRootNode */
  inline PfOptions with_root_parsing_policy(RootParsingPolicy policy) const {
    PfOptions options = *this;
    options._root_parsing_policy = policy;
    return options;
  }
  /** default: NoReordering */
  inline PfOptions with_fragments_reordering(
      FragmentsReordering reordering) const {
    PfOptions options = *this;
    options._fragments_reordering = reordering;
    return options;
  }
  /** short for with_fragments_reordering(ChildrenFirst) */
  inline PfOptions with_children_first() const {
    return with_fragments_reordering(ChildrenFirst);
  }
  /** short for with_fragments_reordering(PayloadFirst) */
  inline PfOptions with_payload_first() const {
    return with_fragments_reordering(PayloadFirst);
  }
};
static_assert(sizeof(PfOptions) == 16);

#endif // PFOPTIONS_H
