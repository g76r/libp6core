/* Copyright 2012-2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
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
#include "log/log.h"
#include "paramsprovider.h"

class ParamSetData;

/** String key-value parameter set with inheritance, substitution macro-language
 * and syntaxic sugar for converting values to non-string types. */
class LIBQTSSUSHARED_EXPORT ParamSet : public ParamsProvider {
  QSharedDataPointer<ParamSetData> d;
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
  /** Split string and perform parameters substitution. */
  QStringList splitAndEvaluate(
      QString rawValue, QString separator = " ", bool inherit = true,
      const ParamsProvider *context = 0) const {
    return splitAndEvaluate(rawValue, separator, inherit, context,
                            QSet<QString>());
  }
  QStringList splitAndEvaluate(QString rawValue,
                               const ParamsProvider *context) const {
    return splitAndEvaluate(rawValue, " ", true, context); }
  /** Return a globing expression that matches any string that can result
   * in evaluation of the rawValue (@see QRegExp::Wildcard).
   * For instance "foo%!yyyy-%{bar}" is converted into "foo????-*" or
   * into "foo*-*". */
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

private:
  inline bool appendVariableValue(
      QString *value, QString variable, bool inherit,
      const ParamsProvider *context, QSet<QString> alreadyEvaluated,
      bool logIfVariableNotFound) const;
  inline QString evaluateImplicitVariable(
      QString key, bool inherit, const ParamsProvider *context,
      QSet<QString> alreadyEvaluated) const;
  QString evaluate(QString rawValue, bool inherit,
                   const ParamsProvider *context,
                   QSet<QString> alreadyEvaluated) const;
  QStringList splitAndEvaluate(
      QString rawValue, QString separator, bool inherit,
      const ParamsProvider *context, QSet<QString> alreadyEvaluated) const;
};

QDebug LIBQTSSUSHARED_EXPORT operator<<(QDebug dbg, const ParamSet &params);

LogHelper LIBQTSSUSHARED_EXPORT operator<<(LogHelper lh,
                                            const ParamSet &params);

#endif // PARAMSET_H
