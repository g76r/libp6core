/* Copyright 2012-2014 Hallowyn and others.
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

public:
  PfOptionsData() : _shouldLazyLoadBinaryFragments(false),
    _shouldTranslateArrayIntoTree(false), _shouldIndent(false),
    _shouldIgnoreComment(true), _shouldWriteContentBeforeSubnodes(false),
    _preferedCharactersProtection(PfDoubleQuoteProtection) {
  }
};

class LIBQTPFSHARED_EXPORT PfOptions {
  QSharedDataPointer<PfOptionsData> d;
public:
  inline PfOptions() : d(new PfOptionsData) { }
  inline PfOptions(const PfOptions &other) : d(other.d) { }
  inline PfOptions &operator=(const PfOptions &other) {
    d = other.d; return *this; }
  /** Parser should enable lazy loading for binary fragments. This option will
    * be ignored when parsing non-seekable sources (e.g. network sockets).
    * Default: false. */
  inline bool shouldLazyLoadBinaryFragments() const {
    return d->_shouldLazyLoadBinaryFragments; }
  inline PfOptions &setShouldLazyLoadBinaryFragments(bool value = true) {
    d->_shouldLazyLoadBinaryFragments = value; return *this; }
  /** Parser should load array contents as children tree rather than as arrays.
    * Writing methods should write arrays as children tree rahter than as
    * arrays. Default: false. */
  inline bool shouldTranslateArrayIntoTree() const {
    return d->_shouldTranslateArrayIntoTree; }
  inline PfOptions &setShouldTranslateArrayIntoTree(bool value = true) {
    d->_shouldTranslateArrayIntoTree = value; return *this; }
  /** Writing methods should indent output to make it easier to read by human
    * beings even though it use more space and take (slightly) more time to
    * parse. Default: false. */
  inline bool shouldIndent() const { return d->_shouldIndent; }
  inline PfOptions &setShouldIndent(bool value = true) {
    d->_shouldIndent = value; return *this; }
  /** Parser should not create comment nodes. Writing methods should not write
    * comments. Default: true. */
  inline bool shouldIgnoreComment() const { return d->_shouldIgnoreComment; }
  inline PfOptions &setShouldIgnoreComment(bool value = true) {
    d->_shouldIgnoreComment = value; return *this; }
  /** Writing methods should write node content before subnodes, which is not
   * done by default because parsing such a document consumes more memory if
   * nodes have large content. */
  inline bool shouldWriteContentBeforeSubnodes() const {
    return d->_shouldWriteContentBeforeSubnodes; }
  inline PfOptions &setShouldWriteContentBeforeSubnodes(bool value = true) {
    d->_shouldWriteContentBeforeSubnodes = value; return *this; }
  /** Surface used by writing methods to write for binary fragments. If
    * isNull() then the surface found when parsing will be used when writing
    * back (or no surface if the binary fragment wasn't parsed but created
    * by API. Otherwise force a new surface. If isEmpty() but not isNull()
    * then no surface will be used when writing, whatever surface has been
    * defined when parsing or creating the fragment through API.
    * Default: QString(). */
  inline QString outputSurface() const { return d->_outputSurface; }
  inline PfOptions &setOutputSurface(const QString value) {
    d->_outputSurface = normalizeSurface(value); return *this; }
  /** Normalize a surface string description, e.g. transform ":::null:zlib:hex:"
    * into "zlib:hex".
    * This method is rather intended for internal use by the PF library but it
    * is part of the public API and can be used by any user code. */
  static QString normalizeSurface(QString surface);
  /** Prefered method to protect special characters.
    * default: PfDoubleQuoteProtection. */
  inline PfPreferedCharactersProtection preferedCharactersProtection() const {
    return d->_preferedCharactersProtection; }
  inline PfOptions &preferBackslashCharactersProtection() {
    d->_preferedCharactersProtection = PfBackslashProtection; return *this; }
  inline PfOptions &preferDoubleQuoteCharactersProtection() {
    d->_preferedCharactersProtection = PfDoubleQuoteProtection; return *this; }
  inline PfOptions &preferSimpleQuoteCharactersProtection() {
    d->_preferedCharactersProtection = PfSimpleQuoteProtection; return *this; }
};

#endif // PFOPTIONS_H
