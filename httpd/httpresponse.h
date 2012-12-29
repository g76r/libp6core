/* Copyright 2012 Hallowyn and others.
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
  // LATER cookies, session
};

#endif // HTTPRESPONSE_H
