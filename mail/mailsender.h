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
#include <QMap>

class LIBQTSSUSHARED_EXPORT MailSender {
  QUrl _url;

public:
  MailSender(const QUrl &url);
  MailSender(const QString &url);
  /** @return true only if SMTP server accepted to queue the mail */
  bool send(const QString sender, const QList<QString> recipients,
            const QVariant body, const QMap<QString, QString> headers,
            const QList<QVariant> attachments, QString &errorString);
  inline bool send(const QString sender, const QList<QString> recipients,
                   const QVariant body,
                   const QMap<QString, QString> headers = QMap<QString,QString>(),
                   const QList<QVariant> attachments = QList<QVariant>()) {
    QString errorString;
    return send(sender, recipients, body, headers, attachments, errorString);
  }
};

#endif // MAILSENDER_H
