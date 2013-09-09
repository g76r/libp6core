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

/** Model used by MemoryLoger. Contains a log entry per row, the first row
 * being the last recorded entry.
 * @see MemoryLogger */
class LIBQTSSUSHARED_EXPORT LogModel : public QAbstractListModel {
  friend class MemoryLogger;
  Q_OBJECT
  Q_DISABLE_COPY(LogModel)

public:
  static const int HtmlPrefixRole = Qt::UserRole;
  static const int TrClassRole = Qt::UserRole+1;

private:
  // LATER remove old logs too
  class LogEntryData;
  class LogEntry {
    QSharedPointer<LogEntryData> d;
  public:
    LogEntry(QDateTime timestamp, QString message, Log::Severity severity,
             QString task, QString execId, QString sourceCode);
    LogEntry();
    LogEntry(const LogEntry &o);
    ~LogEntry();
    LogEntry &operator=(const LogEntry &o);
    QString timestamp() const;
    QString message() const;
    Log::Severity severity() const;
    QString severityText() const;
    QString task() const;
    QString execId() const;
    QString sourceCode() const;
  };
  QList<LogEntry> _log;
  int _maxrows;
  QString _warningIcon, _errorIcon, _warningTrClass, _errorTrClass;

public:
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  void setWarningIcon(QString rawHtml) { _warningIcon = rawHtml; }
  void setErrorIcon(QString rawHtml) { _errorIcon = rawHtml; }
  void setWarningTrClass(QString rawHtml) { _warningTrClass = rawHtml; }
  void setErrorTrClass(QString rawHtml) { _errorTrClass = rawHtml; }

private:
  // only MemoryLogger can create LogModel or log to it
  // this ensures that LogModel and MemoryLogger share the same thread
  explicit LogModel(QObject *parent = 0, int maxrows = 100);
  void log(QDateTime timestamp, QString message, Log::Severity severity,
           QString task, QString execId, QString sourceCode);
};

#endif // LOGMODEL_H
