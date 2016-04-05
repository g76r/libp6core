/* Copyright 2016 Hallowyn and others.
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
#ifndef STRINGMAP_H
#define STRINGMAP_H

#include "libqtssu_global.h"
#include <QSharedData>
#include <QString>
#include <QHash>
#include <QMap>
//#include <QtDebug>

// LATER T &operator[](QString key) { return operator[](key.toUtf8()); }
// LATER keys(), values(), operator==
// LATER methods w/ QStringRef ?
// LATER remove()
// LATER make it stdlib compliant

/** Helper class to make it possible to initialize a StringMap with such syntax:
 * StringMap<int> foo { {"abc", 42, true}, { "xyz", -1 } };
 * StringMap<std::function<double(double)>> bar {
 *   { "square", [](double d){ return d*d; } },
 *   { "round", ::round },
 *   { { "opposite", "-" }, [](double d){ return -d; } }
 * };
 */
template<class T>
struct StringMapInitializerHelper {
  std::vector<const char *> _keys;
  T _value;
  bool _isPrefix;
  StringMapInitializerHelper(std::vector<const char *>key, T value,
                             bool isPrefix = false)
    : _keys(key), _value(value), _isPrefix(isPrefix) { }
  StringMapInitializerHelper(const char *key, T value, bool isPrefix = false)
    : _value(value), _isPrefix(isPrefix) { _keys.push_back(key); }
};

/** Lookup-optimized dictionary for a large number of strings or string prefixes
 * as keys, based on a radix tree.
 * Kind of specialized QMap<K,T> for K = utf8 character string.
 * T must be castable to bool and this conversion must have "not null" semantics
 * such as the one provided by QPointer, std::function or even int.
 * The class is optimized for handling const char *, any QString parameter is
 * first converted to const char * using QString::utf8();
 */
template<class T>
class LIBQTSSUSHARED_EXPORT StringMap {
  struct Node {
    char *_fragment;
    T _value;
    bool _isPrefix;
    Node **_children;
    int _childrenCount;
    //Node() : _fragment(0), _isPrefix(false), _children(0), _childrenCount(0) { }
    Node(const char *fragment, T value, bool isPrefix, Node *parent)
      : _fragment(0), _value(value), _isPrefix(isPrefix), _children(0),
        _childrenCount(0) {
      Q_ASSERT(fragment);
      //qDebug() << "new Node" << fragment << parent;
      _fragment = strdup(fragment);
      if (parent)
        parent->addChild(this);
    }
    ~Node() {
      if (_fragment)
        free(_fragment);
      if (_children) {
        for (Node *child : _children)
          delete child;
        free(_children);
      }
    }
    /** Recursively insert into the radix tree/branch
     */
    void inline insert(const char *key, T value, bool isPrefix) {
      Q_ASSERT(key);
      //qDebug() << "Node::insert" << key;
      int i = 0;
      for (; key[i] == _fragment[i] && key[i]; ++i)
        ;
      //qDebug() << "" << i << key[i] << _fragment[i] << key << _fragment << !!_isPrefix << !!isPrefix << _childrenCount;
      if (!_fragment[i] && !key[i] && (!_isPrefix || isPrefix)) {
        // exact match -> override old value
        //qDebug() << "" << "exact match" << _fragment << key << this;
        _value = value;
        _isPrefix = isPrefix;
      } else if (!_childrenCount || _fragment[i]) {
        // have to split the tree -> make current and new content two children
        //qDebug() << "" << "have to split" << _fragment+i << key+i << this;
        auto oldChildren = _children;
        auto oldChildrenCount = _childrenCount;
        _children = 0;
        _childrenCount = 0;
        auto mainChild = new Node(_fragment+i, _value, _isPrefix, this);
        mainChild->_children = oldChildren;
        mainChild->_childrenCount = oldChildrenCount;
        _value = T();
        _isPrefix = false;
        _fragment[i] = 0; // shorten _fragment without reallocating memory
        new Node(key+i, value, isPrefix, this);
      } else {
        //qDebug() << "" << "among children" << i << _fragment << key << this;
        for (int j = 0; j < _childrenCount; ++j) {
          //qDebug() << "  " << i << _children[j]->_fragment[0] << key[i];
          // LATER binary search rather than full scan
          if (_children[j]->_fragment[0] == key[i]) {
            _children[j]->insert(key+i, value, isPrefix);
            return;
          }
        }
        new Node(key+i, value, isPrefix, this);
      }
    }
    /** Recursively lookup the radix tree/branch
     * @return true if found
     * @param value receive a copy of the value if found and value!=0
     */
    bool inline lookup(const char *key, T *value = 0) const {
      Q_ASSERT(key);
      Q_ASSERT(_fragment);
      int i = 0;
      for (; key[i] == _fragment[i] && key[i]; ++i)
        ;
      if (_fragment[i]) { // _fragment is longer -> key doesn't match
        //qDebug() << "Node::lookup false" << i << _fragment << key;
        return false;
      }
      if (_isPrefix) { // prefix match
        // -> check for a better (more precise) match among children
        bool amongChildren = lookupAmongChildren(key+i, value, _children,
                                                 _childrenCount);
        if (amongChildren) {
          //qDebug() << "Node::lookup prefix match deferred to children"
          //         << _fragment;
          return true;
        }
        // -> or select value if not null (i.e. if not in intermediary node)
        if (value) {
          //qDebug() << "Node::lookup prefix match not deferred to children"
          //         << _fragment;
          *value = _value;
          return true;
        }
      } else if (!key[i]) { // exact match -> select value if not null
        if (value) {
          //qDebug() << "Node::lookup exact match" << _fragment;
          *value = _value;
          return true;
        }
      }
      // _fragment is shorter and not _isPrefix, or value is null
      //     -> continue among children
      //qDebug() << "Node::lookup continue among children" << i << _fragment
      //         << key;
      return lookupAmongChildren(key+i, value, _children, _childrenCount);
    }
    QString toString(QString indentation = QString()) const {
      QString s;
      s += indentation + _fragment + " " + (_value ? "set" : "null") + " "
          + (_isPrefix ? "prefix" : "exact" ) + "\n";
      //int n = strlen(_fragment);
      //for (int i = 0; i < n; ++i)
      indentation += ' ';
      for (int i = 0; i < _childrenCount; ++i)
        s += _children[i]->toString(indentation);
      return s;
    }

  private:
    /// create a new sorted children list with one more child
    void addChild(Node *newChild) {
      //qDebug() << "addChild" << newChild << this << _children  << _childrenCount << sizeof(Node);
      Node **newChildren = new Node*[_childrenCount+1];
      int k = _childrenCount;
      for (int i = 0, j = 0; i < _childrenCount; ++i, ++j) {
        //qDebug() << "" << i << j;
        //qDebug() << "  " << _children[i]->_fragment << "/" << newChild->_fragment;
        //qDebug() << "  " << strcmp(_children[i]->_fragment, newChild->_fragment);
        if (strcmp(_children[i]->_fragment, newChild->_fragment) > 0 && i == j)
          k = j++;
        newChildren[j] = _children[i];
      }
      newChildren[k] = newChild;
      if (_children)
        delete[] _children;
      _children = newChildren;
      ++_childrenCount;
      //qDebug() << "/addChild" << this << _children << _childrenCount;
    }
    /// binary search among children and recurse lookup
    static inline bool lookupAmongChildren(
        const char *key, T *value, const Node * const *children,
        int childrenCount) {
      if (childrenCount <= 0)
        return false;
      int middle = childrenCount >> 1;
      int cmp = children[middle]->_fragment[0] - key[0];
      if (cmp > 0)
        return lookupAmongChildren(key, value, children, middle);
      if (cmp < 0)
        return lookupAmongChildren(key, value, children+middle+1,
                                   childrenCount-middle-1);
      return children[middle]->lookup(key, value);
    }
  };

  struct StringMapData : QSharedData {
    Node *_root;
    StringMapData() : _root(0) { }
  };

  QSharedDataPointer<StringMapData> d;

public:
  StringMap() : d(new StringMapData) { }
  StringMap(std::initializer_list<StringMapInitializerHelper<T>> list)
    : StringMap() {
    for (const StringMapInitializerHelper<T> &helper : list)
      for (const char *key: helper._keys)
        insert(key, helper._value, helper._isPrefix);
    //if (d->_root)
    //  qDebug() << "initialized:" << "\n"+d->_root->toString()+"\n";
  }
  StringMap(QHash<QString,T> hash) : StringMap() {
    foreach (const QString &key, hash.keys())
      insert(key.toUtf8().constData(), hash.value(key));
  }
  StringMap(QHash<const char *,T> hash) : StringMap() {
    foreach (const char *key, hash.keys())
      insert(key, hash.value(key));
  }
  StringMap(QMap<QString,T> map) : StringMap() {
    foreach (const QString &key, map.keys())
      insert(key.toUtf8().constData(), map.value(key));
  }
  StringMap(QMap<const char *,T> map) : StringMap() {
    foreach (const char *key, map.keys())
      insert(key, map.value(key));
  }
  StringMap(const StringMap &other) : d(other.d) { }
  StringMap &operator=(const StringMap &other) {
    if (&other != this) d = other.d; }
  void insert(const char *key, T value, bool isPrefix = false) {
    //qDebug() << "StringMap::insert" << !!d << key;
    if (!d)
      return;
    if (d->_root)
      d->_root->insert(key, value, isPrefix);
    else
      d->_root = new Node(key, value, isPrefix, 0);
  }
  const T value(const char *key, T defaultValue) const {
    T value = defaultValue;
    if (d && d->_root)
      d->_root->lookup(key, &value);
    return value;
  }
  bool contains(const char *key) const {
    return (d && d->_root) ? d->_root->lookup(key) : false;
  }
  // LATER T &operator[](const char *key);


  void insert(QString key, T value, bool isPrefix = false) {
    insert (key.toUtf8().constData(), value, isPrefix); }
  const T value(const char *key) const { return value(key, T()); }
  const T value(QString key, T defaultValue) const {
    return value(key.toUtf8().constData(), defaultValue); }
  const T value(QString key) const { return value(key.toUtf8().constData()); }
  bool contains(QString *key) const { return value(key->toUtf8().constData()); }
  const T operator[](QString key) const { return value(key); }
  const T operator[](const char *key) const { return value(key); }

  static StringMap<T> reversed(QHash<T,QString> hash) {
    StringMap<T> that;
    foreach (const T &key, hash.keys())
      that.insert(hash.value(key).toUtf8().constData(), key);
    return that;
  }
  static StringMap<T> reversed(QHash<T,const char *> hash) {
    StringMap<T> that;
    foreach (const T &key, hash.keys())
      that.insert(hash.value(key), key);
    return that;
  }
  static StringMap<T> reversed(QMap<T,QString> map) {
    StringMap<T> that;
    foreach (const T &key, map.keys())
      that.insert(map.value(key).toUtf8().constData(), key);
  }
  static StringMap<T> reversed(QMap<T,const char *> map) {
    StringMap<T> that;
    foreach (const T &key, map.keys())
      that.insert(map.value(key), key);
  }
};

#endif // STRINGMAP_H
