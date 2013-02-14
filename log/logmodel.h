/* Copyright 2013 Hallowyn and others.
 * This file is part of qron, see <http://qron.hallowyn.com/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QAbstractListModel>
#include "log.h"
#include <QDateTime>

class MemoryLogger;

class LIBQTSSUSHARED_EXPORT LogModel : public QAbstractListModel {
  friend class MemoryLogger;
  Q_OBJECT
public:
  static const int HtmlPrefixRole = Qt::UserRole;
  static const int TrClassRole = Qt::UserRole+1;

private:
  // LATER LogEntry should be implicitly shared
  // LATER remove old logs too
  class LogEntry {
  public:
    QString _timestamp, _message, _severityText;
    Log::Severity _severity;
    QString _task, _execId, _sourceCode;
    LogEntry(QDateTime timestamp, QString message, Log::Severity severity,
             QString task, QString execId, QString sourceCode)
      : _timestamp(timestamp.toString("yyyy-MM-ddThh:mm:ss,zzz")),
        _message(message), _severityText(Log::severityToString(severity)),
        _severity(severity), _task(task), _execId(execId),
        _sourceCode(sourceCode) { }
  };
  QList<LogEntry> _log;
  int _maxrows;
  QString _warningIcon, _errorIcon, _warningTrClass, _errorTrClass;

public:
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  void setWarningIcon(const QString rawHtml) { _warningIcon = rawHtml; }
  void setErrorIcon(const QString rawHtml) { _errorIcon = rawHtml; }
  void setWarningTrClass(const QString rawHtml) { _warningTrClass = rawHtml; }
  void setErrorTrClass(const QString rawHtml) { _errorTrClass = rawHtml; }

private:
  // only MemoryLogger can create LogModel or log to it
  // this ensures that LogModel and MemoryLogger share the same thread
  explicit LogModel(QObject *parent = 0, int maxrows = 100);
  void log(QDateTime timestamp, QString message, Log::Severity severity,
           QString task, QString execId, QString sourceCode);
};

#endif // LOGMODEL_H
