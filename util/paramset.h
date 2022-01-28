/* Copyright 2012-2022 Hallowyn, Gregoire Barbier and others.
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
#ifndef PARAMSET_H
#define PARAMSET_H

#include <QSharedData>
#include <QList>
#include <QStringList>
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
 ******************************************************************************
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
 * %{=date!iso} short for %{=date!yyyy-MM-ddThh:mm:ss,zzz}
 * %{=date!ms1970} milliseconds since 1970-01-01 00:00:00,000
 * %{=date!s1970} seconds since 1970
 ******************************************************************************
 * %=coarsetimeinterval function: %{=coarsetimeinterval:seconds}
 *
 * formats a time interval as a coarse human readable expression
 *
 * examples:
 * %{=coarsetimeinterval:1.250} -> "1.250 seconds"
 * %{=coarsetimeinterval:125.35} -> "2 minutes 5 seconds"
 * %{=coarsetimeinterval:86402.21} -> "1 days 0 hours"
 ******************************************************************************
 * %=default function: %{=default!expr1[!expr2[...]][!value_if_not_set]}
 *
 * take first non-empty expression in order: expr1 if not empty, expr2 if expr1
 * is empty, expr3 if neither expr1 nor expr2 are set, etc.
 * the function works like nvl/coalesce/ifnull functions in sql
 *   and almost like ${variable:-value_if_not_set} in shell scripts
 * expr1..n are evaluated
 * value_if_not_set is evaluated hence %foo is replaced by foo's value
 *
 * examples:
 * %{=default!%foo!null}
 * %{=default!%foo!foo not set}
 * %{=default:%foo:foo not set!!!}
 * %{=default:%foo:%bar:neither foo nor bar are set!!!}
 * %{=default!%foo!%bar}
 * %{=default!%foo}
 ******************************************************************************
 * %=rawvalue function: %{=rawvalue!variable[!flags]}
 *
 * the function return unevaluated value of a variable
 * flags is a combination of letters with the following meaning:
 * e %-escape value (in case it will be further %-evaluated)
 * h html encode value
 * u html encode will transform urls them into a links
 * n html encode will add br whenever it founds a newline
 *
 * example:
 * %{=rawvalue!foo}
 * %{=rawvalue!foo!hun}
 * %{=rawvalue!foo!e} is an equivalent to %{=escape!%foo}
 ******************************************************************************
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
 ******************************************************************************
 * %=switch function: %{=switch:value[:case1:value1[:case2:value2[...]]][:default_value]}
 * test an input against different reference values and replace it according to
 * matching case
 * all parameters are evaluated, hence %foo is replaced by foo's value
 * if default_value is not specified, left input as is if no case matches
 * see also %=match, with regexps
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
 ******************************************************************************
 * %=match function: %{=match:value[:case1:regexp1[:case2:regexp2[...]]][:default_value]}
 * test an input against different reference regexps and replace it according to
 * matching case
 * all parameters are evaluated, hence %foo is replaced by foo's value
 * if default_value is not specified, left input as is if no case matches
 * see also %=switch if regexps are not needed
 *
 * examples:
 * %{=match:%foo:^a:false:true} -> if starts with 'a': false else: true
 * %{=match:%foo:[0-9]+:true} -> if only digits: true else: %foo
 * %{=match:%foo} -> always return %foo, but won't warn if foo is not defined
 ******************************************************************************
 * %=sub function: %{=sub!input!s-expression!...}
 *
 * input is the data to transform, it is evaluated (%foo become the content of
 *   foo param)
 * s-expression is a substitution expression like those taken by sed's s
 *   command, e.g. /foo/bar/gi or ,.*,,
 *   replacement is evaluated and both regular params substitution and
 *   regexp substitution are available, e.g. %1 will be replaced by first
 *   capture group and %name will be replaced by named capture group if
 *   availlable or param
 *   regular expression are those supported by Qt's QRegularExpression, which
 *   are almost identical to Perl's regexps
 *
 * examples:
 * %{=sub!foo!/o/O} returns "fOo"
 * %{=sub!foo!/o/O/g} returns "fOO"
 * %{=sub;%foo;/a/b/g;/([a-z]+)[0-9]/%1%bar/g}"
 * %{=sub;2015-04-17;~.*-(?<month>[0-9]+)-.*~%month} returns "04"
 ******************************************************************************
 * %=left function: %{=left:input:length}
 *
 * input is the data to transform, it is evaluated (%foo become the content of
 *   foo param)
 * length is the number of character to keep from the input, if negative or
 *   invalid, the whole input is kept
 *
 * examples:
 * %{=left:%foo:4}
 ******************************************************************************
 * %=right function: %{=right:input:length}
 *
 * input is the data to transform, it is evaluated (%foo become the content of
 *   foo param)
 * length is the number of character to keep from the input, if negative or
 *   invalid, the whole input is kept
 *
 * examples:
 * %{=right:%foo:4}
 ******************************************************************************
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
 ******************************************************************************
 * %=htmlencode function: %{=htmlencode:input[:flags]}
 *
 * input is the data to transform, it is evaluated (%foo become the content of
 *   foo param) and cannot contain the separator character (e.g. :)
 * flags can contain following characters:
 *   u to surround url with links markups
 *   n to add br markup whenever a newline is encountered
 *
 * examples:
 * %{=htmlencode:1 < 2} -> 1 &lt; 2
 * %{=htmlencode,http://wwww.google.com/,u} -> <a href="http://wwww.google.com/">http://wwww.google.com/</a>
 * %{=htmlencode http://wwww.google.com/ u} -> same
 * %{=htmlencode|http://wwww.google.com/} -> http://wwww.google.com/
 ******************************************************************************
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
 ******************************************************************************
 * %=random function: %{=random[:modulo[:shift]]
 *
 * produce a pseudo-random integer number between shift (default: 0) and
 * modulo-1+shift.
 * negative modulos are silently converted to their absolute values.
 *
 * examples:
 * %{=random} -> any integer number (at less 32 bits, maybe more)
 * %{=random:100} -> an integer between 0 and 99
 * %{=random:6:1} -> an integer between 1 and 6
 * %{=random:-8:-4} -> an integer between -4 and 3
 ******************************************************************************
 * %=env function:
 *    %{=env:varname1[[:varname2[:...]]:defaultvalue]}
 *
 * lookup system environment variable.
 * varnames and defaultValue are evaluated.
 * values of envvars themselves are not evaluated (USER=%foo will remain %foo),
 * but you can still use %=eval if needed.
 * you must provide a default value if there are more than 1 varname
 *
 * exemples:
 * %{=env:SHELL}
 * %{=env:EDITOR_FOR_%foo:vim}
 * %{=env:USERNAME:%{=env:USER}}
 * %{=env:USERNAME:USER:} (equivalent to previous line, note the trailing :)
 * %{=env:EDITOR_FOR_%foo:${=env:EDITOR:vim}}
 * %{=env,EDITOR_FOR_%foo,EDITOR,vim} (equivalent to previous line)
 * %{=eval!%{=env:FOO}} allows evaluation of % in FOO value
 ******************************************************************************
 * %=eval function: %{=eval!expression}
 *
 * double-evaluate expression to provide a way to force %-evaluation of a
 * variable (or expression) value
 *
 * examples:
 * %{=eval!%foo} returns "baz" if foo is "%bar" and bar is "baz"
 * %{=eval!%{=env:FOO}} allows evaluation of % in FOO env var value
 * %{=eval!%{=rawvalue:foo}} is an equivalent of %foo
 ******************************************************************************
 * %=escape function: %{=escape!expression}
 *
 * escape %-evaluation special characters from expression result (i.e. replace
 * "%" with "%%"), which is the opposite from %=eval
 *
 * examples:
 * %{=escape!%foo} returns "%%bar" if foo is "%bar"
 * %{=rawvalue!foo!e} is an equivalent to %{=escape!%foo}
 * %{=escape!%foo-%baz} returns "%%bar-42" if foo is "%bar" and baz is "42"
 * %{=eval:%{=escape!%foo}} is an equivalent of %{=rawvalue:foo}
 ******************************************************************************
 * %=sha1 function: %{=sha1!expression}
 *
 * compute sha1 of evaluated expression
 *
 * examples:
 * %{=sha1:%%baz} returns "3d8555b0a81f8344fd128060117b985ce9de6bd5"
 ******************************************************************************
 * %=sha256 function: %{=sha256!expression}
 *
 * compute sha256 of evaluated expression
 *
 * examples:
 * %{=sha256:%%baz} returns "48b56c9eb1d1d80188aeda808c72a047cd15803c57117bec272c75145f84f525"
 ******************************************************************************
 * %=md5 function: %{=md5!expression}
 *
 * compute md5 of evaluated expression
 *
 * examples:
 * %{=md5:%%baz} returns "96ab86a37cef7e27d8d45af9c29dc974"
 ******************************************************************************
 * %=hex function: %{=hex!expression[!separtor[!flags]]}
 *
 * hexadecimal representation of utf-8 form of evaluated expression
 * can optionnaly use a one-latin1-character separator between bytes
 * flags is a combination of letters with the following meaning:
 * b process expression value as binary, not utf-8
 *
 * examples:
 * %{=hex:%%baz} returns "2562617a"
 * %{=hex:%%baz: } returns "25 62 61 7a"
 * %{=hex!%%baz!:} returns "25:62:61:7a"
 * %{=hex:%{=fromhex!fbff61!b}::b} returns "fbff61"
 * %{=hex:%{=fromhex!fbff61}::b} returns "3f3f61" (\x3f is '?' placeholder)
 ******************************************************************************
 * %=fromhex function: %{=fromhex!expression[!flags]}
 *
 * convert an hexadecimal representation to actual data
 * ignore invalid characters in input, hence tolerate separators if any
 * flags is a combination of letters with the following meaning:
 * b produces binary result, not utf-8
 *
 * examples:
 * %{=fromhex!2562617a!} returns "%%baz"
 * %{=fromhex!25:62/61 7a!} returns "%%baz"
 * %{=hex:%{=fromhex!fbff61!b}::b} returns "fbff61"
 * %{=hex:%{=fromhex!fbff61}::b} returns "3f3f61" (\x3f is '?' placeholder)
 ******************************************************************************
 * %=base64 function: %{=base64!expression[!flags]}
 *
 * base64 representation of utf-8 form of evaluated expression
 * flags is a combination of letters with the following meaning:
 * b process expression value as binary, not utf-8
 * u encode using base64url instead of base64 (use - and _ instead of + and /)
 * t omit trailing =
 *
 * examples:
 * %{=base64:§} returns "wqc="
 * %{=base64!%{=fromhex:fbff61:b}!b} returns "+/9h"
 * %{=base64!%{=fromhex:fbff61:b}!utb} returns "-_9h"
 * Basic %{=base64!login:password} returns "Basic QmFzaWMgbG9naW46cGFzc3dvcmQ="
 ******************************************************************************
 * %=frombase64 function: %{=frombase64!expression[!flags]}
 *
 * convert a base64 representation to actual data
 * flags is a combination of letters with the following meaning:
 * b produces binary result, not utf-8
 * u decode using base64url instead of base64 (use - and _ instead of + and /)
 *
 * examples:
 * %{=frombase64:wqc=} returns "§"
 * %{=hex!%{=frombase64:+/9h:b}!!b} returns "fbff61"
 * %{=hex!%{=frombase64:-_9h:ub}!!b} returns "fbff61"
 * %{=frombase64!QmFzaWMgbG9naW46cGFzc3dvcmQ=} returns "login:password"
 ******************************************************************************
 */
class LIBP6CORESHARED_EXPORT ParamSet : public ParamsProvider {
  friend class ParamsProviderMerger;
  QSharedDataPointer<ParamSetData> d;
  static bool _variableNotFoundLoggingEnabled;
  static const QString _true;
  static const QString _false;

public:
  ParamSet();
  /** First item processed as a key, second one as the matching value and so on.
   * If list size is odd the last key will be inserted with "". */
  ParamSet(std::initializer_list<QString> list);
  ParamSet(std::initializer_list<std::pair<QString,QVariant>> list);
  ParamSet(const ParamSet &other);
  explicit ParamSet(QHash<QString,QString> params);
  explicit ParamSet(QMap<QString,QString> params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(QMultiMap<QString,QString> params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(QMultiHash<QString,QString> params);
  ~ParamSet();
  ParamSet &operator=(const ParamSet &other);
  ParamSet parent() const;
  void setParent(ParamSet parent);
  void setValue(QString key, QString value);
  void setValue(QString key, QVariant value) {
    setValue(key, value.toString()); }
  void setValue(QString key, bool value) {
    setValue(key, value ? _true : _false); }
  void setValue(QString key, qint8 value, int base = 10) {
    setValue(key, QString::number(value, base)); }
  void setValue(QString key, qint16 value, int base = 10) {
    setValue(key, QString::number(value, base)); }
  void setValue(QString key, qint32 value, int base = 10) {
    setValue(key, QString::number(value, base)); }
  void setValue(QString key, qint64 value, int base = 10) {
    setValue(key, QString::number(value, base)); }
  void setValue(QString key, quint8 value, int base = 10) {
    setValue(key, QString::number(value, base)); }
  void setValue(QString key, quint16 value, int base = 10) {
    setValue(key, QString::number(value, base)); }
  void setValue(QString key, quint32 value, int base = 10) {
    setValue(key, QString::number(value, base)); }
  void setValue(QString key, quint64 value, int base = 10) {
    setValue(key, QString::number(value, base)); }
  void setValue(QString key, double value, char format='g', int precision=6) {
    setValue(key, QString::number(value, format, precision)); }
  void setValue(QString key, float value, char format='g', int precision=6) {
    setValue(key, QString::number(value, format, precision)); }
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
  QStringList valueAsStrings(QString key, QString defaultRawValue = QString(),
                             bool inherit = true,
                             const ParamsProvider *context = 0,
                             QString separators = " ") const {
    return splitAndEvaluate(rawValue(key, defaultRawValue), separators,
                            inherit, context); }
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
    if (v == _true)
      return true;
    if (v == _false)
      return false;
    bool ok;
    int i = v.toInt(&ok);
    if (ok)
      return i == 0 ? false : true;
    return defaultValue;
  }
  /** Return all keys for which the ParamSet or one of its parents hold a value.
    */
  QSet<QString> keys(bool inherit) const;
  QSet<QString> keys() const;
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
  QStringList splitAndEvaluate(
      QString rawValue, QString separators = " ", bool inherit = true,
      const ParamsProvider *context = 0) const {
    return splitAndEvaluate(rawValue, separators, inherit, context,
                            QSet<QString>());
  }
  QStringList splitAndEvaluate(QString rawValue,
                               const ParamsProvider *context) const {
    return splitAndEvaluate(rawValue, " ", true, context); }
  /** Split string and perform parameters substitution.
   *
   * If (and only if) separators is not empty, raw value is splitted into parts
   * separated by any character in separators string, several separators are
   * processed as only one (hence splitted parts cannot be empty) and leading
   * or trailing separators are ignored.
   * Separators, and any other character, can be escaped with backslash (\),
   * therefore backslashes must be backslashed.
   * If separators is empty, neither split nor backslash escape is performed.
   */
  QStringList splitAndEvaluate(
      QString rawValue, QString separators, bool inherit,
      const ParamsProvider *context, QSet<QString> alreadyEvaluated) const;
  /** Escape all characters in string so that they no longer have special
   * meaning for evaluate() and splitAndEvaluate() methods.
   * That is: replace % with %% within the string. */
  static QString escape(QString string);
  /** Return a regular expression that matches any string that can result
   * in evaluation of the rawValue.
   * For instance "foo%{=date:yyyy}-%{bar}.log" is converted into some pattern
   * that can be "foo....-.*\\.log" or "foo.*-.*\\.log" (let be frank: currently
   * the second pattern is returned, not the first one, and it's likely to stay
   * this way).
   * Can be used as an input for QRegularExpression(QString) constructor. */
  static QString matchingRegexp(QString rawValue);
  QVariant paramValue(QString key, const ParamsProvider *context,
                      QVariant defaultValue = QVariant(),
                      QSet<QString> alreadyEvaluated = QSet<QString>()
          ) const override;
  bool isNull() const;
  int size() const;
  bool isEmpty() const;
  void detach();
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
  QHash<QString,QString> toHash(bool inherit = true) const;
  QMap<QString,QString> toMap(bool inherit = true) const;

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

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const ParamSet &params);

LogHelper LIBP6CORESHARED_EXPORT operator<<(LogHelper lh,
                                             const ParamSet &params);

#endif // PARAMSET_H
