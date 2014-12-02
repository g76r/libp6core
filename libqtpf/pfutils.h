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

#ifndef PFUTILS_H
#define PFUTILS_H

#include "libqtpf_global.h"
#include <QString>
#include "pfoptions.h"

class LIBQTPFSHARED_EXPORT PfUtils {
public:
  /** Return a string with all PF special chars escaped excepted single spaces
    * in the middle of the string, e.g.
    * foo 'bar      ->      foo \'bar
    *  foo  bar     ->      \ foo\  bar
    * foo\\bar      ->      foo\\\\bar
    * "foo"(|       ->      \"foo\"\(\|
    *
    * Set escapeEvenSingleSpaces to true to escape every spaces, which is useful
    * e.g. for node names containing spaces.
    */
  static QString escape(QString string, PfOptions options,
                        bool escapeEvenSingleSpaces = false);

public:
  inline PfUtils() {}
};

#endif // PFUTILS_H
