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
#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "libp6core_global.h"
#include <QString>
#include <QDateTime>
#include <QExplicitlySharedDataPointer>
#include <QByteArray>
#include <QAbstractSocket>

using namespace Qt::Literals::StringLiterals;

class HttpResponseData;

/** Class holding all information and actions about the response to an incoming
 * HTTP request.
 * This class uses Qt explicit sharing idiom, i.e. it can be copied for a
 * very low cost in thread-safe manner, however it must not be accessed from
 * several threads at a time. */
class LIBP6CORESHARED_EXPORT HttpResponse {
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
  inline void setContentType(QByteArray type) {
    setHeader("Content-Type"_ba, type); }
  /** Syntaxic sugar for setHeader("Content-Length", length). */
  inline void setContentLength(qint64 length) {
    setHeader("Content-Length"_ba, QByteArray::number(length)); }
  /** Set HTTP status code. Default is 200.
   * Must be called before output().
   * @see StatusCode */
  void setStatus(int status);
  /** Current http status, as set by last setStatus() call */
  int status() const;
  /** Replace any header of this name by one header with this value.
   * Must be called before output(). */
  void setHeader(const QByteArray &name, const QByteArray &value);
  /** Append a header regardless one already exists with the same name.
   * Must be called before output(). */
  void addHeader(const QByteArray &name, const QByteArray &value);
  /** Append a value at the end of a header containing a separated list (e.g.
   * a comma separated list).
   * Also merge several headers if the header is already multi-valued
   * e.g. Vary: Origin followed by Vary: X-MyHeader followed by a call to
   * appendValueToHeader("Vary", "X-Login") will produce a mono-valued
   * Vary: Origin, X-MyHeader, X-Login */
  void appendValueToHeader(
      const QByteArray &name, const QByteArray &value,
      const QByteArray &separator = { ", " });
  /** Value associated to a response header.
   * If the header is found several time, last value is returned. */
  QByteArray header(
      const QByteArray &name, const QByteArray &defaultValue = {}) const;
  /** Values associated to a response header, last occurrence first. */
  QByteArrayList headers(const QByteArray &name) const;
  /** Full header hash */
  QMultiMap<QByteArray, QByteArray> headers() const;
  /** Redirect to another URL, by default using a temporary redirect (302).
   * Must be called before output(). */
  void redirect(QByteArray location, int status = HTTP_Found);
  void redirect(QString location, int status = HTTP_Found) {
    redirect(location.toUtf8(), status); }
  /** Set a session cookie
  * Some characters are not allowed in value, see RFC6265 or use
  * setBase64SessionCookie() */
  inline void setSessionCookie(
      const QByteArray &name, const QByteArray &value,
      const QByteArray &path = {}, const QByteArray &domain = {},
      bool secure = false, bool httponly = false) {
    setCookie(name, value, QDateTime(), path, domain, secure, httponly); }
  /** Set a session cookie
  * Some characters are not allowed in value, see RFC6265 or use
  * setBase64SessionCookie() */
  inline void setSessionCookie(
      const QString &name, const QString &value, const QString &path = {},
      const QString &domain = {}, bool secure = false, bool httponly = false) {
    setCookie(name.toUtf8(), value.toUtf8(), QDateTime{}, path.toUtf8(),
              domain.toUtf8(), secure, httponly); }
  /** Set a session cookie, encoding its value using base64. */
  inline void setBase64SessionCookie(
      const QByteArray &name, const QByteArray &value,
      const QByteArray &path = {}, const QByteArray &domain = {},
      bool secure = false, bool httponly = false) {
    setCookie(name, value.toBase64(), QDateTime(), path,
              domain, secure, httponly); }
  /** Set a session cookie, encoding its value using base64 and utf-8. */
  inline void setBase64SessionCookie(
      const QString &name, const QString &value, const QString &path = {},
      const QString &domain = {}, bool secure = false, bool httponly = false) {
    setCookie(name.toUtf8(), value.toUtf8().toBase64(), QDateTime{},
              path.toUtf8(), domain.toUtf8(), secure, httponly); }
  /** Set a persistent cookie.
   * Some characters are not allowed in value, see RFC6265 or use
   * setBase64PersistentCookie()
   * @param expires defaults to now + 1 day */
  inline void setPersistentCookie(
      const QByteArray &name, const QByteArray &value, QDateTime expires = {},
      const QByteArray &path = {}, const QByteArray &domain = {},
      bool secure = false, bool httponly = false) {
    setCookie(name, value,
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path, domain, secure, httponly); }
  /** Set a persistent cookie.
   * Some characters are not allowed in value, see RFC6265 or use
   * setBase64PersistentCookie()
   * @param expires defaults to now + 1 day */
  inline void setPersistentCookie(
      const QString &name, const QString &value, QDateTime expires = {},
      const QString &path = {}, const QString &domain = {}, bool secure = false,
      bool httponly = false) {
    setCookie(name.toUtf8(), value.toUtf8(),
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path.toUtf8(), domain.toUtf8(), secure, httponly); }
  /** Set a persistent cookie.
   * Some characters are not allowed in value, see RFC6265 or use
   * setBase64PersistentCookie() */
  inline void setPersistentCookie(
      const QByteArray &name, const QByteArray &value, int seconds,
      const QByteArray &path = {}, const QByteArray &domain = {},
      bool secure = false, bool httponly = false) {
    setCookie(name, value, QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly); }
  /** Set a persistent cookie.
   * Some characters are not allowed in value, see RFC6265 or use
   * setBase64PersistentCookie() */
  inline void setPersistentCookie(
      const QString &name, const QString &value, int seconds,
      const QString &path = {}, const QString &domain = {},
      bool secure = false, bool httponly = false) {
    setCookie(name.toUtf8(), value.toUtf8(),
              QDateTime::currentDateTime().addSecs(seconds),
              path.toUtf8(), domain.toUtf8(), secure, httponly); }
  /** Set a persistent cookie, encoding its value using base64.
   * @param expires defaults to now + 1 day */
  inline void setBase64PersistentCookie(
      const QByteArray &name, const QByteArray &value, QDateTime expires = {},
      const QByteArray &path = {}, const QByteArray &domain = {},
      bool secure = false, bool httponly = false) {
    setCookie(name, value.toBase64(),
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path, domain, secure, httponly); }
  /** Set a persistent cookie, encoding its value using base64 and utf-8.
   * @param expires defaults to now + 1 day */
  inline void setBase64PersistentCookie(
      const QString &name, const QString &value, QDateTime expires = {},
      const QString &path = {}, const QString &domain = {}, bool secure = false,
      bool httponly = false) {
    setCookie(name.toUtf8(), value.toUtf8().toBase64(),
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path.toUtf8(), domain.toUtf8(), secure, httponly); }
  /** Set a persistent cookie, encoding its value using base64. */
  inline void setBase64PersistentCookie(
      const QByteArray &name, const QByteArray &value, int seconds,
      const QByteArray &path = {}, const QByteArray &domain = {},
      bool secure = false, bool httponly = false) {
    setCookie(name, value.toBase64(),
              QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly); }
  /** Set a persistent cookie, encoding its value using base64 and utf-8. */
  inline void setBase64PersistentCookie(
      const QString &name, const QString &value, int seconds,
      const QString &path = {}, const QString &domain = {}, bool secure = false,
      bool httponly = false) {
    setCookie(name.toUtf8(), value.toUtf8().toBase64(),
              QDateTime::currentDateTime().addSecs(seconds),
              path.toUtf8(), domain.toUtf8(), secure, httponly); }
  /** Remove a cookie. */
  inline void clearCookie(
      const QByteArray &name, const QByteArray &path = {},
      const QByteArray &domain = {}) {
    setCookie(name, {}, QDateTime::fromMSecsSinceEpoch(0),
              path, domain, false, false); }
  /** Remove a cookie. */
  inline void clearCookie(
      const QString &name, const QString &path = {},
      const QString &domain = {}) {
    setCookie(name.toUtf8(), {}, QDateTime::fromMSecsSinceEpoch(0),
              path.toUtf8(), domain.toUtf8(), false, false); }
  // LATER session
  QByteArray statusAsString() { return statusAsString(status()); }
  static QByteArray statusAsString(int status);

private:
  void setCookie(
      const QByteArray &name, const QByteArray &value, QDateTime expires,
      const QByteArray &path, const QByteArray &domain, bool secure,
      bool httponly);
};

#endif // HTTPRESPONSE_H
