/* Copyright 2013-2025 Hallowyn, Gregoire Barbier and others.
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
#include "qterrorcodes.h"

QString networkErrorAsString(QNetworkReply::NetworkError code) {
  switch(code) {
    using enum QNetworkReply::NetworkError;
    case NoError:
      return QObject::tr("no error");
    case ConnectionRefusedError:
      return QObject::tr("connection refused");
    case RemoteHostClosedError:
      return QObject::tr("remote host closed");
    case HostNotFoundError:
      return QObject::tr("host not found");
    case TimeoutError:
      return QObject::tr("timeout");
    case OperationCanceledError:
      return QObject::tr("operation canceled");
    case SslHandshakeFailedError:
      return QObject::tr("SSL handshake failed");
    case TemporaryNetworkFailureError:
      return QObject::tr("temporary network failure");
    case NetworkSessionFailedError:
      return QObject::tr("network session failed");
    case BackgroundRequestNotAllowedError:
      return QObject::tr("background request not allowed");
    case TooManyRedirectsError:
      return QObject::tr("too many redirects");
    case InsecureRedirectError:
      return QObject::tr("insecure redirect");
    case ProxyConnectionRefusedError:
      return QObject::tr("proxy connection refused");
    case ProxyConnectionClosedError:
      return QObject::tr("proxy connection closed");
    case ProxyNotFoundError:
      return QObject::tr("proxy not found");
    case ProxyTimeoutError:
      return QObject::tr("proxy timeout");
    case ProxyAuthenticationRequiredError:
      return QObject::tr("proxy authentication required");
    case ContentAccessDenied:
      return QObject::tr("content access denied");
    case ContentOperationNotPermittedError:
      return QObject::tr("content operation not permitted");
    case ContentNotFoundError:
      return QObject::tr("content not found");
    case AuthenticationRequiredError:
      return QObject::tr("authentication required");
    case ContentReSendError:
      return QObject::tr("content resend");
    case ContentConflictError:
      return QObject::tr("content conflict");
    case ContentGoneError:
      return QObject::tr("content gone");
    case InternalServerError:
      return QObject::tr("internal server error");
    case OperationNotImplementedError:
      return QObject::tr("operation not implemented");
    case ServiceUnavailableError:
      return QObject::tr("service unavailable");
    case ProtocolUnknownError:
      return QObject::tr("protocol unknown");
    case ProtocolInvalidOperationError:
      return QObject::tr("protocol invalid operation");
    case UnknownNetworkError:
      return QObject::tr("unknown network error");
    case UnknownProxyError:
      return QObject::tr("unknown proxy error");
    case UnknownContentError:
      return QObject::tr("unknown content error");
    case ProtocolFailure:
      return QObject::tr("protocol failure");
    case UnknownServerError:
      return QObject::tr("unknown server error");
  }
  return QObject::tr("unknown error %d").arg(code);
}
