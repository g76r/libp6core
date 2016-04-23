/* Copyright 2012-2016 Hallowyn and others.
 * This file is part of libqtssu, see <https://gitlab.com/g76r/libqtssu>.
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
public:
  enum WellKnownStatusCode {
    Unknown,
    HTTP_Continue = 100,
    HTTP_Switching_Protocols,
    HTTP_Ok = 200,
    HTTP_Created,
    HTTP_Accepted,
    HTTP_Non_Authoritative_Information,
    HTTP_No_Content,
    HTTP_Reset_Content,
    HTTP_Partial_Content,
    HTTP_Multi_Status, // WebDAV
    HTTP_Content_Different = 210, // WebDAV
    HTTP_Multiple_Choices = 300,
    HTTP_Moved_Permanently,
    HTTP_Found,
    HTTP_See_Other,
    HTTP_Not_Modified,
    HTTP_Use_Proxy,
    HTTP_Switch_Proxy,
    HTTP_Temporary_Redirect,
    HTTP_Permanent_Redirect,
    HTTP_Too_Many_Redirects = 310,
    HTTP_Bad_Request = 400,
    HTTP_Authentication_Required,
    HTTP_Payment_Required,
    HTTP_Forbidden,
    HTTP_Not_Found,
    HTTP_Method_Not_Allowed,
    HTTP_Not_Acceptable,
    HTTP_Proxy_Authentication_Required,
    HTTP_Request_Timeout = 408,
    HTTP_Conflict,
    HTTP_Gone,
    HTTP_Length_Required,
    HTTP_Precondition_Failed,
    HTTP_Request_Representation_Too_Large,
    HTTP_URI_Too_Long,
    HTTP_Unsupported_Media_Type,
    HTTP_Expectation_Failed = 417,
    HTTP_I_Am_Teapot = 418, // RFC 2324
    HTTP_Unprocessable_Entity = 422, // WebDAV
    HTTP_Locked, // WebDAV
    HTTP_Method_Failure, // WebDAV
    HTTP_Unordered_Collection, // WebDAV RFC 3648
    HTTP_Upgrade_Required = 426, // RFC 2817
    HTTP_Unavailable_For_Legal_Reasons = 451, // RFC 7725
    HTTP_Internal_Server_Error = 500,
    HTTP_Not_Implemented,
    HTTP_Bad_Gateway,
    HTTP_Service_Unavailable,
    HTTP_Gateway_Timeout,
    HTTP_Version_Not_Supported,
    HTTP_Variant_Also_Negociates,
    HTTP_Insufficient_Storage, // WebDAV
    HTTP_Loop_Detected, // WebDAV
    HTTP_Bandwidth_Limit_Exceeded
  };

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
  /** Further calls to output() will return a dummy socket to disable sending
   * data to the client. This method is only intended to be called by HttpWorker
   * when processing a HEAD request, to disable naive HttpHandlers from sending
   * a body in response to a HEAD request. */
  void disableBodyOutput();
  /** Syntaxic sugar for setHeader("Content-Type", type).
   * Default content type is "text/plain;charset=UTF-8". */
  inline void setContentType(QString type) {
    setHeader(QStringLiteral("Content-Type"), type); }
  /** Syntaxic sugar for setHeader("Content-Length", length). */
  inline void setContentLength(qint64 length) {
    setHeader(QStringLiteral("Content-Length"), QString::number(length)); }
  /** Set HTTP status code. Default is 200.
   * Must be called before output().
   * @see StatusCode */
  void setStatus(int status);
  /** Current http status, as set by last setStatus() call */
  int status() const;
  /** Replace any header of this name by one header with this value.
   * Must be called before output(). */
  void setHeader(QString name, QString value);
  /** Append a header regardless one already exists with the same name.
   * Must be called before output(). */
  void addHeader(QString name, QString value);
  /** Value associated to a response header.
   * If the header is found several time, last value is returned. */
  QString header(QString name, QString defaultValue = QString()) const;
  /** Values associated to a response header, last occurrence first. */
  QStringList headers(QString name) const;
  /** Full header hash */
  QMultiHash<QString,QString> headers() const;
  /** Redirect to another URL, by default using a temporary redirect (302).
   * Must be called before output(). */
  void redirect(QString location, int status = HTTP_Found);
  /** Set a session cookie
  * Some characters are not allowed in value, see RFC6265 or use
  * setBase64SessionCookie() */
  inline void setSessionCookie(
      QString name, QString value, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, value, QDateTime(), path, domain, secure, httponly);
  }
  /** Set a session cookie, encoding its value using base64 and utf-8. */
  inline void setBase64SessionCookie(
      QString name, QString value, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, value.toUtf8().toBase64().constData(), QDateTime(), path,
              domain, secure, httponly);
  }
  /** Set a session cookie, encoding its value using base64 and assuming char*
   * array is already encoded using utf-8 (or of course 7 bits ascii). */
  inline void setBase64SessionCookie(
      QString name, const char *value, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, QByteArray(value).toBase64().constData(), QDateTime(), path,
              domain, secure, httponly);
  }
  /** Set a session cookie, encoding its value using base64 */
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
  /** Set a persistent cookie, encoding its value using base64 and utf-8.
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
  /** Set a persistent cookie, encoding its value using base64 and assuming char*
   * array is already encoded using utf-8 (or of course 7 bits ascii).
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
  /** Set a persistent cookie, encoding its value using base64.
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
  /** Set a persistent cookie, encoding its value using base64 and utf-8. */
  inline void setBase64PersistentCookie(
      QString name, QString value, int seconds, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, value.toUtf8().toBase64().constData(),
              QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie, encoding its value using base64 and assuming
   * char* array is already encoded using utf-8 (or of course 7 bits ascii). */
  inline void setBase64PersistentCookie(
      QString name, const char *value, int seconds, QString path = QString(),
      QString domain = QString(), bool secure = false, bool httponly = false) {
    setCookie(name, QByteArray(value).toBase64().constData(),
              QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly);
  }
  /** Set a persistent cookie, encoding its value using base64. */
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
  QString statusAsString() { return statusAsString(status()); }
  static QString statusAsString(int status);

private:
  void setCookie(QString name, QString value, QDateTime expires, QString path,
                 QString domain, bool secure, bool httponly);
};

#endif // HTTPRESPONSE_H
