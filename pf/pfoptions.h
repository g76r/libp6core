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

#ifndef PFOPTIONS_H
#define PFOPTIONS_H

#include "util/utf8string.h"
#include <QSharedData>

enum PfPreferedCharactersProtection {
  PfBackslashProtection, PfDoubleQuoteProtection, PfSimpleQuoteProtection
};

enum PfRootNodesParsingPolicy {
  ParseEveryRootNode, StopAfterFirstRootNode, FailAtSecondRootNode
};

class LIBP6CORESHARED_EXPORT PfOptionsData : public QSharedData {
  friend class PfOptions;
  bool _shouldLazyLoadBinaryFragments;
  bool _shouldTranslateArrayIntoTree;
  bool _shouldIndent;
  bool _shouldIgnoreComment;
  bool _shouldWriteContentBeforeSubnodes;
  // LATER maxBinaryFragmentSize (then split them into several fragments)
  Utf8String _outputSurface;
  PfPreferedCharactersProtection _preferedCharactersProtection;
  PfRootNodesParsingPolicy _rootNodesParsingPolicy;
  int _readTimeout; // ms

public:
  PfOptionsData() : _shouldLazyLoadBinaryFragments(false),
    _shouldTranslateArrayIntoTree(false), _shouldIndent(false),
    _shouldIgnoreComment(true), _shouldWriteContentBeforeSubnodes(false),
    _preferedCharactersProtection(PfDoubleQuoteProtection),
    _rootNodesParsingPolicy(ParseEveryRootNode),
    _readTimeout(30000) {
  }
};

class LIBP6CORESHARED_EXPORT PfOptions {
  QSharedDataPointer<PfOptionsData> d;
public:
  PfOptions() : d(new PfOptionsData) { }
  PfOptions(const PfOptions &other) : d(other.d) { }
  PfOptions &operator=(const PfOptions &other) {
    d = other.d; return *this; }
  /** Parser should enable lazy loading for binary fragments. This option will
    * be ignored when parsing non-seekable sources (e.g. network sockets).
    * Default: false. */
  bool shouldLazyLoadBinaryFragments() const {
    return d->_shouldLazyLoadBinaryFragments; }
  PfOptions &setShouldLazyLoadBinaryFragments(bool value = true) {
    d->_shouldLazyLoadBinaryFragments = value; return *this; }
  /** Parser should load array contents as children tree rather than as arrays.
    * Writing methods should write arrays as children tree rahter than as
    * arrays. Default: false. */
  bool shouldTranslateArrayIntoTree() const {
    return d->_shouldTranslateArrayIntoTree; }
  PfOptions &setShouldTranslateArrayIntoTree(bool value = true) {
    d->_shouldTranslateArrayIntoTree = value; return *this; }
  /** Writing methods should indent output to make it easier to read by human
    * beings even though it use more space and take (slightly) more time to
    * parse. Default: false. */
  bool shouldIndent() const { return d->_shouldIndent; }
  PfOptions &setShouldIndent(bool value = true) {
    d->_shouldIndent = value; return *this; }
  /** Parser should not create comment nodes. Writing methods should not write
    * comments. Default: true. */
  bool shouldIgnoreComment() const { return d->_shouldIgnoreComment; }
  PfOptions &setShouldIgnoreComment(bool value = true) {
    d->_shouldIgnoreComment = value; return *this; }
  /** Writing methods should write node content before subnodes, which is not
   * done by default because parsing such a document consumes more memory if
   * nodes have large content. */
  bool shouldWriteContentBeforeSubnodes() const {
    return d->_shouldWriteContentBeforeSubnodes; }
  PfOptions &setShouldWriteContentBeforeSubnodes(bool value = true) {
    d->_shouldWriteContentBeforeSubnodes = value; return *this; }
  /** Surface used by writing methods to write for binary fragments. If
    * isNull() then the surface found when parsing will be used when writing
    * back (or no surface if the binary fragment wasn't parsed but created
    * by API. Otherwise force a new surface. If isEmpty() but not isNull()
    * then no surface will be used when writing, whatever surface has been
    * defined when parsing or creating the fragment through API.
    * Default: QString(). */
  inline Utf8String outputSurface() const { return d->_outputSurface; }
  inline PfOptions &setOutputSurface(const Utf8String &value) {
    d->_outputSurface = normalizeSurface(value); return *this; }
  /** Normalize a surface string description, e.g. transform ":::null:zlib:hex:"
    * into "zlib:hex".
    * This method is rather intended for internal use by the PF library but it
    * is part of the public API and can be used by any user code. */
  static Utf8String normalizeSurface(const Utf8String &surface);
  /** Prefered method to protect special characters.
    * default: PfDoubleQuoteProtection. */
  PfPreferedCharactersProtection preferedCharactersProtection() const {
    return d->_preferedCharactersProtection; }
  PfOptions &preferBackslashCharactersProtection() {
    d->_preferedCharactersProtection = PfBackslashProtection; return *this; }
  PfOptions &preferDoubleQuoteCharactersProtection() {
    d->_preferedCharactersProtection = PfDoubleQuoteProtection; return *this; }
  PfOptions &preferSimpleQuoteCharactersProtection() {
    d->_preferedCharactersProtection = PfSimpleQuoteProtection; return *this; }
  /** Root nodes parsing policy.
   * default: ParseEveryRootNode */
  PfRootNodesParsingPolicy rootNodesParsingPolicy() const {
    return d->_rootNodesParsingPolicy; }
  PfOptions &parseEveryRootNode() {
    d->_rootNodesParsingPolicy = ParseEveryRootNode; return *this; }
  PfOptions &stopAfterFirstRootNode() {
    d->_rootNodesParsingPolicy = StopAfterFirstRootNode; return *this; }
  PfOptions &failAtSecondRootNode() {
    d->_rootNodesParsingPolicy = FailAtSecondRootNode; return *this; }
  /** Read timeout used e.g. when parsing a network stream, in milliseconds.
   * default: 30000 (30") */
  int readTimeout() const { return d->_readTimeout; }
  PfOptions &setReadTimeout(int ms) { d->_readTimeout = ms; return *this; }
};

#endif // PFOPTIONS_H
