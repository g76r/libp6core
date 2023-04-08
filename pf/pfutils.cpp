/* Copyright 2012-2023 Hallowyn and others.
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
#include "pfinternals_p.h"
#include <QRegularExpression>

using namespace Qt::Literals::StringLiterals;

static QRegularExpression _whitespace { "\\s+" };

QString PfUtils::escape(const QString &string, const PfOptions &options,
                        bool escapeEvenSingleSpaces) {
  QString s;
  qsizetype imax = string.size()-1;
  PfPreferedCharactersProtection protection
      = options.preferedCharactersProtection();
  bool protectionUsed = false;
  for (int i = 0; i <= imax; ++i) {
    QChar c = string.at(i);
    if (pfisspecial(c.toLatin1())
        && (escapeEvenSingleSpaces || c != ' ' || i == 0 || i == imax
            || string.at(i+1) == ' ')) {
      switch (protection) {
      case PfBackslashProtection:
        s.append(PF_ESCAPE);
        s.append(c);
        break;
      case PfDoubleQuoteProtection:
        if (c == '\\')
          s.append("\\\\");
        else if (c == '"')
          s.append("\\\"");
        else
          s.append(c);
        protectionUsed = true;
        break;
      case PfSimpleQuoteProtection:
        if (c == '\'')
          s.append("'\\''");
        else
          s.append(c);
        protectionUsed = true;
        break;
      }
    } else {
      s.append(c);
    }
  }
  switch (protection) {
  case PfBackslashProtection:
    return s;
  case PfDoubleQuoteProtection:
    return protectionUsed ? "\""+s+"\"" : s;
  case PfSimpleQuoteProtection:
    return protectionUsed ? "'"+s+"'" : s;
  }
  return s; // should never happen
}

qlonglong PfUtils::stringAsLongLong(
  QString s, qlonglong defaultValue, bool *ok) {
  s = s.trimmed();
  auto len = s.size();
  if (len >= 2) {
    switch(s.at(len-1).toLatin1()) {
      case 'k':
        s = s.left(len-1)+"000";
        break;
      case 'M':
      case 'm':
        s = s.left(len-1)+"000000";
        break;
      case 'G':
      case 'b':
        s = s.left(len-1)+"000000000";
        break;
      case 'T':
        s = s.left(len-1)+"000000000000";
        break;
      case 'P':
        s = s.left(len-1)+"000000000000000";
        break;
    }
  }
  bool _ok;
  auto v = s.toLongLong(&_ok, 0);
  if (ok)
    *ok = _ok;
  return _ok ? v : defaultValue;
}

int PfUtils::stringAsInt(QString s, int defaultValue, bool *ok) {
  s = s.trimmed();
  auto len = s.size();
  if (len >= 2) {
    switch(s.at(len-1).toLatin1()) {
      case 'k':
        s = s.left(len-1)+"000";
        break;
      case 'M':
      case 'm':
        s = s.left(len-1)+"000000";
        break;
      case 'G':
      case 'b':
        s = s.left(len-1)+"000000000";
        break;
    }
  }
  bool _ok;
  auto v = s.toInt(&_ok, 0);
  if (ok)
    *ok = _ok;
  return _ok ? v : defaultValue;
}

bool PfUtils::stringAsBool(QStringView s, bool defaultValue, bool *ok) {
  bool b = defaultValue, _ok = true;
  s = s.trimmed();
  if (s.compare("true"_ba, Qt::CaseInsensitive) == 0)
    b = true;
  if (s.compare("false"_ba, Qt::CaseInsensitive) == 0)
    b = false;
  int i = s.toLongLong(&_ok, 0);
  if (_ok)
    b = i != 0;
  if (ok)
    *ok = _ok;
  return b;
}

#ifdef PF_DOUBLE_SUPPORTS_SI_SUFFIXES
static QHash<char,double> _multipliers {
  { 'k', 1e3 },
  { 'M', 1e6 },
  { 'G', 1e9 },
  { 'T', 1e12 },
  { 'P', 1e15 },
  { 'm', 1e-3 },
  { '\xb5', 1e-6 },
  { 'u', 1e-6 },
  { 'n', 1e-9 },
  { 'p', 1e-12 },
  { 'f', 1e-15 },
};
#endif

double PfUtils::stringAsDouble(QStringView s, double defaultValue, bool *ok) {
  bool _ok;
  s = s.trimmed();
#ifdef PF_DOUBLE_SUPPORTS_SI_SUFFIXES
  double mul = 1.0;
  auto len = s.size();
  if (len >= 2) {
    auto c = s.at(len-1).toLatin1();
    if (_multipliers.contains(c)) {
      mul = _multipliers.value(c);
      s.chop(1);
    }
  }
#endif
  auto v = s.toDouble(&_ok);
  if (ok)
    *ok = _ok;
#ifdef PF_DOUBLE_SUPPORTS_SI_SUFFIXES
  return _ok ? v*mul : defaultValue;
#else
  return _ok ? v : defaultValue;
#endif
}

const QStringList PfUtils::stringSplittedOnWhitespace(QStringView v) {
  QString s;
  QStringList l;
  for (qsizetype i = 0; i < v.size(); ++i) {
    const QChar &c = v[i];
    if (c == '\\') {
      if (++i < v.size())
        s.append(v[i]);
      continue;
    }
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      if (!s.isNull())
        l.append(s);
      s = QString();
      continue;
    }
    s.append(c);
  }
  if (!s.isEmpty())
    l.append(s);
  return l;
}

const QStringList PfUtils::stringSplittedOnFirstWhitespace(QStringView v) {
  v = v.trimmed();
  if (v.isEmpty())
    return { };
  int i = v.indexOf(_whitespace);
  if (i <= 0)
    return { v.toString() };
  return { v.left(i).toString(), v.mid(i+1).toString() };
}
