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
#include "paramset.h"
#include "format/timeformats.h"
#include "characterseparatedexpression.h"
#include "regexpparamsprovider.h"
#include "paramsprovidermerger.h"
#include "format/stringutils.h"
#include "radixtree.h"
#include "pf/pfnode.h"
#include "util/mathexpr.h"
#include "pf/pfutils.h"
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

static QMap<Utf8String,ParamSet> _externals;

QMutex _externals_mutex;

class ParamSetData : public QSharedData {
public:
  ParamSet _parent;
  QMap<Utf8String,Utf8String> _params;
  ParamSetData() { }
  ParamSetData(QMap<Utf8String,Utf8String> params) : _params(params) { }
  ParamSetData(ParamSet parent) : _parent(parent) { }
};

ParamSet::ParamSet() {
}

ParamSet::ParamSet(ParamSetData *data) : d(data){
}

ParamSet::ParamSet(std::initializer_list<Utf8String> list) {
  for (auto it = std::begin(list); it != std::end(list); ++it) {
    auto key = *it;
    ++it;
    if (it != std::end(list)) {
      setValue(key, *it);
    } else {
      setValue(key, ""_u8);
      break;
    }
  }
}

ParamSet::ParamSet(
    std::initializer_list<std::pair<Utf8String, QVariant> > list) {
  for (const std::pair<Utf8String,QVariant> &p : list)
    setValue(p.first, Utf8String(p.second));
}

ParamSet::ParamSet(const ParamSet &other) : d(other.d) {
}

ParamSet::ParamSet(const QHash<QString, QString> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QHash<Utf8String,Utf8String> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QMap<QString, QString> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QMap<Utf8String,Utf8String> &params)
  : d(new ParamSetData(params)) {
}

ParamSet::ParamSet(const QMultiMap<QString, QString> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QMultiHash<QString, QString> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QMultiMap<Utf8String, Utf8String> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(const QMultiHash<Utf8String, Utf8String> &params)
  : d(new ParamSetData) {
  for (auto key: params.keys())
    d->_params.insert(key, params.value(key));
}

ParamSet::ParamSet(
  const PfNode &parentnode, const Utf8String &attrname,
  const Utf8String &constattrname, const ParamSet &parent)
  : d(new ParamSetData(parent)) {
  if (!attrname.isEmpty()) {
    for (auto p: parentnode.stringsPairChildrenByName(attrname)) {
      if (p.first.isEmpty())
        continue;
      Utf8String value = p.second;
      d->_params.insert(p.first, value.isNull() ? ""_u8 : value);
    }
  }
  if (!constattrname.isEmpty()) {
    ParamSet constparams(parentnode, constattrname, Utf8String(), ParamSet());
    for (auto k: constparams.paramKeys()) {
      auto value = PercentEvaluator::escape(constparams.paramUtf8(k, this));
      d->_params.insert(k, value.isNull() ? ""_u8 : value);
    }
  }
  if (d->_params.isEmpty() && parent.isNull())
    d.reset();
}

ParamSet::ParamSet(const PfNode &parentnode, const Utf8String &attrname,
                   const ParamSet &parent)
  : ParamSet(parentnode, attrname, Utf8String(), parent) {
}

ParamSet::ParamSet(const PfNode &parentnode, const Utf8StringSet &attrnames,
  const ParamSet &parent) : d(new ParamSetData(parent)) {
  for (const PfNode &child : parentnode.children())
    if (attrnames.contains(child.name()))
      d->_params.insert(child.name(), child.contentAsString());
  if (d->_params.isEmpty() && parent.isNull())
    d.reset();
}

ParamSet::ParamSet(
    const QSqlDatabase &db, const Utf8String &sql,
    const QMap<int,Utf8String> &bindings, const ParamSet &parent)
  : d(new ParamSetData(parent)) {
  QSqlQuery query(db);
  query.prepare(parent.evaluate(sql));
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
      auto s = r.field(i).value().toString();
      if (s.isEmpty()) // ignoring both nulls and empty strings
        continue;
      values[i].append(PercentEvaluator::escape(s));
    }
  }
  for (auto i: bindings.keys()) {
    setValue(bindings.value(i), values[i].join(" "));
  }
}

ParamSet::ParamSet(
    QIODevice *input, const Utf8String &format,
    const QMap<Utf8String,Utf8String> options, const bool escape_percent,
    const ParamSet &parent)
  : d(fromQIODevice(input, format, options, escape_percent, parent)) {
}

ParamSet::~ParamSet() {
}

ParamSet &ParamSet::operator=(const ParamSet &other) {
  if (this != &other)
    d = other.d;
  return *this;
}

ParamSet ParamSet::parent() const {
  return d ? d->_parent : ParamSet();
}

void ParamSet::setParent(ParamSet parent) {
  if (!d)
    d = new ParamSetData();
  if (d.constData() != parent.d.constData())
    d->_parent = parent;
}

void ParamSet::setValue(Utf8String key, Utf8String value) {
  if (!d)
    d = new ParamSetData();
  d->_params.insert(key, value);
}

void ParamSet::setValues(ParamSet params, bool inherit) {
  if (!d)
    d = new ParamSetData();
  for (auto k: params.paramKeys(inherit))
    d->_params.insert(k, params.paramRawUtf8(k));
}

void ParamSet::removeValue(const Utf8String &key) {
  if (d)
    d->_params.remove(key);
}

void ParamSet::clear() {
  d = new ParamSetData();
}

const QVariant ParamSet::paramRawValue(
    const Utf8String &key, const QVariant &def) const {
  return paramRawValue(key, def, true);
}

const ParamSet::ScopedValue ParamSet::paramScopedRawValue(
    const Utf8String &key, const QVariant &def, bool inherit) const {
  if (!d) [[unlikely]]
    return { {}, def };
  auto value = d->_params.value(key);
  if (!value.isNull())
    return { paramScope(), value };
  if (inherit) {
    auto sv = parent().paramScopedRawValue(key);
    if (sv.isValid())
      return sv;
  }
  return { {}, def };
}

const ParamSet::ScopedValue ParamSet::paramScopedRawValue(
    const Utf8String &key, const QVariant &def) const {
  return paramScopedRawValue(key, def, true);
}

const QVariant ParamSet::paramRawValue(
    const Utf8String &key, const QVariant &def, bool inherit) const {
  return paramScopedRawValue(key, def, inherit);
}

const Utf8String ParamSet::paramRawUtf8(
    const Utf8String &key, const Utf8String &def, bool inherit) const {
  if (!d) [[unlikely]]
    return def;
  auto value = d->_params.value(key);
  if (value.isNull() && inherit)
    value = parent().paramRawUtf8(key);
  return value.isNull() ? def : value;
}

const Utf8String ParamSet::paramRawUtf8(
    const Utf8String &key, const Utf8String &def) const {
  return paramRawUtf8(key, def, true);
}

const Utf8String ParamSet::paramUtf8(
    const Utf8String &key, const Utf8String &def,
    const ParamsProvider *context, bool inherit,
    Utf8StringSet *ae) const {
  auto v = paramScopedRawValue(key, def, inherit);
  if (!v.isValid())
    return def;
  return Utf8String(PercentEvaluator::eval(Utf8String(v), context, ae));
}

const Utf8String ParamSet::paramUtf8(
    const Utf8String &key, const Utf8String &def,
    const ParamsProvider *context, bool inherit) const {
  Utf8StringSet ae;
  return paramUtf8(key, def, context, inherit, &ae);
}

const Utf8String ParamSet::paramUtf8(
    const Utf8String &key, const Utf8String &def,
    const ParamsProvider *context, Utf8StringSet *ae) const {
  return paramUtf8(key, def, context, true, ae);
}

const Utf8StringSet ParamSet::paramKeys(bool inherit) const {
  if (!d) [[unlikely]]
    return {};
  Utf8StringSet set(d->_params.keys());
  if (inherit)
    set += parent().paramKeys(true);
  return set;
}

const Utf8StringSet ParamSet::paramKeys() const {
  return paramKeys(true);
}

bool ParamSet::paramContains(const Utf8String &key, bool inherit) const {
  if (!d) [[unlikely]]
    return false;
  if (d->_params.contains(key))
    return true;
  return inherit && parent().paramContains(key, true);
}

bool ParamSet::paramContains(const Utf8String &key) const {
  return paramContains(key, true);
}

bool ParamSet::isNull() const {
  return !d;
}

int ParamSet::size() const {
  return d ? d->_params.size() : 0;
}

bool ParamSet::isEmpty() const {
  return d ? d->_params.isEmpty() : true;
}

const QString ParamSet::toString(bool inherit, bool decorate) const {
  QString s;
  if (decorate)
    s.append("{ ");
  bool first = true;
  foreach(QString key, paramKeys(inherit)) {
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

const QHash<Utf8String, Utf8String> ParamSet::toHash(bool inherit) const {
  QHash<Utf8String,Utf8String> hash;
  for (auto key: paramKeys(inherit))
    hash.insert(key, paramRawUtf8(key));
  return hash;
}

const QMap<Utf8String, Utf8String> ParamSet::toMap(bool inherit) const {
  QMap<Utf8String,Utf8String> map;
  for (auto key: paramKeys(inherit))
    map.insert(key, paramRawUtf8(key));
  return map;
}

const QHash<QString, QString> ParamSet::toStringHash(bool inherit) const {
  QHash<QString,QString> hash;
  for (auto key: paramKeys(inherit))
    hash.insert(key, paramRawUtf8(key));
  return hash;
}

const QMap<QString,QString> ParamSet::toStringMap(bool inherit) const {
  QMap<QString,QString> map;
  for (auto key: paramKeys(inherit))
    map.insert(key, paramRawUtf8(key));
  return map;
}

QDebug operator<<(QDebug dbg, const ParamSet &params) {
  dbg.nospace() << "{";
  auto keys = params.paramKeys().values();
  std::sort(keys.begin(), keys.end());
  for (auto key: keys)
    dbg.space() << key << "=" << params.paramRawUtf8(key) << ",";
  dbg.nospace() << "}";
  return dbg.space();
}

LogHelper operator<<(LogHelper lh, const ParamSet &params) {
  lh << "{ ";
  auto keys = params.paramKeys().values();
  std::sort(keys.begin(), keys.end());
  for (auto key: keys)
    lh << key << "=" << params.paramRawUtf8(key) << " ";
  return lh << "}";
}

void ParamSet::detach() {
  d.detach();
}

void ParamSet::setValuesFromSqlDb(
    const QSqlDatabase &db, const Utf8String &sql,
    const QMap<int, Utf8String> &bindings) {
  ParamSet params(db, sql, bindings, *this);
  this->setValues(params, false);
}

void ParamSet::setValuesFromSqlDb(
    const Utf8String &dbname, const Utf8String &sql,
    const QMap<int, Utf8String> &bindings) {
  setValuesFromSqlDb(QSqlDatabase::database(dbname), sql, bindings);
}

void ParamSet::setValuesFromSqlDb(
    const Utf8String &dbname, const Utf8String &sql,
    const Utf8StringList &bindings) {
  setValuesFromSqlDb(QSqlDatabase::database(dbname), sql, bindings);
}

void ParamSet::setValuesFromSqlDb(
    const QSqlDatabase &db, const Utf8String &sql,
    const Utf8StringList &bindings) {
  QMap<int,Utf8String> map;
  int i = 0;
  for (auto key: bindings)
    map.insert(i++, key);
  setValuesFromSqlDb(db, sql, map);
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
  auto separator = Utf8String(options.value("separator"_u8)).value(0,',');
  auto quote = Utf8String(options.value("quote"_u8)).value(0,'"');
  auto escape = Utf8String(options.value("escape"_u8)).value(0,'\\');
  CsvFile csvfile;
  csvfile.enableHeaders(false);
  csvfile.setFieldSeparator(separator);
  csvfile.setQuoteChar(quote);
  csvfile.setEscapeChar(escape);
  csvfile.openReadonly(input);
  auto rows = csvfile.rows();
  //qDebug() << "***password from csv" << rows << separator << quote << escape;
  for (auto row: rows) {
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
