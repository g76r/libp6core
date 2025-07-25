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
#include "paramset.h"
#include "format/timeformats.h"
#include "regexpparamsprovider.h"
#include "paramsprovidermerger.h"
#include "format/stringutils.h"
#include "radixtree.h"
#include "pf/pfnode.h"
#include "csv/csvfile.h"
#include "util/utf8string.h"
#include <QRegularExpression>
#include <QFile>
#include <QBuffer>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QProcess>
#include <QTimer>

using EvalContext = ParamSet::EvalContext;

#if PARAMSET_SUPPORTS_DONTINHERIT
const Utf8String ParamSet::DontInheritScope = "!inherit"_u8;
const EvalContext ParamSet::DontInherit =
    ParamSet::EvalContext{ ParamSet::DontInheritScope };
static Utf8StringSet _almost_empty_pretend_it_is{"!inherit"_u8};
#endif
static QMap<Utf8String,ParamSet> _externals;
static QMutex _externals_mutex;

static int staticInit() {
  qRegisterMetaType<ParamSet>();
  return 0;
}
Q_CONSTRUCTOR_FUNCTION(staticInit)

ParamSet::ParamSet(const QMap<Utf8String, TypedValue> &params)
  : d(new ParamSetData(params)) {
}

ParamSet::ParamSet(const QHash<QString, QString> &params)
  : d(new ParamSetData) {
  for (const auto &[key,value]: params.asKeyValueRange())
    d->_params.insert(key, value);
}

ParamSet::ParamSet(const QHash<Utf8String,Utf8String> &params)
  : d(new ParamSetData) {
  for (const auto &[key,value]: params.asKeyValueRange())
    d->_params.insert(key, value);
}

ParamSet::ParamSet(const QHash<Utf8String,TypedValue> &params)
  : d(new ParamSetData) {
  for (const auto &[key,value]: params.asKeyValueRange())
    d->_params.insert(key, value);
}

ParamSet::ParamSet(const QMap<QString, QString> &params)
  : d(new ParamSetData) {
  for (const auto &[key,value]: params.asKeyValueRange())
    d->_params.insert(key, value);
}

ParamSet::ParamSet(const QMap<Utf8String,Utf8String> &params)
  : d(new ParamSetData) {
  for (const auto &[key,value]: params.asKeyValueRange())
    d->_params.insert(key, value);
}

ParamSet::ParamSet(const QMultiMap<QString, QString> &params)
  : d(new ParamSetData) {
  for (const auto &[key,value]: params.asKeyValueRange())
    d->_params.insert(key, value);
}

ParamSet::ParamSet(const QMultiHash<QString, QString> &params)
  : d(new ParamSetData) {
  for (const auto &[key,value]: params.asKeyValueRange())
    d->_params.insert(key, value);
}

ParamSet::ParamSet(const QMultiMap<Utf8String, Utf8String> &params)
  : d(new ParamSetData) {
  for (const auto &[key,value]: params.asKeyValueRange())
    d->_params.insert(key, value);
}

ParamSet::ParamSet(const QMultiHash<Utf8String, Utf8String> &params)
  : d(new ParamSetData) {
  for (const auto &[key,value]: params.asKeyValueRange())
    d->_params.insert(key, value);
}

ParamSet::ParamSet(const QMultiMap<Utf8String, TypedValue> &params)
  : d(new ParamSetData) {
  for (const auto &[key,value]: params.asKeyValueRange())
    d->_params.insert(key, value);
}

ParamSet::ParamSet(const QMultiHash<Utf8String, TypedValue> &params)
  : d(new ParamSetData) {
  for (const auto &[key,value]: params.asKeyValueRange())
    d->_params.insert(key, value);
}

ParamSet::ParamSet(
  const PfNode &parentnode, const Utf8String &attrname,
  const Utf8String &constattrname, const ParamSet &parent)
  : d(new ParamSetData(parent)) {
  if (!attrname.isEmpty())
    for (const auto &[key, value]: parentnode.children_as_text_pairs_range(attrname))
      if (!key.isEmpty())
        d->_params.insert(key, value.isNull() ? ""_u8 : value);
  if (!constattrname.isEmpty()) {
    ParamSet constparams(parentnode, constattrname, Utf8String(), ParamSet());
    for (const auto &k: constparams.paramKeys()) {
      auto value = PercentEvaluator::escape(constparams.paramUtf8(k, this));
      d->_params.insert(k, value.isNull() ? ""_u8 : value);
    }
  }
  if (d->_params.isEmpty() && !parent)
    d.reset();
}

ParamSet::ParamSet(const PfNode &parentnode, const Utf8String &attrname,
                   const ParamSet &parent)
  : ParamSet(parentnode, attrname, Utf8String(), parent) {
}

ParamSet::ParamSet(const PfNode &parentnode, const Utf8StringSet &attrnames,
  const ParamSet &parent) : d(new ParamSetData(parent)) {
  for (const PfNode &child : parentnode.children())
    if (attrnames.contains(child.name())) {
      Utf8String value = child.content_as_text();
      d->_params.insert(child.name(), value.isNull() ? ""_u8 : value);
    }
  if (d->_params.isEmpty() && !parent)
    d.reset();
}

ParamSet::ParamSet(
    const QSqlDatabase &db, const Utf8String &sql,
    const QMap<int,Utf8String> &bindings, const ParamSet &parent)
  : d(new ParamSetData(parent)) {
  QSqlQuery query(db);
  query.prepare(PercentEvaluator::eval_utf16(sql, &parent));
  if (!query.exec()) {
    QSqlError error = query.lastError();
    Log::warning() << "failure trying to load params from SQL query: "
                   << " error: " << error.nativeErrorCode() << " "
                   << error.driverText() << " " << error.databaseText()
                   << " " << sql;
    return;
  }
  QMap<int,Utf8StringList> values;
  while (query.next()) {
    auto r = query.record();
    for (int i = 0; i < r.count(); ++i) {
      if (!bindings.contains(i))
        continue;
      auto s = Utf8String(r.field(i).value());
      if (s.isEmpty()) // ignoring both nulls and empty strings
        continue;
      values[i].append(PercentEvaluator::escape(s));
    }
  }
  for (const auto &[i,key]: bindings.asKeyValueRange()) {
    insert(key, values[i].join(' '));
  }
}

ParamSet::ParamSet(
    QIODevice *input, const Utf8String &format,
    const QMap<Utf8String,Utf8String> options, const bool escape_percent,
    const ParamSet &parent)
  : d(fromQIODevice(input, format, options, escape_percent, parent)) {
}

ParamSet &ParamSet::insert(const Utf8String &key, const TypedValue &value) {
  if (key.isEmpty())
    [[unlikely]] return *this;
  if (!d)
    [[unlikely]] d = new ParamSetData;
  if (!!value)
    d->_params.insert(key, value);
  else
    d->_params.remove(key);
  return *this;
}

ParamSet &ParamSet::insert(const ParamSet &params) {
  if (!d)
    [[unlikely]] d = new ParamSetData;
  for (const auto &key: params.paramKeys())
    d->_params.insert(key, params.paramRawValue(key));
  return *this;
}

ParamSet &ParamSet::merge(const ParamSet &params) {
  if (!d)
    [[unlikely]] d = new ParamSetData;
  for (const auto &key: params.paramKeys())
    if (!d->_params.contains(key))
      d->_params.insert(key, params.paramRawValue(key));
  return *this;
}

ParamSet &ParamSet::erase(const Utf8String &key) {
  if (d)
    d->_params.remove(key);
  return *this;
}

void ParamSet::clear() {
  if (d)
    d->clear();
}

TypedValue ParamSet::paramRawValue(
    const Utf8String &key, const TypedValue &def,
    const EvalContext &original_context) const {
  EvalContext context = original_context;
  TypedValue v;
  if (!context.functionsEvaluated()) {
    bool is_function;
    v = PercentEvaluator::eval_function(key, context, &is_function);
    if (is_function)
      return v;
    context.setFunctionsEvaluated(); // avoid function eval in parents
  }
  if (!d)
    [[unlikely]] return def;
  if (context.hasScopeOrNone(paramScope())
#if PARAMSET_SUPPORTS_DONTINHERIT
      || context.scopeFilter() == _almost_empty_pretend_it_is
#endif
      ) {
    v = d->_params.value(key);
    if (!!v)
      return v;
  }
#if PARAMSET_SUPPORTS_DONTINHERIT
  if (context.containsScope(DontInheritScope))
    return def;
#endif
  return parent().paramRawValue(key, def, context);
}

Utf8StringSet ParamSet::paramKeys(const EvalContext &context) const {
  if (!d)
    [[unlikely]] return {};
  Utf8StringSet set;
  if (context.hasScopeOrNone(paramScope())
#if PARAMSET_SUPPORTS_DONTINHERIT
      || context.scopeFilter() == _almost_empty_pretend_it_is
#endif
      )
    for (const auto &[k,v]: d->_params.asKeyValueRange())
      set += k;
#if PARAMSET_SUPPORTS_DONTINHERIT
  if (!context.containsScope(DontInheritScope))
#endif
    set += parent().paramKeys(context);
  return set;
}

bool ParamSet::paramContains(
    const Utf8String &key, const EvalContext &context) const {
  if (!d)
    [[unlikely]] return false;
  if (context.hasScopeOrNone(paramScope()) && d->_params.contains(key))
    return true;
#if PARAMSET_SUPPORTS_DONTINHERIT
  if (context.containsScope(DontInheritScope))
    return false;
#endif
  return parent().paramContains(key, context);
}

Utf8StringSet ParamSet::unscopedParamKeys(bool inherit) const {
  Utf8StringSet keys;
  if (!d)
    return {};
  for (const auto &[k,v]: d->_params.asKeyValueRange())
    keys += k;
  if (inherit)
    keys += d->_parent.unscopedParamKeys(true);
  return keys;
}

const QString ParamSet::toString(bool inherit, bool decorate) const {
  QString s;
  if (decorate)
    s.append("{ ");
  bool first = true;
  for(const auto &key: unscopedParamKeys(inherit).toSortedList()) {
    if (first)
      first = false;
    else
      s.append(' ');
    s.append(key).append('=').append(paramRawUtf8(key));
  }
  if (decorate)
    s.append('}');
  return s;
}

const QHash<Utf8String, TypedValue> ParamSet::toHash(bool inherit) const {
  QHash<Utf8String,TypedValue> hash;
  for (const auto &key: unscopedParamKeys(inherit))
    hash.insert(key, paramRawValue(key));
  return hash;
}

const QMap<Utf8String, TypedValue> ParamSet::toMap(bool inherit) const {
  QMap<Utf8String,TypedValue> map;
  for (const auto &key: unscopedParamKeys(inherit))
    map.insert(key, paramRawValue(key));
  return map;
}

const QHash<Utf8String, Utf8String> ParamSet::toUtf8Hash(bool inherit) const {
  QHash<Utf8String,Utf8String> hash;
  for (const auto &key: unscopedParamKeys(inherit))
    hash.insert(key, paramRawUtf8(key));
  return hash;
}

const QMap<Utf8String, Utf8String> ParamSet::toUtf8Map(bool inherit) const {
  QMap<Utf8String,Utf8String> map;
  for (const auto &key: unscopedParamKeys(inherit))
    map.insert(key, paramRawUtf8(key));
  return map;
}

const QHash<QString, QString> ParamSet::toUtf16Hash(bool inherit) const {
  QHash<QString,QString> hash;
  for (const auto &key: unscopedParamKeys(inherit))
    hash.insert(key, paramRawValue(key).as_utf16());
  return hash;
}

const QMap<QString,QString> ParamSet::toUtf16Map(bool inherit) const {
  QMap<QString,QString> map;
  for (const auto &key: unscopedParamKeys(inherit))
    map.insert(key, paramRawValue(key).as_utf16());
  return map;
}

Utf8String ParamSet::paramScope() const {
  return d ? d->_scope : Utf8String{};
}

QDebug operator<<(QDebug dbg, const ParamSet &params) {
  dbg.nospace() << "{";
  bool first = true;
  for (ParamSet p = params; !!p; p = p.parent()) {
    if (first)
      first = false;
    else
      dbg.nospace().noquote() << "->>";
    auto scope = p.paramScope();
    auto keys = p.paramKeys(EvalContext(scope)).toSortedList();
    for (const auto &key: keys)
      if (scope.isEmpty())
        dbg.space().noquote() << key << "="
                              << p.paramRawValue(key);
      else
        dbg.space().noquote() << "["+scope+"]"+key << "="
                              << p.paramRawValue(key);
    if (keys.isEmpty())
      dbg.space().noquote() << "0";
    if (scope.isEmpty())
      break;
  }
  dbg.nospace().noquote() << "}";
  return dbg.space();
}

p6::log::LogHelper operator<<(p6::log::LogHelper lh, const ParamSet &params) {
  lh << "{ ";

  bool first = true;
  for (ParamSet p = params; !!p; p = p.parent()) {
    if (first)
      first = false;
    else
      lh << "->> ";
    auto scope = p.paramScope();
    auto keys = p.paramKeys(EvalContext(scope)).toSortedList();
    for (const auto &key: keys)
      if (scope.isEmpty())
        lh << key << "=" << p.paramRawValue(key) << " ";
      else
        lh << "["+scope+"]"+key << "=" << p.paramRawValue(key) << " ";
    if (keys.isEmpty())
      lh << "0 ";
    if (scope.isEmpty())
      break;
  }
  return lh << "}";
}

void ParamSet::insertFromSqlDb(
    const QSqlDatabase &db, const Utf8String &sql,
    const QMap<int, Utf8String> &bindings) {
  *this += ParamSet(db, sql, bindings, *this);
}

void ParamSet::insertFromSqlDb(
    const Utf8String &dbname, const Utf8String &sql,
    const QMap<int, Utf8String> &bindings) {
  insertFromSqlDb(QSqlDatabase::database(dbname), sql, bindings);
}

void ParamSet::insertFromSqlDb(
    const Utf8String &dbname, const Utf8String &sql,
    const Utf8StringList &bindings) {
  insertFromSqlDb(QSqlDatabase::database(dbname), sql, bindings);
}

void ParamSet::insertFromSqlDb(
    const QSqlDatabase &db, const Utf8String &sql,
    const Utf8StringList &bindings) {
  QMap<int,Utf8String> map;
  int i = 0;
  for (const auto &key: bindings)
    map.insert(i++, key);
  insertFromSqlDb(db, sql, map);
}

ParamSetData *ParamSet::fromQIODevice(
    QIODevice *input, const Utf8String &format,
    const QMap<Utf8String,Utf8String> options,
    const bool escape_percent, const ParamSet &parent) {
  auto d = new ParamSetData(parent);
  if (!input || format != "csv")
    return d;
  if (!input->isOpen()) {
    if (!input->open(QIODevice::ReadOnly)) {
      QFile *file = qobject_cast<QFile*>(input);
      Log::error() << "cannot open file to read parameters: "
                   << (file ? file->fileName() : input->metaObject()->className())
                   << input->errorString();
      return d;
    }
  }
  auto separator = options.value("separator"_u8).value(0,',');
  auto quote = options.value("quote"_u8).value(0,'"');
  auto escape = options.value("escape"_u8).value(0,'\\');
  CsvFile csvfile;
  csvfile.enableHeaders(false);
  csvfile.setFieldSeparator(separator);
  csvfile.setQuoteChar(quote);
  csvfile.setEscapeChar(escape);
  csvfile.openReadonly(input);
  auto rows = csvfile.rows();
  //qDebug() << "***password from csv" << rows << separator << quote << escape;
  for (const auto &row: rows) {
    auto key = row.value(0);
    auto value = row.value(1);
    if (key.isEmpty())
      continue;
    if (escape_percent)
      d->_params.insert(key, PercentEvaluator::escape(value));
    else
      d->_params.insert(key, value);
  }
  return d;
}

ParamSet ParamSet::fromFile(
    const QByteArray &file_name, const Utf8String &format,
    const QMap<Utf8String,Utf8String> options,
    const bool escape_percent, const ParamSet &parent) {
  //qDebug() << "***password fromFile" << file_name;
  QFile file(file_name);
  return ParamSet(fromQIODevice(&file, format, options, escape_percent, parent));
}

ParamSet ParamSet::fromCommandOutput(
    const QStringList &cmdline, const Utf8String &format,
    const QMap<Utf8String, Utf8String> options,
    const bool escape_percent, const ParamSet &parent){
  //qDebug() << "***password fromCommandOutput" << cmdline;
  ParamSet params(parent);
  if (cmdline.size() < 1) {
    Log::error() << "cannot start external params command with empty cmdline";
    return params;
  }
  auto program = cmdline.value(0);
  auto args = cmdline.sliced(1);
  auto process = new QProcess;
  QObject::connect(process, &QProcess::finished,
                   [cmdline](int exitCode, QProcess::ExitStatus exitStatus) {
    bool success = (exitStatus == QProcess::NormalExit
                    && exitCode == 0);
    if (success)
      return;
    Log::error() << "cannot execute external params command " << cmdline
                 << ": process failed with exit code " << exitCode;
  });
  process->setStandardErrorFile(QProcess::nullDevice());
  QTimer::singleShot(10'000, process, &QProcess::terminate);
  process->start(program, args);
  if (!process->waitForStarted(10'000)) {
    Log::error() << "cannot start external params command " << cmdline;
    process->deleteLater();
    return params;
  }
  if (!process->waitForFinished(10'000)) {
    Log::error() << "cannot wait for external params command finishing "
                 << cmdline;
    process->deleteLater();
    return params;
  }
  auto output = process->readAllStandardOutput();
  QBuffer buffer(&output);
  params = fromQIODevice(&buffer, format, options, escape_percent, parent);
  process->deleteLater();
  return params;
}

ParamSet ParamSet::externalParams(Utf8String set_name) {
  QMutexLocker ml(&_externals_mutex);
  return _externals.value(set_name);
}

void ParamSet::registerExternalParams(
    const Utf8String &set_name, ParamSet params) {
  QMutexLocker ml(&_externals_mutex);
  //qDebug() << "***password registerExternalParams" << set_name << params.toString();
  _externals.insert(set_name, params);
}

void ParamSet::clearExternalParams() {
  QMutexLocker ml(&_externals_mutex);
  _externals.clear();
}

Utf8StringList ParamSet::externalParamsNames() {
  QMutexLocker ml(&_externals_mutex);
  auto list = _externals.keys();
  list.detach();
  return list;
}
