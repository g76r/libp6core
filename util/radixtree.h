/* Copyright 2016-2017 Hallowyn, Gregoire Barbier and others.
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
#ifndef RADIXTREE_H
#define RADIXTREE_H

#include "libp6core_global.h"
#include <QSharedData>
#include <QString>
#include <QHash>
#include <QMap>

/** Helper class to make it possible to initialize a RadixTree with such syntax:
 * RadixTree<int> foo { {"abc", 42, true}, { "xyz", -1 } };
 * RadixTree<std::function<double(double)>> bar {
 *   { "square", [](double d){ return d*d; } },
 *   { "round", ::round },
 *   { { "opposite", "-" }, [](double d){ return -d; } }
 * };
 */
template<class T>
struct RadixTreeInitializerHelper {
  std::vector<const char *> _keys;
  T _value;
  bool _isPrefix;
  RadixTreeInitializerHelper(std::vector<const char *>key, T value,
                             bool isPrefix = false)
    : _keys(key), _value(value), _isPrefix(isPrefix) { }
  RadixTreeInitializerHelper(const char *key, T value, bool isPrefix = false)
    : _value(value), _isPrefix(isPrefix) { _keys.push_back(key); }
};

/** Lookup-optimized dictionary for a large number of strings or string prefixes
 * as keys, based on a radix tree.
 *
 * When isPrefix=false for every key, it behaves like a QMap<K,T> with K = utf8
 * characters string.
 * If at less on key is set with isPrefix=true, it becomes more powerful than
 * a map and can e.g. match "/rest/customers/434909" with "/rest/customers/"
 * key.
 *
 * The class is optimized for handling const char *, any QString parameter is
 * first converted to const char * using QString::toUtf8().constData() which is
 * not costless.
 *
 * See e.g. https://en.wikipedia.org/wiki/Radix_tree for a more detailed
 * explanation of radix tree internals.
 *
 * This class implements Qt's implicit sharing pattern.
 */
template<class T>
class LIBPUMPKINSHARED_EXPORT RadixTree {
  struct Node {
    char *_fragment;
    bool _isPrefix;
    int _length;
    Node **_children;
    int _childrenCount;
    T _value;
    //Node() : _fragment(0), _isPrefix(false), _children(0), _childrenCount(0) { }
    Node(const char *fragment, T value, bool isPrefix, Node *parent)
      : _fragment(0), _isPrefix(isPrefix),
        _length((parent ? parent->_length : 0) + strlen(fragment)),
        _children(0), _childrenCount(0), _value(value) {
      Q_ASSERT(fragment);
      //qDebug() << "new Node" << fragment << parent << (parent ? parent->_fragment : "");
      _fragment = strdup(fragment);
      if (parent)
        parent->addChild(this);
    }
    Node(const Node &other)
      : _fragment(0), _isPrefix(other._isPrefix),
        _length(other._length), _children(0),
        _childrenCount(other._childrenCount), _value(other._value) {
      Q_ASSERT(other._fragment);
      _fragment = strdup(other._fragment);
      if (_childrenCount) {
        _children = new Node*[other._childrenCount];
        for (int i = 0; i < _childrenCount; ++i)
          _children[i] = new Node(*other._children[i]);
      }
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
      //qDebug() << "" << i << (int)key[i] << (int)_fragment[i] << key
      //         << _fragment << !!_isPrefix << !!isPrefix << _childrenCount;
      if (!_fragment[i] && !key[i] && (!_isPrefix || isPrefix)) {
        // exact match -> override old value
        //qDebug() << "" << "exact match" << _fragment << key << this;
        _value = value;
        _isPrefix = isPrefix;
      } else if (_fragment[i]) {
        // have to split the tree -> make current and new content two children
        //qDebug() << "" << "have to split" << _fragment+i << key+i << this
        //         << (i && key[i] ? "" : "without second child");
        auto oldChildren = _children;
        auto oldChildrenCount = _childrenCount;
        auto oldFragment = _fragment;
        _children = 0;
        _childrenCount = 0;
        _fragment = strdup(_fragment);
        _fragment[i] = 0; // must be done before new Node()
        _length = i;
        auto mainChild = new Node(oldFragment+i, _value, _isPrefix, this);
        mainChild->_children = oldChildren;
        mainChild->_childrenCount = oldChildrenCount;
        free(oldFragment);
        if (key[i]) {
          // inserted key > this node key or totally different (for root node),
          // need a second child
          _value = T();
          _isPrefix = false;
          new Node(key+i, value, isPrefix, this);
        } else { // inserted key is exactly this node key, override value
          _value = value;
          _isPrefix = isPrefix;
        }
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
    bool inline lookup(const char *key, T *value = 0,
                       int *matchedLength = 0) const {
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
                                                 _childrenCount, matchedLength);
        if (amongChildren) {
          //qDebug() << "Node::lookup prefix match deferred to children"
          //         << _fragment;
          return true;
        }
        // -> or select value
        //qDebug() << "Node::lookup prefix match not deferred to children"
        //         << _fragment;
        if (value)
          *value = _value;
        if (matchedLength)
          *matchedLength = _length;
        return true;
      }
      if (!key[i]) { // exact match -> select value if not null
        // FIXME the special meaning of null is a bad idea, e.g. for RadixTree<int>
        //qDebug() << "Node::lookup exact match" << _fragment;
        if (value)
          *value = _value;
        if (matchedLength)
          *matchedLength = _length;
        return true;
      }
      // _fragment is shorter and not _isPrefix, or value is null
      //     -> continue among children
      //qDebug() << "Node::lookup continue among children" << i << _fragment
      //         << key;
      return lookupAmongChildren(key+i, value, _children, _childrenCount,
                                 matchedLength);
    }
    QString valueToDebugString() const {
      // this method is overriden later, for known displayable types
      return QString();
    }
    QString toDebugString(QString indentation = QString()) const {
      QString s, v = valueToDebugString();
      s += indentation + _fragment + " " + QString::number(_length)
          //+ " " + (_value ? "set" : "null")
          + " " + (_isPrefix ? "prefix" : "exact" )
          + (v.isNull() ? "" :  " -> " + v) + "\n";
      indentation += ' ';
      for (int i = 0; i < _childrenCount; ++i)
        s += _children[i]->toDebugString(indentation);
      return s;
    }

  private:
    /// create a new sorted children list with one more child
    void addChild(Node *newChild) {
      //qDebug() << "addChild" << newChild << this << _children << _childrenCount << sizeof(Node);
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
        int childrenCount, int *matchedLength) {
      if (childrenCount <= 0)
        return false;
      int middle = childrenCount >> 1;
      int cmp = children[middle]->_fragment[0] - key[0];
      if (cmp > 0)
        return lookupAmongChildren(key, value, children, middle, matchedLength);
      if (cmp < 0)
        return lookupAmongChildren(key, value, children+middle+1,
                                   childrenCount-middle-1, matchedLength);
      return children[middle]->lookup(key, value, matchedLength);
    }
  };

  struct RadixTreeData : QSharedData {
    Node *_root;
    RadixTreeData() : _root(0) { }
  };

  QSharedDataPointer<RadixTreeData> d;

public:
  RadixTree() : d(new RadixTreeData) { }
  RadixTree(std::initializer_list<RadixTreeInitializerHelper<T>> list)
    : RadixTree() {
    for (const RadixTreeInitializerHelper<T> &helper : list)
      for (const char *key: helper._keys)
        insert(key, helper._value, helper._isPrefix);
    //dumpContent();
  }
  RadixTree(QHash<QString,T> hash) : RadixTree() {
    foreach (const QString &key, hash.keys())
      insert(key.toUtf8().constData(), hash.value(key));
  }
  RadixTree(QHash<const char *,T> hash) : RadixTree() {
    foreach (const char *key, hash.keys())
      insert(key, hash.value(key));
  }
  RadixTree(QMap<QString,T> map) : RadixTree() {
    foreach (const QString &key, map.keys())
      insert(key.toUtf8().constData(), map.value(key));
  }
  RadixTree(QMap<const char *,T> map) : RadixTree() {
    foreach (const char *key, map.keys())
      insert(key, map.value(key));
  }
  RadixTree(const RadixTree &other) : d(other.d) { }
  RadixTree &operator=(const RadixTree &other) {
    if (&other != this) d = other.d; return *this; }
  void insert(const char *key, T value, bool isPrefix = false) {
    //qDebug() << "RadixTree::insert" << !!d << key;
    if (!d)
      return;
    if (d->_root)
      d->_root->insert(key, value, isPrefix);
    else
      d->_root = new Node(key, value, isPrefix, 0);
  }
  void insert(const QString &key, T value, bool isPrefix = false) {
    insert (key.toUtf8().constData(), value, isPrefix); }
  const T value(const char *key, T defaultValue = T(),
                int *matchedLength = 0) const {
    T value = defaultValue;
    if (matchedLength)
      *matchedLength = 0;
    if (d && d->_root)
      d->_root->lookup(key, &value, matchedLength);
    return value;
  }
  const T value(const char *key, int *matchedLength) const {
    return value(key, T(), matchedLength); }
  const T value(const QString &key, T defaultValue = T(),
                int *matchedLength = 0) const {
    return value(key.toUtf8().constData(), defaultValue, matchedLength); }
  const T value(const QString &key, int *matchedLength) const {
    return value(key.toUtf8().constData(), T(), matchedLength); }
  const T operator[](const QString &key) const { return value(key); }
  const T operator[](const char *key) const { return value(key); }
  bool contains(const char *key) const {
    return (d && d->_root) ? d->_root->lookup(key) : false;
  }
  bool contains(const QString &key) const {
    return value(key.toUtf8().constData()); }
  static RadixTree<T> reversed(QHash<T,QString> hash) {
    RadixTree<T> that;
    foreach (const T &key, hash.keys())
      that.insert(hash.value(key).toUtf8().constData(), key);
    //that.dumpContent();
    return that;
  }
  static RadixTree<T> reversed(QHash<T,const char *> hash) {
    RadixTree<T> that;
    foreach (const T &key, hash.keys())
      that.insert(hash.value(key), key);
    return that;
  }
  static RadixTree<T> reversed(QMap<T,QString> map) {
    RadixTree<T> that;
    foreach (const T &key, map.keys())
      that.insert(map.value(key).toUtf8().constData(), key);
  }
  static RadixTree<T> reversed(QMap<T,const char *> map) {
    RadixTree<T> that;
    foreach (const T &key, map.keys())
      that.insert(map.value(key), key);
  }
  QString toDebugString() {
    QString s = "RadixTree" + QString::number((quint64)this, 16) + '\n';
    if (d->_root)
      s += d->_root->toDebugString();
    return s;
  }
};

template <>
inline QString RadixTree<signed char>::Node::valueToDebugString() const {
  return QString::number(_value);
}

template <>
inline QString RadixTree<unsigned char>::Node::valueToDebugString() const {
  return QString::number(_value);
}

template <>
inline QString RadixTree<signed short>::Node::valueToDebugString() const {
  return QString::number(_value);
}

template <>
inline QString RadixTree<unsigned short>::Node::valueToDebugString() const {
  return QString::number(_value);
}

template <>
inline QString RadixTree<signed int>::Node::valueToDebugString() const {
  return QString::number(_value);
}

template <>
inline QString RadixTree<unsigned int>::Node::valueToDebugString() const {
  return QString::number(_value);
}

template <>
inline QString RadixTree<signed long>::Node::valueToDebugString() const {
  return QString::number(_value);
}

template <>
inline QString RadixTree<unsigned long>::Node::valueToDebugString() const {
  return QString::number(_value);
}

template <>
inline QString RadixTree<signed long long>::Node::valueToDebugString() const {
  return QString::number(_value);
}

template <>
inline QString RadixTree<unsigned long long>::Node::valueToDebugString() const {
  return QString::number(_value);
}

template <>
inline QString RadixTree<float>::Node::valueToDebugString() const {
  return QString::number(_value);
}

template <>
inline QString RadixTree<double>::Node::valueToDebugString() const {
  return QString::number(_value);
}

template <>
inline QString RadixTree<const void*>::Node::valueToDebugString() const {
  return QString::number((qulonglong)_value, 16);
}

template <>
inline QString RadixTree<QString>::Node::valueToDebugString() const {
  return _value;
}

template <>
inline QString RadixTree<const char *>::Node::valueToDebugString() const {
  return QString::fromUtf8(_value);
}

#endif // RADIXTREE_H
