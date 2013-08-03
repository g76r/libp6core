/* Copyright 2013 Hallowyn and others.
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
#include "inmemoryauthenticator.h"
#include <QCryptographicHash>
#include <QRegExp>
#include <QString>
//#include "log/log.h"

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
      static QRegExp openLdapHashFormat("\\s*\\{([^\\}]+)\\}(\\S*)\\s*");
      QRegExp re = openLdapHashFormat;
      if (re.exactMatch(encodedPassword) != -1) {
        QString algo = re.cap(1).trimmed().toUpper();
        encodedPassword = re.cap(2);
        if (algo == "SHA" || algo == "SSHA")
          encoding = InMemoryAuthenticator::Sha1Base64;
        if (algo == "MD5" || algo == "SMD5")
          encoding = InMemoryAuthenticator::Md5Base64;
        if (algo == "CLEARTEXT")
          encoding = InMemoryAuthenticator::Plain;
        else // LATER RFC 2307 also define {CRYPT} algorithm
          encoding = InMemoryAuthenticator::Unknown;
      } else {
        encoding = InMemoryAuthenticator::Unknown;
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
      case InMemoryAuthenticator::Plain:
      case InMemoryAuthenticator::OpenLdapStyle:
      case InMemoryAuthenticator::Unknown:
        break;
      case InMemoryAuthenticator::Md4Hex:
      case InMemoryAuthenticator::Md5Hex:
      case InMemoryAuthenticator::Sha1Hex:
        hash = QByteArray::fromHex(d->_encodedPassword.toUtf8());
        break;
      case InMemoryAuthenticator::Md4Base64:
      case InMemoryAuthenticator::Md5Base64:
      case InMemoryAuthenticator::Sha1Base64:
        hash = QByteArray::fromBase64(d->_encodedPassword.toUtf8());
        break;
      }
      switch(d->_encoding) {
      case InMemoryAuthenticator::Plain:
      case InMemoryAuthenticator::OpenLdapStyle:
      case InMemoryAuthenticator::Unknown:
        break;
      case InMemoryAuthenticator::Md4Hex:
      case InMemoryAuthenticator::Md4Base64:
      case InMemoryAuthenticator::Md5Hex:
      case InMemoryAuthenticator::Md5Base64:
        hashSize = 128/8;
        break;
      case InMemoryAuthenticator::Sha1Hex:
      case InMemoryAuthenticator::Sha1Base64:
        hashSize = 160/8;
        break;
      }
      if (hash.size() > hashSize) {
        salt = hash.right(hash.size() - hashSize);
        hash = hash.left(hashSize);
      }
      bool granted = false;
      switch(d->_encoding) {
      case InMemoryAuthenticator::Plain:
        granted = password.toUtf8() == d->_encodedPassword.toUtf8();
        break;
      case InMemoryAuthenticator::Md4Hex:
      case InMemoryAuthenticator::Md4Base64:
        granted = hash == QCryptographicHash::hash(password.toUtf8()+salt,
                                                   QCryptographicHash::Md4);
        break;
      case InMemoryAuthenticator::Md5Hex:
      case InMemoryAuthenticator::Md5Base64:
        granted = hash == QCryptographicHash::hash(password.toUtf8()+salt,
                                                   QCryptographicHash::Md5);
        break;
      case InMemoryAuthenticator::Sha1Hex:
      case InMemoryAuthenticator::Sha1Base64:
        granted = hash == QCryptographicHash::hash(password.toUtf8()+salt,
                                                   QCryptographicHash::Sha1);
        break;
      case InMemoryAuthenticator::OpenLdapStyle:
      case InMemoryAuthenticator::Unknown:
        break;
      }
      /*Log::fatal() << "authenticate: " << d->_userId << "/" << password
                   << "/" << d->_encodedPassword << "/" << d->_encoding
                   << " salt:" << salt.toHex()
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

