/* Copyright 2013-2016 Hallowyn and others.
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
#include "qterrorcodes.h"
#include <QObject>

QString networkErrorAsString(QNetworkReply::NetworkError code) {
  switch(code) {
  case QNetworkReply::NoError:
    return QObject::tr("no error");
  case QNetworkReply::ConnectionRefusedError:
    return QObject::tr("connection refused");
  case QNetworkReply::RemoteHostClosedError:
    return QObject::tr("remote host closed");
  case QNetworkReply::HostNotFoundError:
    return QObject::tr("host not found");
  case QNetworkReply::TimeoutError:
    return QObject::tr("timeout");
  case QNetworkReply::OperationCanceledError:
    return QObject::tr("operation canceled");
  case QNetworkReply::SslHandshakeFailedError:
    return QObject::tr("SSL handshake failed");
  case QNetworkReply::TemporaryNetworkFailureError:
    return QObject::tr("temporary network failure");
  case QNetworkReply::NetworkSessionFailedError:
    return QObject::tr("network session failed");
  case QNetworkReply::BackgroundRequestNotAllowedError:
    return QObject::tr("background request not allowed");
#if QT_VERSION >= 0x050600
  case QNetworkReply::TooManyRedirectsError:
    return QObject::tr("too many redirects");
  case QNetworkReply::InsecureRedirectError:
    return QObject::tr("insecure redirect");
#endif
  case QNetworkReply::ProxyConnectionRefusedError:
    return QObject::tr("proxy connection refused");
  case QNetworkReply::ProxyConnectionClosedError:
    return QObject::tr("proxy connection closed");
  case QNetworkReply::ProxyNotFoundError:
    return QObject::tr("proxy not found");
  case QNetworkReply::ProxyTimeoutError:
    return QObject::tr("proxy timeout");
  case QNetworkReply::ProxyAuthenticationRequiredError:
    return QObject::tr("proxy authentication required");
  case QNetworkReply::ContentAccessDenied:
    return QObject::tr("content access denied");
  case QNetworkReply::ContentOperationNotPermittedError:
    return QObject::tr("content operation not permitted");
  case QNetworkReply::ContentNotFoundError:
    return QObject::tr("content not found");
  case QNetworkReply::AuthenticationRequiredError:
    return QObject::tr("authentication required");
  case QNetworkReply::ContentReSendError:
      return QObject::tr("content resend");
  case QNetworkReply::ContentConflictError:
      return QObject::tr("content conflict");
  case QNetworkReply::ContentGoneError:
      return QObject::tr("content gone");
  case QNetworkReply::InternalServerError:
      return QObject::tr("internal server error");
  case QNetworkReply::OperationNotImplementedError:
      return QObject::tr("operation not implemented");
  case QNetworkReply::ServiceUnavailableError:
      return QObject::tr("service unavailable");
  case QNetworkReply::ProtocolUnknownError:
      return QObject::tr("protocol unknown");
  case QNetworkReply::ProtocolInvalidOperationError:
    return QObject::tr("protocol invalid operation");
  case QNetworkReply::UnknownNetworkError:
    return QObject::tr("unknown network error");
  case QNetworkReply::UnknownProxyError:
    return QObject::tr("unknown proxy error");
  case QNetworkReply::UnknownContentError:
    return QObject::tr("unknown content error");
  case QNetworkReply::ProtocolFailure:
    return QObject::tr("protocol failure");
  case QNetworkReply::UnknownServerError:
      return QObject::tr("unknown server error");

  }
  return QObject::tr("unknown error %d").arg(code);
}
