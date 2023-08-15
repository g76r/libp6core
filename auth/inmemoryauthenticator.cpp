/* Copyright 2013-2023 Hallowyn, Gregoire Barbier and others.
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
#include "inmemoryauthenticator.h"
#include <QMutexLocker>
#include <QRegularExpression>
#include <QCryptographicHash>

/*
 Examples of encoded passwords:
 MDP=test; SALT=greg; echo -n $MDP$SALT|sha1sum| cut -f1 -d' '|(xxd -p -r; echo -n $SALT) |base64
 KUbmLRQlC8vtgAavqEbbr2RfAXVncmVn
 */

static QRegularExpression _openLdapHashFormat{
  "\\A\\s*\\{([^\\}]+)\\}(\\S*)\\s*\\z" };

class InMemoryAuthenticator::User {
  class Data : public QSharedData {
  public:
    QString _userId;
    QString _encodedPassword;
    InMemoryAuthenticator::Encoding _encoding;
    Data() { }
    Data(QString userId, QString encodedPassword,
         InMemoryAuthenticator::Encoding encoding)
      : _userId(userId), _encodedPassword(encodedPassword),
        _encoding(encoding) { }
  };
  QSharedDataPointer<Data> d;

public:
  User() { }
  User(QString userId, QString encodedPassword,
       InMemoryAuthenticator::Encoding encoding) {
    if (encoding == OpenLdapStyle) {
      auto m = _openLdapHashFormat.match(encodedPassword);
      if (m.hasMatch()) {
        QString algo = m.captured(1).trimmed().toUpper();
        encodedPassword = m.captured(2);
        if (algo == "SHA" || algo == "SSHA")
          encoding = Sha1Base64;
        else if (algo == "MD5" || algo == "SMD5")
          encoding = Md5Base64;
        else if (algo == "CLEARTEXT")
          encoding = Plain;
        else // LATER RFC 2307 also define {CRYPT} algorithm
          encoding = Unknown;
        /*Log::fatal() << "login:" << userId << " password:" << encodedPassword
                     << " algo:" << algo << " encoding:" << encoding;*/
      } else {
        encoding = Unknown;
      }
    }
    d = new Data(userId, encodedPassword, encoding);
  }
  User(const User &other) : d(other.d) { }
  User &operator=(const User &other) {
    if (this != &other)
      d = other.d;
    return *this;
  }
  bool authenticate(QString password) const {
    if (d) {
      QByteArray hash, salt;
      int hashSize = 0;
      switch(d->_encoding) {
      case Plain:
      case OpenLdapStyle:
      case Unknown:
        break;
      case Md4Hex:
      case Md5Hex:
      case Sha1Hex:
        hash = QByteArray::fromHex(d->_encodedPassword.toUtf8());
        break;
      case Md4Base64:
      case Md5Base64:
      case Sha1Base64:
        hash = QByteArray::fromBase64(d->_encodedPassword.toUtf8());
        break;
      }
      switch(d->_encoding) {
      case Plain:
      case OpenLdapStyle:
      case Unknown:
        break;
      case Md4Hex:
      case Md4Base64:
      case Md5Hex:
      case Md5Base64:
        hashSize = 128/8;
        break;
      case Sha1Hex:
      case Sha1Base64:
        hashSize = 160/8;
        break;
      }
      if (hash.size() > hashSize) {
        salt = hash.right(hash.size() - hashSize);
        hash = hash.left(hashSize);
      }
      bool granted = false;
      switch(d->_encoding) {
      case Plain:
        granted = password.toUtf8() == d->_encodedPassword.toUtf8();
        break;
      case Md4Hex:
      case Md4Base64:
        granted = hash == QCryptographicHash::hash(password.toUtf8()+salt,
                                                   QCryptographicHash::Md4);
        break;
      case Md5Hex:
      case Md5Base64:
        granted = hash == QCryptographicHash::hash(password.toUtf8()+salt,
                                                   QCryptographicHash::Md5);
        break;
      case Sha1Hex:
      case Sha1Base64:
        granted = hash == QCryptographicHash::hash(password.toUtf8()+salt,
                                                   QCryptographicHash::Sha1);
        break;
      case OpenLdapStyle:
      case Unknown:
        break;
      }
      /*Log::fatal() << "authenticate: " << d->_userId << "/" << password
                   << "/" << d->_encodedPassword << "/" << d->_encoding
                   << " salt:0x" << salt.toHex()
                   << " granted:" << granted;*/
      return granted;
    }
    return false;
  }
};

InMemoryAuthenticator::InMemoryAuthenticator(QObject *parent)
  : Authenticator(parent) {
}

InMemoryAuthenticator::~InMemoryAuthenticator() {
}

QString InMemoryAuthenticator::authenticate(QString login, QString password,
                                            ParamSet ctxt) const {
  Q_UNUSED(ctxt)
  QMutexLocker locker(&_mutex);
  return _users.contains(login) && _users[login].authenticate(password)
      ? login : QString();
}

InMemoryAuthenticator &InMemoryAuthenticator::insertUser(
    QString userId, QString encodedPassword, Encoding encoding) {
  if (!userId.isEmpty()) {
    QMutexLocker locker(&_mutex);
    _users.insert(userId, User(userId, encodedPassword, encoding));
  }
  return *this;
}

InMemoryAuthenticator &InMemoryAuthenticator::clearUsers() {
  QMutexLocker locker(&_mutex);
  _users.clear();
  return *this;
}

bool InMemoryAuthenticator::containsUser(QString login) const {
  QMutexLocker locker(&_mutex);
  return _users.contains(login);
}

InMemoryAuthenticator::Encoding InMemoryAuthenticator::encodingFromString(
    QString text) {
  text = text.trimmed().toLower();
  if (text == "password" || text == "plain") {
    return Plain;
  } else if (text == "md5hex") {
    return Md5Hex;
  } else if (text == "md5" || text == "md5b64") {
    return Md5Base64;
  } else if (text == "md4hex") {
    return Md5Hex;
  } else if (text == "md4b64") {
    return Md5Base64;
  } else if (text == "sha1" || text == "sha1hex") {
    return Sha1Hex;
  } else if (text == "sha1b64") {
    return Sha1Base64;
  } else if (text == "ldap") {
    return OpenLdapStyle;
  } else {
    return Unknown;
  }
}

QString InMemoryAuthenticator::encodingToString(
    InMemoryAuthenticator::Encoding encoding) {
  switch(encoding) {
  case Plain:
    return "plain";
  case Md5Hex:
    return "md5hex";
  case Md5Base64:
    return "md5b64";
  case Md4Hex:
    return "md4hex";
  case Md4Base64:
    return "md4b64";
  case Sha1Hex:
    return "sha1hex";
  case Sha1Base64:
    return "sha1b64";
  case OpenLdapStyle:
    return "ldap";
  case Unknown:
    ;
  }
  return QString();
}
