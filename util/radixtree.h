/* Copyright 2016-2025 Hallowyn, Gregoire Barbier and others.
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

#include "util/utf8stringset.h"

/** Helper class to make it possible to initialize a RadixTree with such syntax:
 * RadixTree<int> foo { {"abc", 42, true}, { "xyz", -1 } };
 * RadixTree<std::function<double(double)>> bar {
 *   { "square", [](double d) static { return d*d; } },
 *   { "round", ::round },
 *   { { "opposite", "-" }, [](double d) static { return -d; } }
 * };
 * keys are encoded in utf-8
 */
template<class T>
struct RadixTreeInitializerHelper {
  std::vector<const char *> _keys;
  T _value;
  bool _isPrefix;
  /** assumes that key is UTF-8 (or of course ASCII) */
  RadixTreeInitializerHelper(
    std::vector<const char *> key, T value, bool isPrefix = false)
    : _keys(key), _value(value), _isPrefix(isPrefix) { }
  /** assumes that key is UTF-8 (or of course ASCII) */
  RadixTreeInitializerHelper(
    const char *key, T value, bool isPrefix = false)
    : _value(value), _isPrefix(isPrefix) { _keys.push_back(key); }
  RadixTreeInitializerHelper(
    const QString &key, T value, bool isPrefix = false)
    : _value(value), _isPrefix(isPrefix) {
    _keys.push_back(key.toUtf8().constData());
  }
};

/** Lookup-optimized dictionary for a large number of strings or string prefixes
 * as keys, based on a radix tree (which is a size optimized trie a.k.a. prefix
 * tree).
 *
 * @see https://en.wikipedia.org/wiki/Radix_tree
 * @see https://en.wikipedia.org/wiki/Trie
 *
 * When isPrefix=false for every key, it behaves like a QMap<K,T> with K =
 * characters string.
 * If at less on key is set with isPrefix=true, it becomes more powerful than
 * a map and can e.g. match "/rest/customers/434909" with "/rest/customers/"
 * key.
 *
 * The class is optimized for handling const char *, any QString parameter is
 * first converted to const char * using QString::toUtf8().constData()
 * which is not costless.
 *
 * Keys are expected to be encoded in utf-8.
 *
 * This class implements Qt's implicit sharing pattern.
 */
template<class T>
class RadixTree {
  enum NodeType : signed char { Empty = 0, Exact, Prefix };
  using Visitor = std::function<void(const QByteArray *, NodeType, T)>;
  using AbortableVisitor = std::function<bool(const QByteArray *, NodeType, T)>;
  struct Node {
    char *_fragment;
    NodeType _nodetype;
    int _length;
    Node **_children;
    int _childrenCount;
    T _value;
    Node(const char *fragment, T value, NodeType nodetype, Node *parent)
      : _fragment(0), _nodetype(nodetype),
        _length((parent ? parent->_length : 0) + strlen(fragment)),
        _children(0), _childrenCount(0), _value(value) {
      Q_ASSERT(fragment);
      //qDebug() << "new Node" << fragment << parent << (parent ? parent->_fragment : "");
      _fragment = strdup(fragment);
      if (parent)
        parent->addChild(this);
    }
    Node(const Node &other)
      : _fragment(0), _nodetype(other._nodetype),
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
      if (!_fragment[i] && !key[i]) {
        // exact match -> override old value
        //qDebug() << "" << "exact match" << _fragment << key << this;
        _value = value;
        _nodetype = isPrefix ? Prefix : Exact;
      } else if (_fragment[i]) {
        // have to split the tree -> make current and new content two children
        //qDebug() << "" << "have to split" << _fragment+i << key+i << this
        //         << (key[i] ? "" : "without second child");
        auto oldChildren = _children;
        auto oldChildrenCount = _childrenCount;
        auto oldFragment = _fragment;
        _children = 0;
        _childrenCount = 0;
        //_fragment = strdup(_fragment);
        _length -= strlen(_fragment)-i; // must be done before new Node()
        Node *mainChild;
        if (/*oldFragment[i] && */!key[i]) {
          // inserted key shorter than this node key
          // override value and add a child for the remaining
          mainChild = new Node(oldFragment+i, _value, _nodetype, this);
          _value = value;
          _nodetype = isPrefix ? Prefix : Exact;
        } else {
          //qDebug() << "full fork" << oldFragment << oldFragment+i << key+i << isPrefix << nodetypeToString(_nodetype);
          // full fork
          // reset value and add two children for both branches
          mainChild = new Node(oldFragment+i, _value, _nodetype, this);
          new Node(key+i, value, isPrefix ? Prefix : Exact, this);
          _value = T();
          _nodetype = Empty;
        }
        mainChild->_children = oldChildren;
        mainChild->_childrenCount = oldChildrenCount;
        _fragment[i] = 0; // shorten _fragment without reallocating memory
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
        new Node(key+i, value, isPrefix ? Prefix : Exact, this);
      }
    }
    /** Recursively lookup the radix tree/branch
     * @return true if found
     * @param value receive a copy of the value if found and value!=0
     */
    bool inline lookup(
        const char *key, T *value, int *matchedLength) const {
      Q_ASSERT(key);
      Q_ASSERT(_fragment);
      int i = 0;
      for (; key[i] == _fragment[i] && key[i]; ++i)
        ;
      if (_fragment[i]) { // _fragment is longer -> key doesn't match
        //qDebug() << "Node::lookup false" << i << _fragment << key;
        return false;
      }
      if (_nodetype == Prefix) { // prefix match
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
        *value = _value;
        *matchedLength = _length;
        return true;
      }
      if (!key[i] && _nodetype == Exact) { // exact match -> select value
        //qDebug() << "Node::lookup exact match" << _fragment;
        *value = _value;
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
    /** Visit tree in depth-first order */
    void inline visit(Visitor visitor, QByteArray *key_prefix) const {
      key_prefix->append(_fragment);
      visitor(key_prefix->constData(), _nodetype, _value);
      for (int i = 0; i < _childrenCount; ++i)
        _children[i]->visit(visitor, key_prefix);
    }
    /** Visit tree in depth-first order, abort if visitor returns false */
    void inline visit(AbortableVisitor visitor, QByteArray *key_prefix) const {
      key_prefix->append(_fragment);
      if (!visitor(key_prefix->constData(), _nodetype, _value)) [[unlikely]]
        return;
      for (int i = 0; i < _childrenCount; ++i)
        if (!_children[i]->visit(visitor, key_prefix))
          return;
    }
    static QString nodetypeToString(NodeType nodetype) {
      switch (nodetype) {
      case Exact:
        return QStringLiteral("exact");
      case Prefix:
        return QStringLiteral("prefix");
      case Empty:
        return QStringLiteral("*EMPTY*");
      }
      return QStringLiteral("*UNKNOWN*");
    }
    QString valueToDebugString() const {
      // this method is overriden below, for known displayable types
      return QString();
    }
    QString toDebugString(QString indentation = QString()) const {
      QString s, v = valueToDebugString();
      s += indentation + '"'+ _fragment + "\" " + QString::number(_length)
          //+ " " + (_value ? "set" : "null")
          + " " + nodetypeToString(_nodetype)
          + (_nodetype == Empty || v.isNull() ? "" :  " -> " + v) + "\n";
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
    Utf8StringSet _keys;
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
  }
  explicit RadixTree(const QHash<QString,T> &hash) : RadixTree() {
    for (const auto &[k,v]: hash.asKeyValueRange())
      insert(k, v);
  }
  explicit RadixTree(const QHash<Utf8String,T> &hash) : RadixTree() {
    for (const auto &[k,v]: hash.asKeyValueRange())
      insert(k, v);
  }
  /** assumes that key is UTF-8 (or of course ASCII) */
  explicit RadixTree(const QHash<const char *,T> &hash) : RadixTree() {
    for (const auto &[k,v]: hash.asKeyValueRange())
      insert(k, v);
  }
  explicit RadixTree(const QMap<QString,T> &map) : RadixTree() {
    for (const auto &[k,v]: map.asKeyValueRange())
      insert(k, v);
  }
  explicit RadixTree(const QMap<Utf8String,T> &map) : RadixTree() {
    for (const auto &[k,v]: map.asKeyValueRange())
      insert(k, v);
  }
  /** assumes that key is UTF-8 (or of course ASCII) */
  explicit RadixTree(const QMap<const char *,T> &map) : RadixTree() {
    for (const auto &[k,v]: map.asKeyValueRange())
      insert(k, v);
  }
  RadixTree(const RadixTree &other) : d(other.d) { }
  RadixTree &operator=(const RadixTree &other) {
    if (&other != this)
      d = other.d;
    return *this;
  }
  /** assumes that key is UTF-8 (or of course ASCII) */
  void insert(const char *key, T value, bool isPrefix = false) {
    if (d->_root)
      d->_root->insert(key, value, isPrefix);
    else
      d->_root = new Node(key, value, isPrefix ? Prefix : Exact, 0);
    d->_keys.insert(key);
  }
  void insert(const QString &key, T value, bool isPrefix = false) {
    insert (key.toUtf8().constData(), value, isPrefix); }
  void insert(const QByteArray &key, T value, bool isPrefix = false) {
    insert (key.constData(), value, isPrefix); }
  void insert(const RadixTree<T> &other) {
      other.visit([this](const QByteArray *key, NodeType nodetype, T value) {
          insert(key, value, nodetype == Node::Prefix);
      });
  }
  void visit(Visitor visitor) const {
      QByteArray key_prefix;
      if (d->_root)
          d->_root->visit(visitor, &key_prefix);
  }
  void visit(AbortableVisitor visitor) const {
      QByteArray key_prefix;
      if (d->_root)
          d->_root->visit(visitor, &key_prefix);
  }
  /** assumes that key is UTF-8 (or of course ASCII) */
  const T value(const char *key, T defaultValue = T(),
                             int *matchedLength = 0) const {
    T value = defaultValue;
    int ignored_length;
    if (!matchedLength)
      matchedLength = &ignored_length;
    if (!d || !d->_root || !d->_root->lookup(key, &value, matchedLength))
      *matchedLength = 0;
    return value;
  }
  /** assumes that key is UTF-8 (or of course ASCII) */
  const T value(const char *key, int *matchedLength) const {
    return value(key, T(), matchedLength); }
  const T value(const QString &key, T defaultValue = T(),
                             int *matchedLength = 0) const {
    return value(key.toUtf8().constData(), defaultValue, matchedLength); }
  const T value(const QString &key, int *matchedLength) const {
    return value(key.toUtf8().constData(), T(), matchedLength); }
  /** assumes that key is UTF-8 (or of course ASCII) */
  const T value(const QByteArray &key, T defaultValue = T(),
                             int *matchedLength = 0) const {
    return value(key.constData(), defaultValue, matchedLength); }
  /** assumes that key is UTF-8 (or of course ASCII) */
  const T value(const QByteArray &key, int *matchedLength) const {
    return value(key.constData(), T(), matchedLength); }
  /** assumes that key is UTF-8 (or of course ASCII) */
  const T operator[](const char *key) const { return value(key); }
  const T operator[](const QString &key) const {
    return value(key.toUtf8().constData()); }
  /** assumes that key is UTF-8 (or of course ASCII) */
  const T operator[](const QByteArray &key) const {
    return value(key.constData()); }
  bool contains(const char *key) const {
    return (d && d->_root) ? d->_root->lookup(key) : false;
  }
  bool contains(const QString &key) const {
    return contains(key.toUtf8().constData()); }
  /** assumes that key is UTF-8 (or of course ASCII) */
  bool contains(const QByteArray &key) const {
    return contains(key.constData()); }
  Utf8StringSet keys() const {
    return d ? d->_keys : Utf8StringSet();
  }
  static RadixTree<T> reversed(const QHash<T,QString> &hash) {
    RadixTree<T> that;
    for (const auto &[k,v]: hash.asKeyValueRange())
      that.insert(v, k);
    return that;
  }
  static RadixTree<T> reversed(const QHash<T,Utf8String> &hash) {
    RadixTree<T> that;
    for (const auto &[k,v]: hash.asKeyValueRange())
      that.insert(v, k);
    return that;
  }
  /** assumes that key is UTF-8 (or of course ASCII) */
  static RadixTree<T> reversed(const QHash<T,const char *> &hash) {
    RadixTree<T> that;
    for (const auto &[k,v]: hash.asKeyValueRange())
      that.insert(v, k);
    return that;
  }
  /** assumes that key is UTF-8 (or of course ASCII) */
  static RadixTree<T> reversed(const QHash<T,QByteArray> &hash) {
    RadixTree<T> that;
    for (const auto &[k,v]: hash.asKeyValueRange())
      that.insert(v, k);
    return that;
  }
  static RadixTree<T> reversed(const QMap<T,QString> &map) {
    RadixTree<T> that;
    for (const auto &[k,v]: map.asKeyValueRange())
      that.insert(v, k);
    return that;
  }
  static RadixTree<T> reversed(const QMap<T,Utf8String> &map) {
    RadixTree<T> that;
    for (const auto &[k,v]: map.asKeyValueRange())
      that.insert(v, k);
    return that;
  }
  /** assumes that key is UTF-8 (or of course ASCII) */
  static RadixTree<T> reversed(const QMap<T,const char *> &map) {
    RadixTree<T> that;
    for (const auto &[k,v]: map.asKeyValueRange())
      that.insert(v, k);
    return that;
  }
  /** assumes that key is UTF-8 (or of course ASCII) */
  static RadixTree<T> reversed(const QMap<T,QByteArray> &map) {
    RadixTree<T> that;
    for (const auto &[k,v]: map.asKeyValueRange())
      that.insert(v, k);
    return that;
  }
  QMap<QString,T> toUtf16Map() const {
    QMap<QString,T> map;
    for (const auto &key : keys())
      map.insert(key, value(key));
    return map;
  }
  QMap<Utf8String,T> toUtf8Map() const {
    QMap<Utf8String,T> map;
    for (const auto &key : keys())
      map.insert(key, value(key));
    return map;
  }
  QMap<T,QString> toReversedUtf16Map() const {
    QMap<T,QString> map;
    for (const auto &key : keys())
      map.insert(value(key),key);
    return map;
  }
  QMap<T,Utf8String> toReversedUtf8Map() const {
    QMap<T,Utf8String> map;
    for (const auto &key : keys())
      map.insert(value(key),key);
    return map;
  }
  QString toDebugString() const {
    QString s = "RadixTree 0x" + QString::number((quint64)this, 16) + '\n';
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
  return QString::number((double)_value);
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
