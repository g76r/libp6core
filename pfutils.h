#ifndef PFUTILS_H
#define PFUTILS_H

#include "libqtpf_global.h"
#include <QString>

class LIBQTPFSHARED_EXPORT PfUtils {
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
