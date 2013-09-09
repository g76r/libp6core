/* Copyright 2012-2013 Hallowyn and others.
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
#ifndef MAILSENDER_H
#define MAILSENDER_H

#include "libqtssu_global.h"
#include <QVariant>
#include <QString>
#include <QUrl>
#include <QList>
#include <QHash>

class LIBQTSSUSHARED_EXPORT MailSender {
  QUrl _url;

public:
  explicit MailSender(QUrl url);
  explicit MailSender(QString url);
  /** @return true only if SMTP server accepted to queue the mail */
  bool send(QString sender, QList<QString> recipients, QVariant body,
            QHash<QString, QString> headers, QList<QVariant> attachments,
            QString &errorString);
  inline bool send(QString sender, QList<QString> recipients, QVariant body,
                   QHash<QString, QString> headers,
                   QList<QVariant> attachments) {
    QString errorString;
    return send(sender, recipients, body, headers, attachments, errorString);
  }
  inline bool send(const QString sender, const QList<QString> recipients,
                   const QVariant body, const QHash<QString, QString> headers) {
    QString errorString;
    return send(sender, recipients, body, headers, QList<QVariant>(),
                errorString);
  }
  inline bool send(const QString sender, const QList<QString> recipients,
                   const QVariant body) {
    QString errorString;
    return send(sender, recipients, body, QHash<QString,QString>(),
                QList<QVariant>(), errorString);
  }
};

#endif // MAILSENDER_H
