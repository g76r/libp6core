/* Copyright 2012-2025 Hallowyn, Gregoire Barbier and others.
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
  friend class ParamsProvider; // TODO remove, only needed by DontInheritScope
  QSharedDataPointer<ParamSetData> d;
#if PARAMSET_SUPPORTS_DONTINHERIT
  const static Utf8String DontInheritScope; // TODO remove
#endif

public:
#if PARAMSET_SUPPORTS_DONTINHERIT
  /** Special evaluation scope to disallow inheritance from parent and up */
  const static EvalContext DontInherit; // TODO remove
#endif

  inline ParamSet() noexcept;
  /** First item processed as a key, second one as the matching value and so on.
   * If list size is odd the last key will be inserted with "" value. */
  inline ParamSet(std::initializer_list<Utf8String> list);
  inline ParamSet(std::initializer_list<std::pair<Utf8String,QVariant>> list,
                  const Utf8String &scope = {});
  inline ParamSet(const ParamSet &other) noexcept;
  inline ParamSet(ParamSet &&other) noexcept;
  explicit ParamSet(const QHash<QString,QString> &params);
  explicit ParamSet(const QMap<QString,QString> &params);
  explicit ParamSet(const QHash<Utf8String,Utf8String> &params);
  explicit ParamSet(const QMap<Utf8String,Utf8String> &params);
  explicit ParamSet(const QHash<Utf8String,QVariant> &params);
  explicit ParamSet(const QMap<Utf8String,QVariant> &params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(const QMultiMap<QString,QString> &params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(const QMultiHash<QString,QString> &params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(const QMultiMap<Utf8String,Utf8String> &params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(const QMultiHash<Utf8String,Utf8String> &params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(const QMultiMap<Utf8String,QVariant> &params);
  /** For multi-valued keys, only most recently inserted value is kept. */
  explicit ParamSet(const QMultiHash<Utf8String,QVariant> &params);
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
           const ParamSet &parent = {});
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
  inline ~ParamSet() noexcept;
  inline ParamSet &operator=(const ParamSet &other) noexcept;
  inline ParamSet &operator=(ParamSet &&other) noexcept;
  [[nodiscard]] inline ParamSet parent() const noexcept;
  inline void setParent(const ParamSet &parent);
  [[nodiscard]] inline ParamSet withParent(const ParamSet &new_parent) const {
    ParamSet params = *this;
    params.setParent(new_parent);
    return params;
  }
  ParamSet &insert(const Utf8String &key, const QVariant &value);
  [[deprecated("use insert instead")]]
  inline void setValue(const Utf8String &key, const QVariant &value) {
    insert(key, value); }
  /** insert (override) params taking them from a SQL database query
   *  SQL query is %-evaluated within parent context.
   *  QSqlDatabase must already be open.
   *  e.g.: with insertFromSqlDb(db, "select foo, bar from t1
   *  where id=42 limit 1", {{ 0, "foo"}, {1, "bar"}}),
   *  foo will contain value of column 1 and bar value of column 2.
   *  e.g.: with insertFromSqlDb(db, "select distinct foo, null from t1
   *  union select null, bar from t2", {{ 0, "foos"}, {1, "bars"}}),
   *  foos will be a space separated list of unique non null non empty values
   *  in column foo of table t1
   *  and bars of non null non empty values in column bar of table t2.
   */
  void insertFromSqlDb(const QSqlDatabase &db, const Utf8String &sql,
                       const QMap<int,Utf8String> &bindings);
  /** insert (override) params taking them from a SQL database query.
   *  convenience method resolving sql database name. */
  void insertFromSqlDb(
      const Utf8String &dbname, const Utf8String &sql,
      const QMap<int,Utf8String> &bindings);
  /** insert (override) params taking them from a SQL database query.
   *  convenience method mapping each column in order
   *  e.g. {"foo","bar"} is equivalent to {{0,"foo"},{1,"bar"}}. */
  void insertFromSqlDb(const QSqlDatabase &db, const Utf8String &sql,
                          const Utf8StringList &bindings);
  /** insert (override) params taking them from a SQL database query.
   *  convenience method resolving sql database name and mapping each column
   *  in order
   *  e.g. {"foo","bar"} is equivalent to {{0,"foo"},{1,"bar"}}. */
  void insertFromSqlDb(const Utf8String &dbname, const Utf8String &sql,
                          const Utf8StringList &bindings);
  /** insert (override) params using another ParamSet content */
  ParamSet &insert(const ParamSet &other);
  /** short for insert() */
  inline ParamSet &operator+=(const ParamSet &other) { return insert(other); }
  /** merge (complement) params using another ParamSet content */
  ParamSet &merge(const ParamSet &other);
  /** short for merge() */
  inline ParamSet &operator|=(const ParamSet &other) { return merge(other); }
  void clear();
  ParamSet &erase(const Utf8String &key);

  using ParamsProvider::paramRawValue;
  [[nodiscard]] QVariant paramRawValue(
      const Utf8String &key, const QVariant &def = {},
      const EvalContext &context = {}) const override;

  using ParamsProvider::paramKeys;
  [[nodiscard]] Utf8StringSet paramKeys(
      const EvalContext &context = {}) const override;
#if PARAMSET_SUPPORTS_DONTINHERIT
  [[deprecated("use [scope] filter or remove parent before evaluation")]]
  [[nodiscard]] inline const Utf8StringSet paramKeys(bool inherit) const {
    return paramKeys(inherit ? EvalContext{} : DontInherit); }
#endif

  using ParamsProvider::paramContains;
  [[nodiscard]] bool paramContains(
      const Utf8String &key, const EvalContext &context = {}) const override;
#if PARAMSET_SUPPORTS_DONTINHERIT
  [[deprecated("use [scope] filter or remove parent before evaluation")]]
  [[nodiscard]] inline bool paramContains(
      const Utf8String &key, bool inherit) const {
    return paramContains(key, inherit ? EvalContext{} : DontInherit); }
#endif

#if PARAMSET_SUPPORTS_DONTINHERIT
  using ParamsProvider::paramUtf8List;
  [[deprecated("use [scope] filter or remove parent before evaluation")]]
  [[nodiscard]] inline Utf8StringList paramUtf8List(
      const Utf8String &key, const Utf8String &def = {},
      const ParamsProvider *context = 0, bool inherit = true,
      QList<char> seps = Utf8String::AsciiWhitespace) const {
    return paramUtf8List(key, def, inherit ? EvalContext{context} : DontInherit,
                         seps);
  }
#endif

  [[nodiscard]] inline bool isNull() const noexcept { return !d; }
  [[nodiscard]] inline bool operator!() const noexcept { return isNull(); }
  [[nodiscard]] inline int size() const noexcept;
  [[nodiscard]] inline bool isEmpty() const noexcept;
  inline void detach();
  inline ParamSet &detached() { detach(); return *this; }
  /** Turn the paramset into a human readable string showing its content.
   * @param inherit include params inherited from parents
   * @param decorate surround with curly braces */
  const QString toString(bool inherit = true, bool decorate = true) const;
  const QHash<Utf8String,QVariant> toHash(bool inherit = true) const;
  const QMap<Utf8String,QVariant> toMap(bool inherit = true) const;
  const QHash<Utf8String,Utf8String> toUtf8Hash(bool inherit = true) const;
  const QMap<Utf8String,Utf8String> toUtf8Map(bool inherit = true) const;
  const QHash<QString,QString> toUtf16Hash(bool inherit = true) const;
  const QMap<QString, QString> toUtf16Map(bool inherit = true) const;
  /** Get an external paramset. */
  [[nodiscard]] static ParamSet externalParams(Utf8String set_name);
  /** Register an external paramset. */
  static void registerExternalParams(
      const Utf8String &set_name, ParamSet params);
  /** Unregister every external paramset. */
  static void clearExternalParams();
  /** List names of external paramsets. */
  [[nodiscard]] static Utf8StringList externalParamsNames();

  // scope
  [[nodiscard]] Utf8String paramScope() const override;
  inline void setScope(const Utf8String &scope);
  [[nodiscard]] inline ParamSet withScope(const Utf8String &new_scope) const {
    ParamSet params = *this;
    params.setScope(new_scope);
    return params;
  }

private:
  inline ParamSet(ParamSetData *data) noexcept;
  static ParamSetData *fromQIODevice(
      QIODevice *input, const Utf8String &format,
      const QMap<Utf8String,Utf8String> options,
      const bool escape_percent, const ParamSet &parent);
  inline Utf8StringSet unscopedParamKeys(bool inherit) const;

#if PARAMSET_SUPPORTS_DONTINHERIT
public:
  // temporary partial backward compatibility with old API
  // it's partial because for some method it's broken about inherit
  /** Return a value after parameters substitution.
   * @param searchInParents should search values in parents if not found */
  [[deprecated("use paramUtf8() instead")]]
  inline Utf8String value(Utf8String key, Utf8String def,
                       bool inherit,
                       const ParamsProvider *context = 0) const {
    return paramUtf8(key, def, inherit ? EvalContext{context} : DontInherit); }
  [[deprecated("use paramUtf8() instead")]]
  inline Utf8String value(Utf8String key, bool inherit,
                       const ParamsProvider *context = 0) const {
    return paramUtf8(key, {}, inherit ? EvalContext{context} : DontInherit); }
  [[deprecated("use paramUtf8() instead")]]
  inline Utf8String value(Utf8String key, const ParamsProvider *context) const {
    return paramUtf8(key, {}, context); }
  [[deprecated("use paramUtf8() instead")]]
  inline Utf8String value(Utf8String key, Utf8String def = {}) const {
    return paramUtf8(key, def, 0); }
  [[deprecated("use paramUtf8() instead")]]
  inline Utf8String value(
      Utf8String key, const QByteArray &def, bool inherit = true,
      const ParamsProvider *context = 0) const {
    return paramUtf8(key, def, inherit ? EvalContext{context} : DontInherit); }
  [[deprecated("use paramUtf8() instead")]]
  inline Utf8String value(
      Utf8String key, const QByteArray &def,
      const ParamsProvider *context) const {
    return paramUtf8(key, def, context); }
  [[deprecated("use paramUtf8() instead")]]
  inline Utf8String value(
      Utf8String key, const char *def, bool inherit = true,
      const ParamsProvider *context = 0) const {
    return paramUtf8(key, def, inherit ? EvalContext{context} : DontInherit); }
  [[deprecated("use paramUtf8() instead")]]
  inline Utf8String value(Utf8String key, const char *def,
                       const ParamsProvider *context) const {
    return paramUtf8(key, def, context); }
  [[deprecated("use paramUtf8List() instead")]]
  inline Utf8StringList valueAsStrings(Utf8String key, Utf8String def = {},
                             bool inherit = true,
                             const ParamsProvider *context = 0,
                             Utf8String separators = " "_u8) const {
    return paramUtf8List(key, def, inherit ? EvalContext{context} : DontInherit,
                         separators.toBytesSortedList());
  }
  [[deprecated("use paramNumber() instead")]]
  inline qlonglong valueAsLong(
      Utf8String key, qlonglong def = 0, bool fake_inherit = true,
      const ParamsProvider *context = 0) const {
    Q_UNUSED(fake_inherit)
    return paramNumber<qlonglong>(key, def, context); }
  [[deprecated("use paramNumber() instead")]]
  int valueAsInt(
      Utf8String key, int def = 0, bool fake_inherit = true,
      const ParamsProvider *context = 0) const {
    Q_UNUSED(fake_inherit)
    return paramNumber<int>(key, def, context); }
  [[deprecated("use paramNumber() instead")]]
  double valueAsDouble(
      Utf8String key, double def = 0, bool fake_inherit = true,
      const ParamsProvider *context = 0) const{
    Q_UNUSED(fake_inherit)
    return paramNumber<double>(key, def, context); }
  [[deprecated("use paramNumber() instead")]]
  bool valueAsBool(
      Utf8String key, bool def = false, bool fake_inherit = true,
      const ParamsProvider *context = 0) const{
    Q_UNUSED(fake_inherit)
    return paramNumber<bool>(key, def, context); }
#endif
};

Q_DECLARE_METATYPE(ParamSet);
Q_DECLARE_TYPEINFO(ParamSet, Q_RELOCATABLE_TYPE);

class ParamSetData : public QSharedData {
  friend class ParamSet;
  ParamSet _parent;
  QMap<Utf8String,QVariant> _params;
  Utf8String _scope;

public:
  ParamSetData() = default;
  ParamSetData(const ParamSetData &that) = default;
  ParamSetData(ParamSetData &&that) = default;

private:
  inline ParamSetData(QMap<Utf8String,QVariant> params) : _params(params) { }
  inline ParamSetData(ParamSet parent) : _parent(parent) { }
  inline void clear() { _parent = {}; _params.clear(); _scope = {}; }
};

ParamSet::ParamSet() noexcept {
}

ParamSet::ParamSet(ParamSetData *data) noexcept : d(data){
}

ParamSet::ParamSet(std::initializer_list<Utf8String> list) {
  for (auto it = std::begin(list); it != std::end(list); ++it) {
    auto key = *it;
    ++it;
    if (it != std::end(list)) {
      insert(key, *it);
    } else {
      insert(key, ""_u8);
      break;
    }
  }
}

ParamSet::ParamSet(
    std::initializer_list<std::pair<Utf8String, QVariant> > list,
    const Utf8String &scope) : d(new ParamSetData) {
  for (auto p: list)
    d->_params.insert(p.first, p.second);
  d->_scope = scope;
}

ParamSet::ParamSet(const ParamSet &other) noexcept : d(other.d) {
}

ParamSet::ParamSet(ParamSet &&other) noexcept : d(std::move(other.d)) {
}

ParamSet::~ParamSet() noexcept {
}

ParamSet &ParamSet::operator=(const ParamSet &other) noexcept {
  if (this != &other)
    d = other.d;
  return *this;
}

ParamSet &ParamSet::operator=(ParamSet &&other) noexcept {
  if (this != &other)
    d = std::move(other.d);
  return *this;
}

void ParamSet::detach() {
  d.detach();
}

int ParamSet::size() const noexcept {
  return d ? d->_params.size() : 0;
}

bool ParamSet::isEmpty() const noexcept {
  return d ? d->_params.isEmpty() : true;
}

ParamSet ParamSet::parent() const noexcept {
  return d ? d->_parent : ParamSet();
}

void ParamSet::setParent(const ParamSet &parent) {
  if (!d) [[unlikely]]
    d = new ParamSetData;
  if (d.constData() != parent.d.constData())
    d->_parent = parent;
}

void ParamSet::setScope(const Utf8String &scope) {
  if (!d) [[unlikely]]
    d = new ParamSetData;
  d->_scope = scope;
}

QDebug LIBP6CORESHARED_EXPORT operator<<(QDebug dbg, const ParamSet &params);

p6::log::LogHelper LIBP6CORESHARED_EXPORT operator<<(
    p6::log::LogHelper lh, const ParamSet &params);

#endif // PARAMSET_H
