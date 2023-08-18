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
 * @see percent_evaluation.md for more complete information
 * @see https://gitlab.com/g76r/libp6core/-/blob/master/util/percent_evaluation.md
 */
class LIBP6CORESHARED_EXPORT ParamSet : public ParamsProvider {
  friend class ParamsProviderMerger;
  QSharedDataPointer<ParamSetData> d;

public:
  ParamSet();
  /** First item processed as a key, second one as the matching value and so on.
   * If list size is odd the last key will be inserted with "" value. */
  ParamSet(std::initializer_list<Utf8String> list);
  ParamSet(std::initializer_list<std::pair<Utf8String,QVariant>> list);
  ParamSet(const ParamSet &other);
  explicit ParamSet(const QHash<QString,QString> &params);
  explicit ParamSet(const QMap<QString,QString> &params);
  explicit ParamSet(const QHash<Utf8String,Utf8String> &params);
  explicit ParamSet(const QMap<Utf8String,Utf8String> &params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(const QMultiMap<QString,QString> &params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(const QMultiHash<QString,QString> &params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(const QMultiMap<Utf8String,Utf8String> &params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(const QMultiHash<Utf8String,Utf8String> &params);
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
  ParamSet(const PfNode &parentnode, const Utf8String &attrname,
           const Utf8String &constattrname = {},
           const ParamSet &parent = ParamSet());
  ParamSet(const PfNode &parentnode, const Utf8String &attrname,
           const ParamSet &parent);
  /** Takes params from PfNode children given their attribute names,
   *  using their content as value.
   *  e.g.: with ParamSet(node, { "path", "tmp" } )
   *  (node (path /foo/bar)(truncate)) -> { "path" = "/foo/bar", "tmp" = "" }
   */
  ParamSet(const PfNode &parentnode, const Utf8StringSet &attrnames,
           const ParamSet &parent = {});
  /** Takes params from columns values of an SQL query.
   *  SQL query is %-evaluated within parent context.
   *  QSqlDatabase must already be open.
   *  e.g.: with Paramset(db, "select distinct foo, null from t1 union select
   *  null, bar from t2", {{ 0, "foos"}, {1, "bars"}}) foos will be a space
   *  separated list of unique non null non empty values in column foo of table
   *  t1 and bars of non null non empty values in column bar of table t2.
   */
  ParamSet(const QSqlDatabase &db, const Utf8String &sql,
           const QMap<int, Utf8String> &bindings,
           const ParamSet &parent = {});
  /** Takes params from file (or socket or command output...).
   * e.g.: QFile file("/tmp/foo.csv"); ParamSet(&file);
   * If input is not opened, open it. */
  ParamSet(QIODevice *input, const Utf8String &format = "csv",
           const QMap<Utf8String, Utf8String> options = { { "separator", "," } },
           const bool escape_percent = false, const ParamSet &parent = {} );
  /** Takes params from file.
   * e.g.: ParamSet("/tmp/foo.csv") */
  static ParamSet fromFile(
      const QByteArray &file_name, const Utf8String &format = "csv",
      const QMap<Utf8String, Utf8String> options = { { "separator", "," } },
      const bool escape_percent = false, const ParamSet &parent = {} );
  /** Takes params from command output.
   * e.g.: ParamSet({ "/opt/myscript.sh", "-n", "secrets" } ) */
  static ParamSet fromCommandOutput(
      const QStringList &cmdline, const Utf8String &format = "csv",
      const QMap<Utf8String,Utf8String> options = { { "separator", "," } },
      const bool escape_percent = false, const ParamSet &parent = {} );
  ~ParamSet();
  ParamSet &operator=(const ParamSet &other);
  ParamSet parent() const;
  void setParent(ParamSet parent);
  void setValue(Utf8String key, Utf8String value);
  void setValue(Utf8String key, QString value) {
    setValue(key, Utf8String(value)); }
  void setValue(Utf8String key, QByteArray value) {
    setValue(key, Utf8String(value)); }
  void setValue(Utf8String key, const char *value) {
    setValue(key, Utf8String(value)); }
  void setValue(Utf8String key, QVariant value) {
    setValue(key, Utf8String(value)); }
  void setValue(Utf8String key, bool value) {
    setValue(key, Utf8String(value)); }
  void setValue(Utf8String key, qint8 value, int base = 10) {
    setValue(key, Utf8String::number(value, base)); }
  void setValue(Utf8String key, qint16 value, int base = 10) {
    setValue(key, Utf8String::number(value, base)); }
  void setValue(Utf8String key, qint32 value, int base = 10) {
    setValue(key, Utf8String::number(value, base)); }
  void setValue(Utf8String key, qint64 value, int base = 10) {
    setValue(key, Utf8String::number(value, base)); }
  void setValue(Utf8String key, quint8 value, int base = 10) {
    setValue(key, Utf8String::number(value, base)); }
  void setValue(Utf8String key, quint16 value, int base = 10) {
    setValue(key, Utf8String::number(value, base)); }
  void setValue(Utf8String key, quint32 value, int base = 10) {
    setValue(key, Utf8String::number(value, base)); }
  void setValue(Utf8String key, quint64 value, int base = 10) {
    setValue(key, Utf8String::number(value, base)); }
  void setValue(Utf8String key, double value, char format='g', int precision=6) {
    setValue(key, Utf8String::number(value, format, precision)); }
  void setValue(Utf8String key, float value, char format='g', int precision=6) {
    setValue(key, Utf8String::number((double)value, format, precision)); }
  /** merge (override) params using another ParamSet content */
  void setValues(ParamSet params, bool inherit = true);
  /** merge (override) params taking them from a SQL database query
   *  SQL query is %-evaluated within parent context.
   *  QSqlDatabase must already be open.
   *  e.g.: with setValuesFromSqlDb(db, "select foo, bar from t1
   *  where id=42 limit 1", {{ 0, "foo"}, {1, "bar"}}),
   *  foo will contain value of column 1 and bar value of column 2.
   *  e.g.: with setValuesFromSqlDb(db, "select distinct foo, null from t1
   *  union select null, bar from t2", {{ 0, "foos"}, {1, "bars"}}),
   *  foos will be a space separated list of unique non null non empty values
   *  in column foo of table t1
   *  and bars of non null non empty values in column bar of table t2.
   */
  void setValuesFromSqlDb(QSqlDatabase db, Utf8String sql,
                          QMap<int, Utf8String> bindings);
  /** merge (override) params taking them from a SQL database query.
   *  convenience method resolving sql database name. */
  void setValuesFromSqlDb(
      Utf8String dbname, Utf8String sql, QMap<int,Utf8String> bindings);
  /** merge (override) params taking them from a SQL database query.
   *  convenience method mapping each column in order
   *  e.g. {"foo","bar"} is equivalent to {{0,"foo"},{1,"bar"}}. */
  void setValuesFromSqlDb(QSqlDatabase db, Utf8String sql,
                          Utf8StringList bindings);
  /** merge (override) params taking them from a SQL database query.
   *  convenience method resolving sql database name and mapping each column
   *  in order
   *  e.g. {"foo","bar"} is equivalent to {{0,"foo"},{1,"bar"}}. */
  void setValuesFromSqlDb(Utf8String dbname, Utf8String sql,
                          Utf8StringList bindings);
  /** short for setValues(other) */
  ParamSet &operator<<(const ParamSet &other){ setValues(other); return *this; }
  /** short for setValues(other) */
  ParamSet &operator+=(const ParamSet &other){ setValues(other); return *this; }
  void clear();
  void removeValue(Utf8String key);
  /** Return a value after parameters substitution.
   * @param searchInParents should search values in parents if not found */
  inline Utf8String value(Utf8String key, Utf8String defaultValue = {},
                       bool inherit = true,
                       const ParamsProvider *context = 0) const {
    return evaluate(paramRawUtf8(key, defaultValue, inherit), inherit, context); }
  inline Utf8String value(Utf8String key, bool inherit,
                       const ParamsProvider *context = 0) const {
    return evaluate(paramRawUtf8(key, Utf8String{}, inherit), inherit, context); }
  inline Utf8String value(Utf8String key, const ParamsProvider *context) const {
    return evaluate(paramRawUtf8(key, Utf8String{}, true), true, context); }
  inline Utf8String value(Utf8String key, bool inherit, const ParamsProvider *context,
                       Utf8StringSet *alreadyEvaluated) const {
    return evaluate(paramRawUtf8(key, inherit), inherit, context, alreadyEvaluated);
  }
  inline Utf8String value(
      Utf8String key, const QByteArray &defaultValue, bool inherit = true,
      const ParamsProvider *context = 0) const {
    return evaluate(paramRawUtf8(key, Utf8String{defaultValue}, inherit), inherit,
                    context); }
  inline Utf8String value(
      Utf8String key, const QByteArray &defaultValue,
      const ParamsProvider *context) const {
    return evaluate(paramRawUtf8(key, Utf8String{defaultValue}, true), true,
                    context); }
  inline Utf8String value(
      Utf8String key, const char *defaultValue, bool inherit = true,
      const ParamsProvider *context = 0) const {
    return evaluate(paramRawUtf8(key, Utf8String{defaultValue}, inherit), inherit,
                    context); }
  inline Utf8String value(Utf8String key, const char *defaultValue,
                       const ParamsProvider *context) const {
    return evaluate(paramRawUtf8(key, Utf8String{defaultValue}, true), true,
                    context); }
  /** Return a value splitted into strings, %-substitution is done after the
   * split (i.e. "%foo bar" has two elements, regardless the number of spaces
   * in %foo value). */
  Utf8StringList valueAsStrings(Utf8String key, Utf8String defaultRawValue = {},
                             bool inherit = true,
                             const ParamsProvider *context = 0,
                             Utf8String separators = " "_u8) const {
    return splitAndEvaluate(paramRawUtf8(key, defaultRawValue), separators,
                            inherit, context); }
  /** Return a value splitted at first whitespace. Both strings are trimmed.
   * E.g. a raw value of "  foo    bar baz  " is returned as a
   * QPair<>("foo", "bar baz"). */
  const QPair<Utf8String,Utf8String> valueAsStringsPair(
      Utf8String key, bool inherit = true,
      const ParamsProvider *context = 0) const;
  /** @return integer value if the string content is a valid integer
   * C-like prefixes are supported and both kmb and kMGTP suffixes are supported
   * surrounding whitespace is trimmed
   * e.g. 0x1f means 15, 12k means 12000, 12b and 12G mean 12000000000.
   * T and P are supported with long long, not int. */
  qlonglong valueAsLong(Utf8String key, qlonglong defaultValue = 0,
                        bool inherit = true,
                        const ParamsProvider *context = 0) const;
  int valueAsInt(Utf8String key, int defaultValue = 0, bool inherit = true,
                 const ParamsProvider *context = 0) const;
  /** Syntaxic sugar. */
  double valueAsDouble(Utf8String key, double defaultValue = 0,
                       bool inherit = true,
                       const ParamsProvider *context = 0) const;
  /** "faLsE" and "0" are interpreted as false, "trUe" and any non null
   * valid integer number are interpreted as true. whitespace and case are
   * ignored. */
  bool valueAsBool(Utf8String key, bool defaultValue = false,
                   bool inherit = true,
                   const ParamsProvider *context = 0) const;
  /** Return all keys for which the ParamSet or one of its parents hold a value.
    */
  [[nodiscard]] const Utf8StringSet paramKeys(bool inherit) const;
  /** Same as paramKeys(true). */
  [[nodiscard]] const Utf8StringSet paramKeys() const override;
  /** Return true if key is set. */
  [[nodiscard]] bool paramContains(const Utf8String &key, bool inherit) const;
  /** Return true if key is set. */
  [[nodiscard]] bool paramContains(const Utf8String &key) const override;
  /** Same as paramRawValue() but skip the QVariant overhead. */
  [[nodiscard]] const Utf8String paramRawUtf8(
      const Utf8String &key, const Utf8String &def, bool inherit) const;
  /** Same as paramRawUtf8 with inherit = true. */
  [[nodiscard]] const Utf8String paramRawUtf8(
      const Utf8String &key, const Utf8String &def = {}) const override;
//    return Utf8String(paramRawValue(key, def)); }
  /** Convenience method */
  [[nodiscard]] inline const Utf8String paramRawUtf8(
      const Utf8String &key, bool inherit) const {
    return paramRawUtf8(key, {}, inherit); }

  using ParamsProvider::paramRawValue;
  /** Return a value without performing parameters substitution.
   * @param inherit should search values in parents if not found */
  [[nodiscard]] const QVariant paramRawValue(
      const Utf8String &key, const QVariant &def, bool inherit) const;
  /** Same as paramRawValue(key, def, true). */
  [[nodiscard]] const QVariant paramRawValue(
      const Utf8String &key, const QVariant &def = {}) const override;
  [[nodiscard]] const ScopedValue paramScopedRawValue(
      const Utf8String &key, const QVariant &def = {}) const override;
  [[nodiscard]] bool isNull() const;
  [[nodiscard]] int size() const;
  [[nodiscard]] bool isEmpty() const;
  void detach();
  /** Turn the paramset into a human readable string showing its content.
   * @param inherit include params inherited from parents
   * @param decorate surround with curly braces */
  const QString toString(bool inherit = true, bool decorate = true) const;
  const QHash<Utf8String,Utf8String> toHash(bool inherit = true) const;
  const QMap<Utf8String, Utf8String> toMap(bool inherit = true) const;
  const QHash<QString,QString> toStringHash(bool inherit = true) const;
  const QMap<QString, QString> toStringMap(bool inherit = true) const;
  /** Get an external paramset. */
  [[nodiscard]] static ParamSet externalParams(Utf8String set_name);
  /** Register an external paramset. */
  static void registerExternalParams(
      const Utf8String &set_name, ParamSet params);
  /** Unregister every external paramset. */
  static void clearExternalParams();
  /** List names of external paramsets. */
  [[nodiscard]] static Utf8StringList externalParamsNames();

private:
  ParamSet(ParamSetData *data);
  static ParamSetData *fromQIODevice(
      QIODevice *input, const Utf8String &format,
      const QMap<Utf8String,Utf8String> options,
      const bool escape_percent, const ParamSet &parent);
};

Q_DECLARE_METATYPE(ParamSet)
Q_DECLARE_TYPEINFO(ParamSet, Q_MOVABLE_TYPE);

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const ParamSet &params);

LogHelper LIBP6CORESHARED_EXPORT operator<<(LogHelper lh,
                                             const ParamSet &params);

#endif // PARAMSET_H
