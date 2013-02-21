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
#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <QString>
#include <QAbstractSocket>
#include "libqtssu_global.h"
#include <QDateTime>

class LIBQTSSUSHARED_EXPORT HttpResponse {
private:
  QAbstractSocket *_output;
  int _status;
  bool _headersSent;
  QMultiMap<QString,QString> _headers;

public:
  HttpResponse(QAbstractSocket *output);
  /** Get the output to write content; calling this method triggers sending
    * the response status and headers.
    */
  QAbstractSocket *output();
  /** Syntaxic sugar for setHeader("Content-Type", type).
   * Default content type is "text/plain;charset=UTF-8".
   */
  inline void setContentType(const QString type) {
    setHeader("Content-Type", type); }
  /** Syntaxic sugar for setHeader("Content-Length", length).
   */
  inline void setContentLength(qint64 length) {
    setHeader("Content-Length", QString::number(length)); }
  /** Set HTTP status code. Default is 200.
   * Must be called before output().
   */
  void setStatus(int status);
  /** Replace any header of this name by one header with this value.
   * Must be called before output().
   */
  void setHeader(const QString name, const QString value);
  /** Append a header regardless one already exists with the same name.
   * Must be called before output().
   */
  void addHeader(const QString name, const QString value);
  inline const QMultiMap<QString,QString> headers() const { return _headers; }
  /** Respond with a temporary moved page (302).
   * Must be called before output().
   */
  void redirect(const QString location);
  /** Set a session cookie
  * Some characters are not allowed in value, see RFC6265 or use
  * setBase64SessionCookie() */
  inline void setSessionCookie(
      const QString name, const QString value, const QString path = QString(),
      const QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, value, QDateTime(), path, domain, secure, httponly);
  }
  /** Set a session cookie
   * Any Unicode character is allowed in value. */
  inline void setBase64SessionCookie(
      const QString name, const QString value, const QString path = QString(),
      const QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, value.toUtf8().toBase64().constData(), QDateTime(), path,
              domain, secure, httponly);
  }
  /** Set a session cookie
   * Any Unicode character is allowed in value. */
  inline void setBase64SessionCookie(
      const QString name, const char *value, const QString path = QString(),
      const QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, QByteArray(value).toBase64().constData(), QDateTime(), path,
              domain, secure, httponly);
  }
  /** Set a session cookie */
  inline void setBase64SessionCookie(
      const QString name, const QByteArray value,
      const QString path = QString(), const QString domain = QString(),
      bool secure = false, bool httponly = false) {
    setCookie(name, value.toBase64().constData(), QDateTime(), path, domain,
              secure, httponly);
  }
  /** Set a persistent cookie.
   * Some characters are not allowed in value, see RFC6265 or use
   * setBase64PersistentCookie()
   * @param expires defaults to now + 1 day
   */
  inline void setPersistentCookie(
      const QString name, const QString value,
      const QDateTime expires = QDateTime(), const QString path = QString(),
      const QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, value,
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie.
   * Some characters are not allowed in value, see RFC6265 or use
   * setBase64PersistentCookie() */
  inline void setPersistentCookie(
      const QString name, const QString value, int seconds,
      const QString path = QString(), const QString domain = QString(),
      bool secure = false, bool httponly = false) {
    setCookie(name, value, QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie.
   * Any Unicode character is allowed in value.
   * @param expires defaults to now + 1 day
   */
  inline void setBase64PersistentCookie(
      const QString name, const QString value,
      const QDateTime expires = QDateTime(), const QString path = QString(),
      const QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, value.toUtf8().toBase64().constData(),
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie.
   * Any Unicode character is allowed in value.
   * @param expires defaults to now + 1 day
   */
  inline void setBase64PersistentCookie(
      const QString name, const char *value,
      const QDateTime expires = QDateTime(), const QString path = QString(),
      const QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, QByteArray(value).toBase64().constData(),
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie.
   * @param expires defaults to now + 1 day
   */
  inline void setBase64PersistentCookie(
      const QString name, const QByteArray value,
      const QDateTime expires = QDateTime(), const QString path = QString(),
      const QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, value.toBase64().constData(),
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie.
   * Any Unicode character is allowed in value. */
  inline void setBase64PersistentCookie(
      const QString name, const QString value,
      int seconds, const QString path = QString(),
      const QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, value.toUtf8().toBase64().constData(),
              QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie.
   * Any Unicode character is allowed in value. */
  inline void setBase64PersistentCookie(
      const QString name, const char *value,
      int seconds, const QString path = QString(),
      const QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, QByteArray(value).toBase64().constData(),
              QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie. */
  inline void setBase64PersistentCookie(
      const QString name, const QByteArray value,
      int seconds, const QString path = QString(),
      const QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, value.toBase64().constData(),
              QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly);
  }

  /** Remove a cookie. */
  inline void clearCookie(const QString name, const QString path = QString(),
                          const QString domain = QString()) {
    setCookie(name, QString(), QDateTime::currentDateTime().addDays(-2).toUTC(),
              path, domain, false, false);
  }

  // LATER session

private:
  void setCookie(const QString name, const QString value,
                 const QDateTime expires, const QString path,
                 const QString domain, bool secure, bool httponly);
};

#endif // HTTPRESPONSE_H
