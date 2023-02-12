/* Copyright 2012-2023 Hallowyn, Gregoire Barbier and others.
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
#include <QStringList>
#include "log/log.h"
#include "paramsprovider.h"

class ParamSetData;
class PfNode;
class QSqlDatabase;

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
 * Such as %{=date!yyyy-MM-dd}, %{=default!%foo!null} or %{=sub!foo!/o/O/g}.
 *
 * Actually, ParamSet provides such functions begining with an equal sign, and
 * described below, but any application (i.e. any subclass of ParamsProvider)
 * can implement such functions, with any prefix instead of = (or with no
 * prefix, or even with the same = sign although this is discouraged because it
 * would hide future ParamSet functions with the same name).
 *
 * @see paramset_percent_eval_functions.md
 * @see https://gitlab.com/g76r/libp6core/-/blob/master/util/paramset_percent_eval_functions.md
 */
class LIBP6CORESHARED_EXPORT ParamSet : public ParamsProvider {
  friend class ParamsProviderMerger;
  QSharedDataPointer<ParamSetData> d;
  static bool _variableNotFoundLoggingEnabled;

public:
  ParamSet();
  /** First item processed as a key, second one as the matching value and so on.
   * If list size is odd the last key will be inserted with "" value. */
  ParamSet(std::initializer_list<QString> list);
  ParamSet(std::initializer_list<std::pair<QString,QVariant>> list);
  ParamSet(const ParamSet &other);
  explicit ParamSet(const QHash<QString,QString> &params);
  explicit ParamSet(const QMap<QString,QString> &params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(const QMultiMap<QString,QString> &params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(const QMultiHash<QString,QString> &params);
  /** Takes params from PfNode children given an attribute name,
   *  parsing their text content as key-whitespace-value.
   *  Optionnaly a second attribute name is used for "const" params, that is
   *  parameters that are %-evaluated now and have their value %-escaped so
   *  that they won't ever change again later.
   *  e.g.: with ParamSet(node, "param")
   *  (node (param foo bar)(param bar baz)) -> { "foo" = "baz", "bar" = "baz" }
   *  e.g.: with ParamSet(node, "param", "constparam")
   *  (node (param foo bar)(param bar baz)(constparam now "%{=date}"))
   *  -> now's value won't change later and will stay as a timestamp of ParamSet
   *  construction
   */
  ParamSet(const PfNode &parentnode, const QString &attrname,
           const QString &constattrname = QString(),
           const ParamSet &parent = ParamSet());
  ParamSet(const PfNode &parentnode, const QString &attrname,
           const ParamSet &parent);
  /** Takes params from PfNode children given their attribute names,
   *  using their content as value.
   *  e.g.: with ParamSet(node, { "path", "tmp" } )
   *  (node (path /foo/bar)(truncate)) -> { "path" = "/foo/bar", "tmp" = "" }
   */
  ParamSet(const PfNode &parentnode, const QSet<QString> &attrnames,
           const ParamSet &parent = ParamSet());
  /** Takes params from columns values of an SQL query.
   *  SQL query is %-evaluated within parent context.
   *  QSqlDatabase must already be open.
   *  e.g.: with Paramset(db, "select distinct foo, null from t1 union select
   *  null, bar from t2", {{ 0, "foos"}, {1, "bars"}}) foos will be a space
   *  separated list of unique non null non empty values in column foo of table
   *  t1 and bars of non null non empty values in column bar of table t2.
   */
  ParamSet(const QSqlDatabase &db, const QString &sql,
           const QMap<int, QString> &bindings,
           const ParamSet &parent = ParamSet());
  ~ParamSet();
  ParamSet &operator=(const ParamSet &other);
  ParamSet parent() const;
  void setParent(ParamSet parent);
  void setValue(QString key, QString value);
  void setValue(QString key, QVariant value) {
    setValue(key, value.toString()); }
  void setValue(QString key, bool value) {
    setValue(key, value ? QStringLiteral("true") : QStringLiteral("false")); }
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
    setValue(key, QString::number((double)value, format, precision)); }
  /** merge (override) params using another ParamSet content */
  void setValues(ParamSet params, bool inherit = true);
  /** merge (override) params taking them from a SQL database query
   *  SQL query is %-evaluated within parent context.
   *  QSqlDatabase must already be open.
   *  e.g.: with setValuesFromSqlDb(db, "select distinct foo, null from t1
   *  union select null, bar from t2", {{ 0, "foos"}, {1, "bars"}}),
   *  foos will be a space separated list of unique non null non empty values
   *  in column foo of table t1
   *  and bars of non null non empty values in column bar of table t2.
   */
  void setValuesFromSqlDb(
    QSqlDatabase db, QString sql, QMap<int,QString> bindings);
  /** merge (override) params taking them from a SQL database query.
   *  convenience method resolving sql database name. */
  void setValuesFromSqlDb(QString dbname, QString sql, QMap<int,QString> bindings);
  /** merge (override) params taking them from a SQL database query.
   *  convenience method mapping each column in order
   *  e.g. "foo bar" is equivalent to {{0,"foo"},{1,"bar"}}. */
  void setValuesFromSqlDb(QSqlDatabase db, QString sql, QStringList bindings);
  /** merge (override) params taking them from a SQL database query.
   *  convenience method resolving sql database name and mapping each column
   *  in order
   *  e.g. "foo bar" is equivalent to {{0,"foo"},{1,"bar"}}. */
  void setValuesFromSqlDb(QString dbname, QString sql, QStringList bindings);
  /** short for setValues(other) */
  ParamSet &operator<<(const ParamSet &other){ setValues(other); return *this; }
  /** short for setValues(other) */
  ParamSet &operator+=(const ParamSet &other){ setValues(other); return *this; }
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
                       QSet<QString> *alreadyEvaluated) const {
    return evaluate(rawValue(key, inherit), inherit, context, alreadyEvaluated);
  }
  /** Return a value splitted into strings, %-substitution is done after the
   * split (i.e. "%foo bar" has two elements, regardless the number of spaces
   * in %foo value). */
  QStringList valueAsStrings(QString key, QString defaultRawValue = QString(),
                             bool inherit = true,
                             const ParamsProvider *context = 0,
                             QString separators = " ") const {
    return splitAndEvaluate(rawValue(key, defaultRawValue), separators,
                            inherit, context); }
  /** Return a value splitted at first whitespace. Both strings are trimmed.
   * E.g. a raw value of "  foo    bar baz  " is returned as a
   * QPair<>("foo", "bar baz"). */
  const QPair<QString,QString> valueAsStringsPair(
      QString key, bool inherit = true,
      const ParamsProvider *context = 0) const;
  /** @return integer value if the string content is a valid integer
   * C-like prefixes are supported and both kmb and kMGTP suffixes are supported
   * surrounding whitespace is trimmed
   * e.g. 0x1f means 15, 12k means 12000, 12b and 12G mean 12000000000.
   * T and P are supported with long long, not int. */
  qlonglong valueAsLong(QString key, qlonglong defaultValue = 0,
                        bool inherit = true,
                        const ParamsProvider *context = 0) const;
  int valueAsInt(QString key, int defaultValue = 0, bool inherit = true,
                 const ParamsProvider *context = 0) const;
  /** Syntaxic sugar. */
  double valueAsDouble(QString key, double defaultValue = 0,
                       bool inherit = true,
                       const ParamsProvider *context = 0) const;
  /** "faLsE" and "0" are interpreted as false, "trUe" and any non null
   * valid integer number are interpreted as true. whitespace and case are
   * ignored. */
  bool valueAsBool(QString key, bool defaultValue = false,
                          bool inherit = true,
                          const ParamsProvider *context = 0) const;
  /** Return all keys for which the ParamSet or one of its parents hold a value.
    */
  const QSet<QString> keys(bool inherit) const;
  const QSet<QString> keys() const override;
  /** Return true if key is set. */
  bool contains(QString key, bool inherit = true) const;
  /** Perform parameters substitution within the string. */
  QString evaluate(QString rawValue, bool inherit = true,
                   const ParamsProvider *context = 0) const {
    QSet<QString> ae;
    return evaluate(rawValue, inherit, context, &ae); }
  QString evaluate(QString rawValue, const ParamsProvider *context) const {
    return evaluate(rawValue, true, context); }
  QString evaluate(QString rawValue, bool inherit,
                   const ParamsProvider *context,
                   QSet<QString> *alreadyEvaluated) const;
  QStringList splitAndEvaluate(
      QString rawValue, QString separators = " ", bool inherit = true,
      const ParamsProvider *context = 0) const {
    QSet<QString> ae;
    return splitAndEvaluate(rawValue, separators, inherit, context, &ae);
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
      const ParamsProvider *context, QSet<QString> *alreadyEvaluated) const;
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
  static const QString matchingRegexp(QString rawValue);
  using ParamsProvider::paramValue;
  const QVariant paramValue(
    const QString &key, const ParamsProvider *context,
    const QVariant &defaultValue,
    QSet<QString> *alreadyEvaluated) const override;
  bool isNull() const;
  int size() const;
  bool isEmpty() const;
  void detach();
  /** Turn the paramset into a human readable string showing its content.
   * @param inherit include params inherited from parents
   * @param decorate surround with curly braces */
  const QString toString(bool inherit = true, bool decorate = true) const;
  /** Record debug log messages when a variable evaluation is required and not
   * found.
   * Applicable to all params sets in the applicatoin (global parameter).
   * Defaults: disabled, but if "ENABLE_PARAMSET_VARIABLE_NOT_FOUND_LOGGING"
   * environment variable is set to "true". */
  static void enableVariableNotFoundLogging(bool enabled = true) {
    _variableNotFoundLoggingEnabled = enabled; }
  /** Evaluate %= functions out of ParamSet context */
  static QString evaluateFunction(
    const ParamSet &paramset, const QString &key, bool inherit,
    const ParamsProvider *context, QSet<QString> *alreayEvaluated, bool *found);
  const QHash<QString,QString> toHash(bool inherit = true) const;
  const QMap<QString, QString> toMap(bool inherit = true) const;

private:
  inline bool appendVariableValue(
      QString *value, const QString &variable, bool inherit,
      const ParamsProvider *context, QSet<QString> *alreadyEvaluated,
      bool logIfVariableNotFound) const;
};

Q_DECLARE_METATYPE(ParamSet)
Q_DECLARE_TYPEINFO(ParamSet, Q_MOVABLE_TYPE);

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const ParamSet &params);

LogHelper LIBP6CORESHARED_EXPORT operator<<(LogHelper lh,
                                             const ParamSet &params);

#endif // PARAMSET_H
