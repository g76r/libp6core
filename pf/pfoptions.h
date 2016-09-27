/* Copyright 2012-2016 Hallowyn and others.
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

#ifndef PFOPTIONS_H
#define PFOPTIONS_H

#include "libqtpf_global.h"
#include <QString>
#include <QSharedData>

enum PfPreferedCharactersProtection {
  PfBackslashProtection, PfDoubleQuoteProtection, PfSimpleQuoteProtection
};

enum PfRootNodesParsingPolicy {
  ParseEveryRootNode, StopAfterFirstRootNode, FailAtSecondRootNode
};

class LIBQTPFSHARED_EXPORT PfOptionsData : public QSharedData {
  friend class PfOptions;
  bool _shouldLazyLoadBinaryFragments;
  bool _shouldTranslateArrayIntoTree;
  bool _shouldIndent;
  bool _shouldIgnoreComment;
  bool _shouldWriteContentBeforeSubnodes;
  // LATER maxBinaryFragmentSize (then split them into several fragments)
  QString _outputSurface;
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

class LIBQTPFSHARED_EXPORT PfOptions {
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
  QString outputSurface() const { return d->_outputSurface; }
  PfOptions &setOutputSurface(const QString value) {
    d->_outputSurface = normalizeSurface(value); return *this; }
  /** Normalize a surface string description, e.g. transform ":::null:zlib:hex:"
    * into "zlib:hex".
    * This method is rather intended for internal use by the PF library but it
    * is part of the public API and can be used by any user code. */
  static QString normalizeSurface(QString surface);
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
