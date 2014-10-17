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

#include "pfutils.h"
#include "pfinternals.h"

QString PfUtils::escape(QString string, bool escapeEvenSingleSpaces) {
  QString s;
  int imax = string.size()-1;
  for (int i = 0; i <= imax; ++i) {
    QChar c = string.at(i);
    if (pfisspecial(c.toLatin1())
        && (escapeEvenSingleSpaces || c != ' ' || i == 0 || i == imax
            || string.at(i+1) == ' '))
      s.append(PF_ESCAPE);
    s.append(c);
  }
  return s;
}
