#ifndef PFUTILS_H
#define PFUTILS_H

#include <QString>

class PfUtils {
public:
  /** Return a string with all PF special chars escaped, e.g.
    * foo 'bar      ->      foo\ \'bar
    * foo\\bar      ->      foo\\\\bar
    * "foo"(|       ->      \"foo\"\(\|
    */
  static QString escape(const QString &string);

public:
  inline PfUtils() {}
};

#endif // PFUTILS_H
