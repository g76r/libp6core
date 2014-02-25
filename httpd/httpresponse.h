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

#include "libqtssu_global.h"
#include <QString>
#include <QDateTime>
#include <QExplicitlySharedDataPointer>
#include <QByteArray>
#include <QAbstractSocket>

class HttpResponseData;

/** Class holding all information and actions about the response to an incoming
 * HTTP request.
 * This class uses Qt explicit sharing idiom, i.e. it can be copied for a
 * very low cost in thread-safe manner, however it must not be accessed from
 * several threads at a time. */
class LIBQTSSUSHARED_EXPORT HttpResponse {
private:
  QExplicitlySharedDataPointer<HttpResponseData> d;

public:
  explicit HttpResponse(QAbstractSocket *output);
  HttpResponse();
  HttpResponse(const HttpResponse &other);
  ~HttpResponse();
  HttpResponse &operator=(const HttpResponse &other);
  /** Get the output to write content; calling this method triggers sending
    * the response status and headers. */
  QAbstractSocket *output();
  /** Syntaxic sugar for setHeader("Content-Type", type).
   * Default content type is "text/plain;charset=UTF-8". */
  inline void setContentType(QString type) {
    setHeader("Content-Type", type); }
  /** Syntaxic sugar for setHeader("Content-Length", length). */
  inline void setContentLength(qint64 length) {
    setHeader("Content-Length", QString::number(length)); }
  /** Set HTTP status code. Default is 200.
   * Must be called before output(). */
  void setStatus(int status);
  /** Replace any header of this name by one header with this value.
   * Must be called before output(). */
  void setHeader(QString name, QString value);
  /** Append a header regardless one already exists with the same name.
   * Must be called before output(). */
  void addHeader(QString name, QString value);
  QMultiMap<QString,QString> headers() const;
  /** Respond with a temporary moved page (302).
   * Must be called before output(). */
  void redirect(QString location);
  /** Set a session cookie
  * Some characters are not allowed in value, see RFC6265 or use
  * setBase64SessionCookie() */
  inline void setSessionCookie(
      QString name, QString value, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, value, QDateTime(), path, domain, secure, httponly);
  }
  /** Set a session cookie
   * Any Unicode character is allowed in value. */
  inline void setBase64SessionCookie(
      QString name, QString value, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, value.toUtf8().toBase64().constData(), QDateTime(), path,
              domain, secure, httponly);
  }
  /** Set a session cookie
   * Any Unicode character is allowed in value. */
  inline void setBase64SessionCookie(
      QString name, const char *value, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, QByteArray(value).toBase64().constData(), QDateTime(), path,
              domain, secure, httponly);
  }
  /** Set a session cookie */
  inline void setBase64SessionCookie(
      QString name, QByteArray value, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, value.toBase64().constData(), QDateTime(), path, domain,
              secure, httponly);
  }
  /** Set a persistent cookie.
   * Some characters are not allowed in value, see RFC6265 or use
   * setBase64PersistentCookie()
   * @param expires defaults to now + 1 day */
  inline void setPersistentCookie(
      QString name, QString value, QDateTime expires = QDateTime(),
      QString path = QString(), QString domain = QString(), bool secure = false,
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
      QString name, QString value, int seconds, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, value, QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie.
   * Any Unicode character is allowed in value.
   * @param expires defaults to now + 1 day */
  inline void setBase64PersistentCookie(
      QString name, QString value, QDateTime expires = QDateTime(),
      QString path = QString(), QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, value.toUtf8().toBase64().constData(),
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie.
   * Any Unicode character is allowed in value.
   * @param expires defaults to now + 1 day */
  inline void setBase64PersistentCookie(
      QString name, const char *value, QDateTime expires = QDateTime(),
      QString path = QString(), QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, QByteArray(value).toBase64().constData(),
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie.
   * @param expires defaults to now + 1 day */
  inline void setBase64PersistentCookie(
      QString name, QByteArray value, QDateTime expires = QDateTime(),
      QString path = QString(), QString domain = QString(), bool secure = false,
      bool httponly = false) {
    setCookie(name, value.toBase64().constData(),
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie.
   * Any Unicode character is allowed in value. */
  inline void setBase64PersistentCookie(
      QString name, QString value, int seconds, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, value.toUtf8().toBase64().constData(),
              QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie.
   * Any Unicode character is allowed in value. */
  inline void setBase64PersistentCookie(
      QString name, const char *value, int seconds, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, QByteArray(value).toBase64().constData(),
              QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie. */
  inline void setBase64PersistentCookie(
      QString name, QByteArray value, int seconds, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, value.toBase64().constData(),
              QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly);
  }
  /** Remove a cookie. */
  inline void clearCookie(QString name, QString path = QString(),
                          QString domain = QString()) {
    setCookie(name, QString(), QDateTime::fromMSecsSinceEpoch(0),
              path, domain, false, false);
  }
  // LATER session

private:
  void setCookie(QString name, QString value, QDateTime expires, QString path,
                 QString domain, bool secure, bool httponly);
};

#endif // HTTPRESPONSE_H
