/* Copyright 2012-2025 Hallowyn, Gregoire Barbier and others.
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

#include "util/paramsprovider.h"
#include <QAbstractSocket>
#include <QDateTime>

class HttpResponseData;

/** Class holding all information and actions about the response to an incoming
 * HTTP request.
 * This class uses Qt explicit sharing idiom, i.e. it can be copied for a
 * very low cost in thread-safe manner, however it must not be accessed from
 * several threads at a time. */
class LIBP6CORESHARED_EXPORT HttpResponse : public ParamsProvider {
public:
  enum WellKnownStatusCode : signed short {
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
    HTTP_Request_Entity_Too_Large,
    HTTP_Request_URI_Too_Long,
    HTTP_Unsupported_Media_Type,
    HTTP_Request_Range_Unsatisfiable, // bad Range: header
    HTTP_Expectation_Failed = 417,
    HTTP_I_Am_A_Teapot = 418, // RFC 2324
    HTTP_Page_Expired,
    HTTP_Bad_Mapping_Misdirected = 421,
    HTTP_Unprocessable_Entity = 422, // WebDAV
    HTTP_Locked, // WebDAV
    HTTP_Method_Failure, // WebDAV
    HTTP_Unordered_Collection, // WebDAV RFC 3648 == Too Early RFC 8470
    HTTP_Upgrade_Required = 426, // RFC 2817
    HTTP_Invalid_Digital_Signature, // Microsoft
    HTTP_Precondition_Required, // RFC 6585
    HTTP_Too_Many_Requests, // RFC 6585
    HTTP_Request_Header_Fields_Too_Large, // RFC 6585
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
  void disable_body_output();
  /** Syntaxic sugar for set_header("Content-Type", type).
   * Default content type is "text/plain;charset=UTF-8". */
  inline void set_content_type(const Utf8String &type) {
    set_header("Content-Type"_u8, type); }
  /** Syntaxic sugar for set_header("Content-Length", length). */
  inline void set_content_length(qint64 length) {
    set_header("Content-Length"_u8, Utf8String::number(length)); }
  /** Set HTTP status code. Default is 200.
   * Must be called before output().
   * @see StatusCode */
  void set_status(int status);
  /** Current http status, as set by last setStatus() call */
  [[nodiscard]] int status() const;
  /** True iff status() >= 100 && < 300 */
  [[nodiscard]] inline bool success() const {
    auto s = status();
    return s >= 100 && s < 400; }
  /** Replace any header of this name by one header with this value.
   * Must be called before output(). */
  void set_header(const Utf8String &name, const Utf8String &value);
  /** Append a header regardless one already exists with the same name.
   * Must be called before output(). */
  void add_header(const Utf8String &name, const Utf8String &value);
  /** Append a value at the end of a header containing a separated list (e.g.
   * a comma separated list).
   * Also merge several headers if the header is already multi-valued
   * e.g. Vary: Origin followed by Vary: X-MyHeader followed by a call to
   * append_value_to_header("Vary", "X-Login") will produce a mono-valued
   * Vary: Origin, X-MyHeader, X-Login */
  void append_value_to_header(
      const Utf8String &name, const Utf8String &value,
      const Utf8String &separator = ", "_u8);
  /** Value associated to a response header.
   * If the header is found several time, last value is returned. */
  [[nodiscard]] Utf8String header(
      const Utf8String &name, const Utf8String &def = {}) const;
  /** Values associated to a response header, last occurrence first. */
  [[nodiscard]] Utf8StringList headers(const Utf8String &name) const;
  /** Redirect to another URL, by default using a temporary redirect (302).
   * Must be called before output(). */
  void redirect(Utf8String location, int status = HTTP_Found);
  /** Set a session cookie
  * Some characters are not allowed in value, see RFC6265 or use
  * setBase64SessionCookie() */
  inline void set_session_cookie(
      const Utf8String &name, const Utf8String &value,
      const Utf8String &path = {}, const Utf8String &domain = {},
      bool secure = false, bool httponly = false) {
    set_cookie(name, value, QDateTime(), path, domain, secure, httponly); }
  /** Set a session cookie, encoding its value using base64. */
  inline void set_base64_session_cookie(
      const Utf8String &name, const Utf8String &value,
      const Utf8String &path = {}, const Utf8String &domain = {},
      bool secure = false, bool httponly = false) {
    set_cookie(name, value.toBase64(), QDateTime(), path,
              domain, secure, httponly); }
  /** Set a persistent cookie.
   * Some characters are not allowed in value, see RFC6265 or use
   * setBase64PersistentCookie()
   * @param expires defaults to now + 1 day */
  inline void set_persistent_cookie(
      const Utf8String &name, const Utf8String &value, QDateTime expires = {},
      const Utf8String &path = {}, const Utf8String &domain = {},
      bool secure = false, bool httponly = false) {
    set_cookie(name, value,
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path, domain, secure, httponly); }
  /** Set a persistent cookie.
   * Some characters are not allowed in value, see RFC6265 or use
   * setBase64PersistentCookie() */
  inline void set_persistent_cookie(
      const Utf8String &name, const Utf8String &value, int seconds,
      const Utf8String &path = {}, const Utf8String &domain = {},
      bool secure = false, bool httponly = false) {
    set_cookie(name, value, QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly); }
  /** Set a persistent cookie, encoding its value using base64.
   * @param expires defaults to now + 1 day */
  inline void set_base64_persistent_cookie(
      const Utf8String &name, const Utf8String &value, QDateTime expires = {},
      const Utf8String &path = {}, const Utf8String &domain = {},
      bool secure = false, bool httponly = false) {
    set_cookie(name, value.toBase64(),
              expires.isNull() ? QDateTime::currentDateTime().addSecs(86400)
                               : expires,
              path, domain, secure, httponly); }
  /** Set a persistent cookie, encoding its value using base64. */
  inline void set_base64_persistent_cookie(
      const Utf8String &name, const Utf8String &value, int seconds,
      const Utf8String &path = {}, const Utf8String &domain = {},
      bool secure = false, bool httponly = false) {
    set_cookie(name, value.toBase64(),
              QDateTime::currentDateTime().addSecs(seconds),
              path, domain, secure, httponly); }
  /** Remove a cookie. */
  inline void unset_cookie(
      const Utf8String &name, const Utf8String &path = {},
      const Utf8String &domain = {}) {
    set_cookie(name, {}, QDateTime::fromMSecsSinceEpoch(0),
              path, domain, false, false); }
  // LATER session
  [[nodiscard]] Utf8String status_as_text() { return status_as_text(status()); }
  [[nodiscard]] static Utf8String status_as_text(int status);
  [[nodiscard]] QDateTime received_date() const;
  void set_handled_date(const QDateTime &ts = QDateTime::currentDateTime());
  [[nodiscard]] QDateTime handled_date() const;
  void set_flushed_date(const QDateTime &ts = QDateTime::currentDateTime());
  [[nodiscard]] QDateTime flushed_date() const;
  [[nodiscard]] qint64 servicems() const;
  [[nodiscard]] qint64 handlingms() const;
  /** Expose as a ParamsProvider the following data/metadata:
   *  - status e.g. 200
   *  - header:xxx e.g. header:Content-Type -> "text/plain"
   *  - responseheader:xxx e.g. responseheader:Content-Type -> "text/plain"
   *  - receiveddate
   *  - handleddate
   *  - flusheddate
   *  - servicems
   *  - handlingms
   */
  [[nodiscard]] QVariant paramRawValue(
      const Utf8String &key, const QVariant &def = {},
      const EvalContext &context = {}) const override;
  using ParamsProvider::paramKeys;
  [[nodiscard]] Utf8StringSet paramKeys(
      const EvalContext &context = {}) const override;
  using ParamsProvider::paramScope;
  /** Default: "http" */
  [[nodiscard]] Utf8String paramScope() const override;
  /** Set param scope to something else than the default "http". */
  HttpResponse &setScope(const Utf8String &scope);

private:
  void set_cookie(
      const Utf8String &name, const Utf8String &value, QDateTime expires,
      const Utf8String &path, const Utf8String &domain, bool secure,
      bool httponly);
};

Q_DECLARE_METATYPE(HttpResponse)
Q_DECLARE_TYPEINFO(HttpResponse, Q_RELOCATABLE_TYPE);

#endif // HTTPRESPONSE_H
