/* Copyright 2012-2016 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PARAMSET_H
#define PARAMSET_H

#include <QSharedData>
#include <QList>
#include <QStringList>
#include <QRegularExpression>
#include "log/log.h"
#include "paramsprovider.h"

class ParamSetData;

/** String key-value parameter set with inheritance, substitution macro-language
 * and syntaxic sugar for converting values to non-string types.
 *
 * Substitution macro-language supports these kinds of variable evaluation:
 * %variable
 * %{variable}
 * %{variable with spaces or special chars ,!?%}
 * %!variable_with_only_one_leading_special_char
 *
 * It also supports functions in the form or special variable names starting
 * with an equal sign and optional parameters separated by arbitrary chars the
 * same way sed commands do (e.g. s/foo/bar/ is the same than s,foo,bar,).
 *
 * Here are supported functions:
 *
 * %=date function: %{=date!format!relativedatetime!timezone}
 *
 * format defaults to pseudo-iso-8601 "yyyy-MM-dd hh:mm:ss,zzz"
 * relativedatetime defaults to current date time
 * timezone defaults to local time, if specified it must follow IANA's timezone
 *   format, see http://www.iana.org/time-zones
 *
 * examples:
 * %=date
 * %{=date!yyyy-MM-dd}
 * %{=date,yyyy-MM-dd}
 * %{=date!!-2days}
 * %{=date!!!UTC}
 * %{=date,,,UTC}
 * %{=date!hh:mm:ss,zzz!01-01T20:02-2w+1d!GMT}
 *
 * %=default function: %{=default!variable1[!variable2[...]][!value_if_not_set]}
 *
 * the function works like %{variable:-value_if_not_set} in shell scripts and
 * almost like nvl/ifnull functions in sql
 * value_if_not_set is evaluated hence %foo is replaced by foo's value
 * variable1..n are of course not evaluated since foo is already replaced by foo's value if foo is set
 *
 * examples:
 * %{=default!foo!null}
 * %{=default!foo!foo not set}
 * %{=default:foo:foo not set!!!}
 * %{=default:foo:bar:neither foo nor bar are set!!!}
 * %{=default!foo!%bar}
 * %{=default!foo}
 *
 * %=rawvalue function: %{=rawvalue!variable[!flags]}
 *
 * the function return unevaluated value of a variable
 * flags is a combination of letters with the following meaning:
 * e %-escape value (in case it will be further %-evaluated)
 * h html encode value
 * u html encode try to detect urls and to transform them into a links
 * n html encode add br whenever it founds a newline
 *
 * example:
 * %{=rawvalue!foo}
 * %{=rawvalue!foo!hun}
 *
 * %=ifneq function: %{=ifneq!input!reference!value_if_not_equal[!value_else]}
 *
 * test inequality of an input and replace it with another
 * depending on the result of the test
 * all parameters are evaluated, hence %foo is replaced by foo's value
 *
 * examples:
 * %{=ifneq:%foo:0:true:false} -> "true" if not null, else "false"
 * %{=ifneq:%foo::notempty} -> "notempty" if not empty, else ""
 * %{=ifneq:%foo::<a href="page?param=%foo>%foo</a>} -> html link if %foo is set
 *
 * %=switch function: %{=switch:value[:case1:value1[:case2:value2[...]]][:default_value]}
 * test an input against different reference values and replace it according to
 * matching case
 * all parameters are evaluated, hence %foo is replaced by foo's value
 * if default_value is not specified, left input as is if no case matches
 *
 * %=switch can be used as would be an %=ifeq function since those two lines
 * are strictly equivalent:
 * %{=switch:value:case1:value1[:default_value]}
 * %{=switch:value:reference:value_if_equal[:value_if_not_equal]}
 *
 * examples:
 * %{=switch:%loglevel:E:error:W:warning:I:info:debug}
 * %{=switch:%foo:0:false:true} -> if 0: false else: true
 * %{=switch:%foo:0:false} -> if 0: false else: %foo
 * %{=switch:%foo} -> always return %foo, but won't warn if foo is not defined
 *
 * %=sub function: %{=sub!input!s-expression!...}
 *
 * input is the data to transform, it is evaluated (%foo become the content of
 *   foo param)
 * s-expression is a substitution expression like those taken by sed's s
 *   command, e.g. /foo/bar/gi or ,.*,,
 *   replacement par is evaluated and both regular params substitution and
 *   regexp substitution are available, e.g. %1 will be replaced by first
 *   capture group and %name will be replaced by named capture group if
 *   availlable or param
 *   regular expression are those supported by Qt's QRegularExpression, which
 *   are almost identical to Perl's regexps
 *
 * examples:
 * %{=sub!foo!/o/O}
 * %{=sub;%foo;/a/b/g;/([a-z]+)[0-9]/%1%bar/g}"
 * %{=sub;2015-04-17;~.*-(?<month>[0-9]+)-.*~%month}
 *
 * %=left function: %{=left:input:length}
 *
 * input is the data to transform, it is evaluated (%foo become the content of
 *   foo param)
 * length is the number of character to keep from the input, if negative or
 *   invalid, the whole input is kept
 *
 * examples:
 * %{=left:%foo:4}
 *
 * %=right function: %{=right:input:length}
 *
 * input is the data to transform, it is evaluated (%foo become the content of
 *   foo param)
 * length is the number of character to keep from the input, if negative or
 *   invalid, the whole input is kept
 *
 * examples:
 * %{=right:%foo:4}
 *
 * %=mid function: %{=mid:input:position[:length]}
 *
 * input is the data to transform, it is evaluated (%foo become the content of
 *   foo param)
 * position is the starting offset, 0 is first character, negatives values mean
 *   0, values larger than the input size will produce an empty output
 * length is the number of character to keep from the input, if negative or
 *   invalid, or omitted, the whole input is kept
 *
 * examples:
 * %{=mid:%foo:4:5}
 * %{=mid:%foo:4}
 *
 * %=htmlencode function: %{=htmlencode:input[:flags]}
 *
 * input is the data to transform, it is evaluated (%foo become the content of
 *   foo param) and can contain the separator character (e.g. :)
 * flags can contain following characters:
 *   u to surround url with links markups
 *   n to add br markup whenever a newline is encountered
 *
 * examples:
 * %{=htmlencode:1 < 2} -> 1 &lt; 2
 * %{=htmlencode:http://wwww.google.com/:u} -> <a href="http://wwww.google.com/">http://wwww.google.com/</a>
 * %{=htmlencode http://wwww.google.com/ u} -> same
 * %{=htmlencode|http://wwww.google.com/} -> http://wwww.google.com/
 *
 * %=elideright,=elideleft,=elidemiddle functions:
 *    %{=elidexxx:input:length[:placeholder]}
 *
 * input is the data to transform, it is evaluated (%foo become the content of
 *   foo param)
 * length is the number of character to keep from the input, if negative or
 *   invalid or smaller than placeholder, the whole input is kept
 * placeholder is the string replacing removed characters, by default "..."
 *
 * examples:
 * %{=elideright:%foo:40}
 * %{=elideright:Hello World !:10} -> Hello W...
 * %{=elideright:Hello World !:10:(...)} -> Hello(...)
 * %{=elideleft:Hello World !:10} -> ...World !
 * %{=elidemiddle:Hello World !:10} -> Hell...d !
 *
 * %=random function: %{=random[:modulo[:shift]]
 *
 * produce a pseudo-random integer number between shift (default: 0) and
 * modulo-1+shift.
 * negative modulos are silently converted to their absolute values.
 *
 * examples:
 * %{=random} -> any integer number (32 or 64 bits, depending on platform)
 * %{=random:100} -> a number between 0 and 99
 * %{=random:6:1} -> a number between 1 and 6
 * %{=random:-8:-4} -> a number between -4 and 3
 */
class LIBQTSSUSHARED_EXPORT ParamSet : public ParamsProvider {
  friend class ParamsProviderMerger;
  QSharedDataPointer<ParamSetData> d;
  static bool _variableNotFoundLoggingEnabled;

public:
  ParamSet();
  ParamSet(const ParamSet &other);
  ParamSet(QHash<QString,QString> params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(QMap<QString,QString> params);
  ~ParamSet();
  ParamSet &operator=(const ParamSet &other);
  ParamSet parent() const;
  void setParent(ParamSet parent);
  void setValue(QString key, QString value);
  void clear();
  void removeValue(QString key);
  /** Return a value without performing parameters substitution.
   * @param inherit should search values in parents if not found */
  QString rawValue(QString key, QString defaultValue = QString(),
                   bool inherit = true) const;
  inline QString rawValue(QString key, const char *defaultValue,
                   bool inherit = true) const {
    return rawValue(key, QString(defaultValue), inherit); }
  inline QString rawValue(QString key, bool inherit) const {
    return rawValue(key, QString(), inherit); }
  /** Return a value after parameters substitution.
   * @param searchInParents should search values in parents if not found */
  inline QString value(QString key, QString defaultValue = QString(),
                       bool inherit = true,
                       const ParamsProvider *context = 0) const {
    return evaluate(rawValue(key, defaultValue, inherit), inherit, context); }
  inline QString value(QString key, const char *defaultValue,
                       bool inherit = true,
                       const ParamsProvider *context = 0) const {
    return evaluate(rawValue(key, QString(defaultValue), inherit), inherit,
                    context); }
  inline QString value(QString key, const char *defaultValue,
                       const ParamsProvider *context) const {
    return evaluate(rawValue(key, QString(defaultValue), true), true,
                    context); }
  inline QString value(QString key, bool inherit,
                       const ParamsProvider *context = 0) const {
    return evaluate(rawValue(key, QString(), inherit), inherit, context); }
  inline QString value(QString key, const ParamsProvider *context) const {
    return evaluate(rawValue(key, QString(), true), true, context); }
  inline QString value(QString key, bool inherit, const ParamsProvider *context,
                       QSet<QString> alreadyEvaluated) const {
    return evaluate(rawValue(key, inherit), inherit, context, alreadyEvaluated);
  }
  /** Return a value splitted into strings after parameters substitution. */
  QStringList valueAsStrings(QString key, QString separator = " ",
                             bool inherit = true,
                             const ParamsProvider *context = 0) const {
    return splitAndEvaluate(rawValue(key), separator, inherit, context); }
  /** Return a value splitted at first whitespace. Both strings are trimmed.
   * E.g. a raw value of "  foo    bar baz  " is returned as a
   * QPair<>("foo", "bar baz"). */
  QPair<QString,QString> valueAsStringsPair(
      QString key, bool inherit = true,
      const ParamsProvider *context = 0) const;
  /** Syntaxic sugar. */
  inline qlonglong valueAsLong(QString key, qlonglong defaultValue = 0,
                               bool inherit = true,
                               const ParamsProvider *context = 0) const {
    bool ok;
    qlonglong v = evaluate(rawValue(key, QString(), inherit),  inherit,
                           context).toLongLong(&ok);
    return ok ? v : defaultValue; }
  /** Syntaxic sugar. */
  inline int valueAsInt(QString key, int defaultValue = 0, bool inherit = true,
                        const ParamsProvider *context = 0) const {
    bool ok;
    int v = evaluate(rawValue(key, QString(), inherit), inherit,
                     context).toInt(&ok);
    return ok ? v : defaultValue; }
  /** Syntaxic sugar. */
  inline double valueAsDouble(QString key, double defaultValue = 0,
                              bool inherit = true,
                              const ParamsProvider *context = 0) const {
    bool ok;
    double  v = evaluate(rawValue(key, QString(), inherit), inherit,
                         context).toLongLong(&ok);
    return ok ? v : defaultValue; }
  /** "false" and "0" are interpreted as false, "true" and any non null
   * valid integer number are interpreted as true. Spaces and case are
   * ignored. */
  inline bool valueAsBool(QString key, bool defaultValue = false,
                          bool inherit = true,
                          const ParamsProvider *context = 0) const {
    QString v = evaluate(rawValue(key, QString(), inherit), inherit, context)
        .trimmed().toLower();
    if (v == "true")
      return true;
    if (v == "false")
      return false;
    bool ok;
    int i = v.toInt(&ok);
    if (ok)
      return i == 0 ? false : true;
    return defaultValue;
  }
  /** Return all keys for which the ParamSet or one of its parents hold a value.
    */
  QSet<QString> keys(bool inherit = true) const;
  /** Return true if key is set. */
  bool contains(QString key, bool inherit = true) const;
  /** Perform parameters substitution within the string. */
  QString evaluate(QString rawValue, bool inherit = true,
                   const ParamsProvider *context = 0) const {
    return evaluate(rawValue, inherit, context, QSet<QString>()); }
  QString evaluate(QString rawValue, const ParamsProvider *context) const {
    return evaluate(rawValue, true, context); }
  QString evaluate(QString rawValue, bool inherit,
                   const ParamsProvider *context,
                   QSet<QString> alreadyEvaluated) const;
  /** Split string and perform parameters substitution.
    * Raw value is split in parts separated by any character in separator
    * string, several separators are processed as only one (hence splitted parts
    * cannot be empty) and leading or trailing separators are ignored.
    * Separators, and any other character, can be escaped with backslash (\),
    * therefore backslashes must be backslashed. */
  QStringList splitAndEvaluate(
      QString rawValue, QString separator = " ", bool inherit = true,
      const ParamsProvider *context = 0) const {
    return splitAndEvaluate(rawValue, separator, inherit, context,
                            QSet<QString>());
  }
  QStringList splitAndEvaluate(QString rawValue,
                               const ParamsProvider *context) const {
    return splitAndEvaluate(rawValue, " ", true, context); }
  QStringList splitAndEvaluate(
      QString rawValue, QString separator, bool inherit,
      const ParamsProvider *context, QSet<QString> alreadyEvaluated) const;
  /** Escape all characters in string so that they no longer have speciale
   * meaning for evaluate() and splitAndEvaluate() methods. */
  static QString escape(QString string);
  /** Return a globing expression that matches any string that can result
   * in evaluation of the rawValue (@see QRegExp::Wildcard).
   * For instance "foo%{=date:yyyy}-%{bar}.log" is converted into
   * "foo????-*.log" or into "foo*-*.log". */
  // TODO change for QRegularExpresion
  static QString matchingPattern(QString rawValue);
  inline static QRegExp matchingRegexp(QString rawValue) {
    return QRegExp(matchingPattern(rawValue), Qt::CaseSensitive,
                   QRegExp::Wildcard);
  }
  QVariant paramValue(QString key, QVariant defaultValue = QVariant(),
                      QSet<QString> alreadyEvaluated = QSet<QString>()) const;
  bool isNull() const;
  int size() const;
  bool isEmpty() const;
  /** Turn the paramset into a human readable string showing its content.
   * @param inherit include params inherited from parents
   * @param decorate surround with curly braces */
  QString toString(bool inherit = true, bool decorate = true) const;
  /** Create an empty ParamSet having this one for parent. */
  ParamSet createChild() const;
  /** Record debug log messages when a variable evaluation is required and not
   * found.
   * Applicable to all params sets in the applicatoin (global parameter).
   * Defaults: disabled, but if "ENABLE_PARAMSET_VARIABLE_NOT_FOUND_LOGGING"
   * environment variable is set to "true". */
  static void enableVariableNotFoundLogging(bool enabled = true) {
    _variableNotFoundLoggingEnabled = enabled; }

private:
  inline bool appendVariableValue(
      QString *value, QString variable, bool inherit,
      const ParamsProvider *context, QSet<QString> alreadyEvaluated,
      bool logIfVariableNotFound) const;
  inline QString evaluateImplicitVariable(
      QString key, bool inherit, const ParamsProvider *context,
      QSet<QString> alreadyEvaluated) const;
};

Q_DECLARE_METATYPE(ParamSet)
Q_DECLARE_TYPEINFO(ParamSet, Q_MOVABLE_TYPE);

QDebug LIBQTSSUSHARED_EXPORT operator<<(QDebug dbg, const ParamSet &params);

LogHelper LIBQTSSUSHARED_EXPORT operator<<(LogHelper lh,
                                            const ParamSet &params);

#endif // PARAMSET_H
