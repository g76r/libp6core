#ifndef PFOPTIONS_H
#define PFOPTIONS_H

#include <QString>

class PfOptions {
private:
  bool _shouldLazyLoadBinaryFragments;
  bool _shouldTranslateArrayIntoTree;
  bool _shouldIndent;
  bool _shouldIgnoreComment;
  QString _outputSurface;
  // LATER maxBinaryFragmentSize (then split them into several fragments)

public:
  inline PfOptions() : _shouldLazyLoadBinaryFragments(false),
    _shouldTranslateArrayIntoTree(false), _shouldIndent(false),
    _shouldIgnoreComment(true) { }
  inline PfOptions(const PfOptions &other)
    : _shouldLazyLoadBinaryFragments(other._shouldLazyLoadBinaryFragments),
      _shouldTranslateArrayIntoTree(other._shouldTranslateArrayIntoTree),
      _shouldIndent(other._shouldIndent),
      _shouldIgnoreComment(other._shouldIgnoreComment),
      _outputSurface(other._outputSurface) {
  }
  /** Parser should enable lazy loading for binary fragments. This option will
    * be ignored when parsing non-seekable sources (e.g. network sockets).
    * Default: false.
    */
  inline bool shouldLazyLoadBinaryFragments() const {
    return _shouldLazyLoadBinaryFragments; }
  inline PfOptions &setShouldLazyLoadBinaryFragments(bool value = true) {
    _shouldLazyLoadBinaryFragments = value; return *this; }
  /** Parser should load array contents as children tree rather than as arrays.
    * Writing methods should write arrays as children tree rahter than as
    * arrays. Default: false.
    */
  inline bool shouldTranslateArrayIntoTree() const {
    return _shouldTranslateArrayIntoTree; }
  inline PfOptions &setShouldTranslateArrayIntoTree(bool value = true) {
    _shouldTranslateArrayIntoTree = value; return *this; }
  /** Writing methods should indent output to make it easier to read by human
    * beings even though it use more space and take (slightly) more time to
    * parse. Default: false.
    */
  inline bool shouldIndent() const { return _shouldIndent; }
  inline PfOptions &setShouldIndent(bool value = true) {
    _shouldIndent = value; return *this; }
  /** Parser should not create comment nodes. Writing methods should not write
    * comments. Default: true.
    */
  inline bool shouldIgnoreComment() const { return _shouldIgnoreComment; }
  inline PfOptions &setShouldIgnoreComment(bool value = true) {
    _shouldIgnoreComment = value; return *this; }
  /** Surface used by writing methods to write for binary fragments. If
    * isNull() then the surface found when parsing will be used when writing
    * back (or no surface if the binary fragment wasn't parsed but created
    * by API. Otherwise force a new surface. If isEmpty() but not isNull()
    * then no surface will be used when writing, whatever surface has been
    * defined when parsing or creating the fragment through API.
    * Default: QString().
    */
  inline QString outputSurface() const { return _outputSurface; }
  inline PfOptions &setOutputSurface(const QString value) {
    _outputSurface = normalizeSurface(value); return *this; }
  /** Normalize a surface string description, e.g. transform ":::null:zlib:hex:"
    * into "zlib:hex".
    * This method is rather intended for internal use by the PF library but it
    * is part of the public API and can be used by any user code.
    */
  static QString normalizeSurface(QString surface);
};

#endif // PFOPTIONS_H
