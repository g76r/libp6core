/* Copyright 2013 Hallowyn and others.
 * This file is part of qron, see <http://qron.hallowyn.com/>.
 * Qron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Qron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with qron. If not, see <http://www.gnu.org/licenses/>.
 */
#include "qterrorcodes.h"

QString networkErrorAsString(QNetworkReply::NetworkError code) {
  switch(code) {
  case QNetworkReply::NoError:
    return "no error";
  case QNetworkReply::ConnectionRefusedError:
    return "connection refused";
  case QNetworkReply::RemoteHostClosedError:
    return "remote host closed";
  case QNetworkReply::HostNotFoundError:
    return "host not found";
  case QNetworkReply::TimeoutError:
    return "timeout";
  case QNetworkReply::OperationCanceledError:
    return "operation canceled";
  case QNetworkReply::SslHandshakeFailedError:
    return "SSL handshake failed";
  case QNetworkReply::TemporaryNetworkFailureError:
    return "temporary network failure";
  case QNetworkReply::ProxyConnectionRefusedError:
    return "proxy connection refused";
  case QNetworkReply::ProxyConnectionClosedError:
    return "proxy connection closed";
  case QNetworkReply::ProxyNotFoundError:
    return "proxy not found";
  case QNetworkReply::ProxyTimeoutError:
    return "proxy timeout";
  case QNetworkReply::ProxyAuthenticationRequiredError:
    return "proxy authentication required";
  case QNetworkReply::ContentAccessDenied:
    return "content access denied";
  case QNetworkReply::ContentOperationNotPermittedError:
    return "content operation not permitted";
  case QNetworkReply::ContentNotFoundError:
    return "content not found";
  case QNetworkReply::AuthenticationRequiredError:
    return "authentication required";
  case QNetworkReply::ContentReSendError:
    return "content resend";
  case QNetworkReply::ProtocolUnknownError:
    return "protocol unknown";
  case QNetworkReply::ProtocolInvalidOperationError:
    return "protocol invalid operation";
  case QNetworkReply::UnknownNetworkError:
    return "unknown network error";
  case QNetworkReply::UnknownProxyError:
    return "unknown proxy error";
  case QNetworkReply::UnknownContentError:
    return "unknown content error";
   /* protocol */
  case QNetworkReply::ProtocolFailure:
    return "protocol failure";
  }
  return "unknown error";
}
