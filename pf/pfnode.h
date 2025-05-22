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
#ifndef PFNODE_H
#define PFNODE_H

#include "util/utf8stringlist.h"
#include "util/containerutils.h"
#include "pf/pfoptions.h"
#include <ranges>
#include <QPointer>
#include <QIODevice>

class QIODevice;

/** Class representing any level of Parenthesis Format (PF) tree node.
 *  Holds methods reading and modifying data in the node and its children,
 *  and for writing to PF external format.
 *  For reading one should have a look to PfParser. */
struct LIBP6CORESHARED_EXPORT PfNode {
private:
  struct LIBP6CORESHARED_EXPORT Fragment {
    enum FragmentType : quint8 {
      Text = 0, Comment, LoadedBinary, DeferredBinary, Child,
    };
    template <typename T>
    struct FragmentsForwardIterator {
      using difference_type = std::ptrdiff_t;
      using value_type = T *;
      value_type f;
      value_type operator*() const { return f; }
      FragmentsForwardIterator &operator++() {
        f = f ? f->_next : 0;
        return *this;
      };
      void operator++(int) { ++*this; }
      bool operator==(const FragmentsForwardIterator &that) const {
        return f == that.f;
      }
    };
    template <typename T>
    struct FragmentForwardRange {
      T *f;
      FragmentsForwardIterator<T> begin() const { return {f}; }
      FragmentsForwardIterator<T> end() const { return {0}; }
    };

    Fragment *_next = 0;

    inline Fragment() { }
    Fragment(const Fragment&) = delete;
    virtual ~Fragment();
    virtual FragmentType type() const = 0;
    /** method is responsible for performing deep copy of fragment impl.
     *  it's not responsible for copying/recursively cloning _next fragment */
    virtual Fragment *deep_copy() const = 0;
    /** @return true if type is a payload type (text or binary) */
    virtual bool has_payload() const;
    virtual Utf8String wrappings() const;
    /** change wrappings of LoadedBinary fragment.
     *  do nothing to other fragment types, including DeferredBinary. */
    virtual void set_wrappings(const Utf8String &wrappings);
    virtual Utf8String text() const;
    virtual Utf8String escaped_text() const;
    virtual Utf8String comment() const;
    virtual QByteArray unwrapped_data() const;
    virtual qsizetype size() const;
    virtual PfNode *child();
    const PfNode *child() const { return const_cast<Fragment*>(this)->child(); }
    /** append text to fragments list: merge on last text fragment if any or
     *  append a new text fragment at end if no text fragment was found or if
     *  last fragment wasn't a text fragment.
     *  @return true if merged, false if appended a new text fragment */
    virtual bool merge_text_on_last_fragment(const Utf8String &text);
    static void push_back(Fragment **pthis, Fragment *f) {
      Q_ASSERT(pthis != 0);
      while (*pthis)
        pthis = &((*pthis)->_next);
      *pthis = f;
    }
    static void push_front(Fragment **pthis, Fragment *f) {
      Q_ASSERT(pthis != 0);
      f->_next = *pthis;
      *pthis = f;
    }
    template <std::predicate<Fragment*> Pred>
    static inline size_t delete_if(Fragment **pthis, Pred pred) {
      Q_ASSERT(pthis != 0);
      size_t count = 0;
      while (*pthis) {
        // qDebug() << "deleting?" << Utf8String::number(*pthis)
        //          << (*pthis)->type() << Utf8String::number((*pthis)->_next)
        //          << pred(*pthis) << count;
        if (pred(*pthis)) {
          auto doomed = *pthis;
          *pthis = (*pthis)->_next;
          doomed->_next = 0; // disable delete of new *pthis
          delete doomed;
          ++count;
        } else {
          pthis = &((*pthis)->_next);
        }
      }
      return count;
    }
    static void set_attribute(
        Fragment **pthis, const Utf8String &name, const Utf8String &value);
    /** take first fragment if any, removing it from the list and
     *  giving ownership */
    static inline Fragment *take_first(Fragment **pthis) {
      Q_ASSERT(pthis != 0);
      auto taken = *pthis;
      if (taken) {
        *pthis = (*pthis)->_next;
        taken->_next = 0; // disable delete of new *pthis
      }
      return taken;
    }
    inline qsizetype count() const {
      qsizetype count = 0;
      for (auto f = this; f; f = f->_next)
        ++count;
      return count;
    }
    inline qsizetype count_of_type(FragmentType type) const {
      qsizetype count = 0;
      for (auto f = this; f; f = f->_next)
        if (f->type() == type)
          ++count;
      return count;
    }
    static inline void deep_copy(Fragment **pthis, const Fragment *original) {
      // qDebug() << "*** deep_copy" << *pthis << original;
      for (; original; original = original->_next) {
        *pthis = original->deep_copy();
        // qDebug() << "   copied" << original << "as" << *pthis
        //          << (*pthis)->_next << original->text() << original->type()
        //          << original->child();
        pthis = &((*pthis)->_next);
      }
      *pthis = 0;
    }
  };
  struct LIBP6CORESHARED_EXPORT TextFragment : Fragment {
    Utf8String _text;
    inline TextFragment(const Utf8String &utf8) : _text(utf8) {}
    FragmentType type() const override;
    Fragment *deep_copy() const override;
    bool has_payload() const override;
    Utf8String text() const override;
    Utf8String escaped_text() const override;
    QByteArray unwrapped_data() const override;
    qsizetype size() const override;
    bool merge_text_on_last_fragment(const Utf8String &text) override;
  };
  struct LIBP6CORESHARED_EXPORT CommentFragment : Fragment {
    Utf8String _comment;
    inline CommentFragment(const Utf8String &comment) : _comment(comment) {}
    FragmentType type() const override;
    Fragment *deep_copy() const override;
    Utf8String comment() const override;
  };
  struct LIBP6CORESHARED_EXPORT LoadedBinaryFragment : Fragment {
    QByteArray _data;
    Utf8String _wrappings;
    inline LoadedBinaryFragment(const QByteArray &unwrapped_data,
                                const Utf8String &wrappings)
      : _data(unwrapped_data), _wrappings(wrappings) {}
    FragmentType type() const override;
    Fragment *deep_copy() const override;
    bool has_payload() const override;
    QByteArray unwrapped_data() const override;
    qsizetype size() const override;
    Utf8String wrappings() const override;
    void set_wrappings(const Utf8String &wrappings) override;
  };
  struct LIBP6CORESHARED_EXPORT DeferredBinaryFragment : Fragment {
    QPointer<QIODevice> _file;
    qsizetype _pos = 0, _len = 0;
    quint8 _should_cache:1 = 0;
    mutable QByteArray _cache = {};
    inline DeferredBinaryFragment(QIODevice *file, qsizetype pos,
                                  qsizetype len, bool should_cache)
      : _file(file), _pos(pos), _len(len), _should_cache(should_cache) { }
    FragmentType type() const override;
    Fragment *deep_copy() const override;
    bool has_payload() const override;
    QByteArray unwrapped_data() const override;
    qsizetype size() const override;
  };
  struct ChildFragment;

  using enum Fragment::FragmentType;

  Utf8String _name;
  Fragment *_fragments = 0;
  quint64 _line = 0, _column = 0;
  static PfNode _empty;

public:
  inline PfNode() {}
  /** If name is empty the node will be null. */
  inline PfNode(const Utf8String &name, const Utf8String &text = {})
    : _name(name) {
    if (!text.isEmpty())
      _fragments = new TextFragment(text);
  }
  inline PfNode(const PfNode &other)
    : _name(other._name), _line(other._line), _column(other._column) {
    Fragment::deep_copy(&_fragments, other._fragments);
    // qDebug() << "&PfNode" << Utf8String::number(this)
    //          << Utf8String::number(&other);
  }
  inline PfNode(PfNode &&other) // takes ownership of fragments
    : _name(std::move(other._name)),
      _fragments(std::exchange(other._fragments, nullptr)), // prevents double delete
      _line(other._line), _column(other._column) {
    // qDebug() << "&&PfNode" << Utf8String::number(this)
    //          << Utf8String::number(&other);
  }
  inline ~PfNode() {
    // qDebug() << "~PfNode" << Utf8String::number(this)
    //          << Utf8String::number(_fragments) << _name;
    if (_fragments)
      delete _fragments;
  }
  /** Convert number into utf8 text.
   *  Includes bool, which will be converted to "true" or "false" */
  inline PfNode(const Utf8String &name, p6::arithmetic auto number)
    : PfNode(name, Utf8String::number(number)) { }
  /** Create node with children (take their content with std::move).
   *
   *  Usefull for instance to write this without deep copy:
   *  PfNode n;
   *  n = { "name", "content", PfNode{"child1", "content1"} };
   *
   *  If name is empty, the node will be null (and children ignored). */
  template <typename... Ts>
  requires (std::same_as<Ts, PfNode> && ...)
  inline PfNode(const Utf8String &name, const Utf8String &content,
                Ts&&... children) : PfNode(name, content) {
    if (!name.isEmpty())
      (append_child(std::move(children)), ...);
  }
  /** Create node with children (deep copy).
   *  If name is empty, the node will be null (and children ignored). */
  template <typename... Ts>
  requires (std::same_as<Ts, PfNode> && ...)
  inline PfNode(const Utf8String &name, const Utf8String &content,
                const Ts&... children) : PfNode(name, content) {
    if (!name.isEmpty())
      (append_child(children), ...);
  }
  /** Create node with children (take their content with std::move).
   *
   *  Usefull for instance to write this without deep copy:
   *  PfNode n;
   *  n = { "name", PfNode{"child1", "content1"} };
   *
   *  If name is empty, the node will be null (and children ignored). */
  template <typename... Ts>
  requires (std::same_as<Ts, PfNode> && ...)
  inline PfNode(const Utf8String &name, Ts&&... children) : PfNode(name) {
    if (!name.isEmpty())
      (append_child(std::move(children)), ...);
  }
  /** Create node with children (deep copy).
   *  If name is empty, the node will be null (and children ignored). */
  template <typename... Ts>
  requires (std::same_as<Ts, PfNode> && ...)
  inline PfNode(const Utf8String &name, const Ts&... children) : PfNode(name) {
    if (!name.isEmpty())
      (append_child(children), ...);
  }
  inline PfNode &operator=(const PfNode &other) {
    if (&other != this) {
      _name = other._name;
      _line = other._line;
      _column = other._column;
      if (_fragments)
        delete _fragments;
      Fragment::deep_copy(&_fragments, other._fragments);
      // qDebug() << "=&Node" << Utf8String::number(this)
      //          << Utf8String::number(&other);
    }
    return *this;
  }
  inline PfNode &operator=(PfNode &&other) {
    if (&other != this) {
      _name = std::move(other._name);
      _line = other._line;
      _column = other._column;
      if (_fragments)
        delete _fragments;
      _fragments = std::exchange(other._fragments, nullptr); // prevents double delete
      // qDebug() << "=&&Node" << Utf8String::number(this)
      //          << Utf8String::number(&other);
    }
    return *this;
  }
  inline PfNode &clear() {
    _name.clear();
    _line = 0;
    _column = 0;
    if (_fragments) {
      delete _fragments;
      _fragments = 0;
    }
    return *this;
  }
  inline bool operator!() const { return _name.isEmpty(); }
  inline bool is_null() const { return !*this; }
  inline std::weak_ordering operator<=>(const PfNode &other) const {
    return _name <=> other._name; }
  inline qsizetype fragments_count() const {
    return _fragments ? _fragments->count() : 0;
  }
  inline qsizetype children_count() const {
    return _fragments ? _fragments->count_of_type(Child) : 0;
  }

  ////// name /////////////////////////////////////////////////////////////////

  /** A node has an empty string name if and only if the node is null. */
  [[nodiscard]] inline Utf8String name() const { return _name; }
  inline PfNode &set_name(const Utf8String &name) {
    _name = name; return *this; } // LATER maybe remove content if name empty
  /** Syntaxic sugar: node ^ "foo" === !!node && node.name() == "foo" */
  inline bool operator^(const Utf8String &name) const {
    return _name == name; }
  /** Syntaxic sugar:
   *  node ^ node2 === !!node && !!node2 && node.name() == node2.name() */
  inline bool operator^(const PfNode &that) const {
    return _name == that._name; }
  /** Syntaxic sugar: node ^ list === !!node && names.contains(node.name()) */
  template <class S>
  requires (p6::readable_set<S,Utf8String> && !std::same_as<S,QString>
            && !std::same_as<S,QByteArray>)
  [[nodiscard]] inline bool operator^(const S &names) const {
    return names.contains(_name); }
  /** Children as range loop expression (currently: as copy). */

  ///// children //////////////////////////////////////////////////////////////

  /** Children as range loop expression. */
  [[nodiscard]] inline auto children() const {
    return std::views::all(Fragment::FragmentForwardRange(_fragments))
        | std::views::filter([](const Fragment *f) STATIC_LAMBDA { return f->type() == Child; })
        | std::views::transform([](const Fragment *f) STATIC_LAMBDA -> const PfNode & { return *f->child(); });
  }
  /** Children as range loop expression. */
  [[nodiscard]] inline auto children() {
    return std::views::all(Fragment::FragmentForwardRange(_fragments))
        | std::views::filter([](struct Fragment *f) STATIC_LAMBDA { return f->type() == Child; })
        | std::views::transform([](struct Fragment *f) STATIC_LAMBDA -> PfNode & { return *f->child(); });
  }
  /** Children filtered by their name, as range loop expression. */
  [[nodiscard]] inline auto children(const Utf8String &name) const {
    return std::views::all(Fragment::FragmentForwardRange(_fragments))
        | std::views::filter([name](const Fragment *f) { auto child = f->child(); return child && *child^name; })
        | std::views::transform([](const Fragment *f) STATIC_LAMBDA -> const PfNode & { return *f->child(); });
  }
  /** Children filtered by their name, as range loop expression. */
  [[nodiscard]] inline auto children(const Utf8String &name) {
    return std::views::all(Fragment::FragmentForwardRange(_fragments))
        | std::views::filter([name](struct Fragment *f) { auto child = f->child(); return child && *child^name; })
        | std::views::transform([](struct Fragment *f) STATIC_LAMBDA -> PfNode & { return *f->child(); });
  }
  /** Children filtered by their name, as range loop expression. */
  template <class S>
  requires (p6::readable_set<S,Utf8String> && !std::same_as<S,QString>
            && !std::same_as<S,QByteArray>)
  [[nodiscard]] inline auto children(const S &names) const {
    return std::views::all(Fragment::FragmentForwardRange(_fragments))
        | std::views::filter([names](const Fragment *f) { auto child = f->child(); return child && *child^names; })
        | std::views::transform([](const Fragment *f) STATIC_LAMBDA -> const PfNode & { return *f->child(); });
  }
  /** Children filtered by their name, as range loop expression. */
  template <class S>
  requires (p6::readable_set<S,Utf8String> && !std::same_as<S,QString>
            && !std::same_as<S,QByteArray>)
  [[nodiscard]] inline auto children(const S &names) {
    return std::views::all(Fragment::FragmentForwardRange(_fragments))
        | std::views::filter([names](struct Fragment *f) { auto child = f->child(); return child && *child^names; })
        | std::views::transform([](struct Fragment *f) STATIC_LAMBDA -> PfNode & { return *f->child(); });
  }
  /** Syntaxic sugar: node/"foo" === node.children("foo") */
  [[nodiscard]] inline auto operator/(const Utf8String &name) const {
    return children(name); }
  /** Syntaxic sugar: node/{"foo","bar"} === node.children({"foo","bar"}) */
  template <class S>
  requires (p6::readable_set<S,Utf8String> && !std::same_as<S,QString>
            && !std::same_as<S,QByteArray>)
  [[nodiscard]] inline auto operator/(const S &names) const {
    return children(names); }
  /** Children as QList copy */
  [[nodiscard]] inline QList<PfNode> children_copy() const {
    // C++23: return children() | std::ranges::to<QList<PfNode>>();
    // maybe rather let the caller do it (to whatever container he wants)
    QList<PfNode> list;
    for (const auto &child: children())
      list += child;
    return list;
  }
  /** Children as QList copy */
  template <typename T>
  [[nodiscard]] inline QList<PfNode> children_copy(const T &name) const {
    // C++23: return children() | std::ranges::to<QList<PfNode>>();
    // maybe rather let the caller do it (to whatever container he wants)
    QList<PfNode> list;
    for (const auto &child: children(name))
      list += child;
    return list;
  }
  inline PfNode &append_child(const PfNode &child);
  inline PfNode &append_child(PfNode &&child);
  [[deprecated("use append_child() instead")]]
  inline PfNode &appendChild(const PfNode &child) {
    return append_child(child);
  }
  /** copy every children (deep copy) */
  template <typename T>
  requires std::ranges::input_range<T>
  && (std::same_as<std::ranges::range_reference_t<T>,const PfNode&>
      || std::same_as<std::ranges::range_reference_t<T>,PfNode&>
      || std::same_as<std::ranges::range_reference_t<T>,PfNode&&>)
  inline PfNode &append_children(T children) {
    //qDebug() << "*** append_children range";
     // LATER optimize against following list n times (O(n) rather than O(n²))
    for (auto &&child: children)
      append_child(child);
    return *this;
  }
  /** take every children (std::move) */
  template <typename... Ts>
  requires (std::same_as<Ts, PfNode> && ...)
  inline PfNode &append_children(Ts&&... children) {
    // LATER optimize against following list n times (O(n) rather than O(n²))
    //qDebug() << "*** append_children&&";
    (append_child(std::move(children)), ...);
    return *this;
  }
  /** copy every children (deep copy) */
  template <typename... Ts>
  requires (std::same_as<Ts, PfNode> && ...)
  inline PfNode &append_children(const Ts&... children) {
    //qDebug() << "*** append_children&";
    // LATER optimize against following list n times (O(n) rather than O(n²))
    (append_child(children), ...);
    return *this;
  }
  inline PfNode &prepend_child(const PfNode &child);
  inline PfNode &prepend_child(PfNode &&child);
  inline PfNode &remove_children_by_name(const Utf8String &name) {
    Fragment::delete_if(&_fragments, [name](Fragment *f) {
      auto node = f->child();
      return node && *node^name;
    });
    return *this;
  }
  /** @return First child matching name, null when not found
   * If you want its text content rather call attribute() or operator[]. */
  [[nodiscard]] inline const PfNode &first_child(const Utf8String &name) const {
    for (auto f: Fragment::FragmentForwardRange(_fragments))
      if (auto child = f->child(); child && *child^name)
        return *child;
    return _empty;
  }
  [[nodiscard]] inline const PfNode &first_child() const {
    for (auto f: Fragment::FragmentForwardRange(_fragments))
      if (auto child = f->child(); child)
        return *child;
    return _empty;
  }
  /** Return a pair of first two children with a given name.
   *  Never fail (the children can be null).
   *  Convenient e.g. to display an error message if a child is duplicated:
   *  auto [child,child2] = node.first_two_children("foo")
   *  if (!!child2) { warn_ignoring_duplicate_child(); }
   *  if (!!child) { perform_regular_action(); }
   *  else { warn_lacking_child(); } */
  [[nodiscard]] inline std::pair<const PfNode&, const PfNode&>
  first_two_children(const Utf8String &name) const {
    const PfNode *first = 0;
    for (auto f: Fragment::FragmentForwardRange(_fragments))
      if (const auto &child = f->child(); child && *child^name) {
        if (!first)
          first = child;
        else
          return { *first, *child };
      }
    return { first ? *first : _empty, _empty };
  }
  [[nodiscard]] inline bool has_child(const Utf8String &name) const {
    for (auto f: Fragment::FragmentForwardRange(_fragments))
      if (const auto &child = f->child(); child && *child^name)
        return true;
    return false;
  }
  [[nodiscard]] inline bool has_child() const {
    for (auto f: Fragment::FragmentForwardRange(_fragments))
      if (f->type() == Child)
        return true;
    return false;
  }
  [[deprecated("use has_child() instead")]]
  inline bool hasChild(const Utf8String &name) const {
    return has_child(name); }

  // content: as low level types //////////////////////////////////////////////

  /** Return content as text.
   *  Merge all text fragments (which have been partially trimmed when parsing
   *  external PF data).
   *  Return "" when there is no text data.
   */
  [[nodiscard]] inline Utf8String content_as_text() const {
    Utf8String s = ""_u8;
    bool first = true;
    for (auto f: Fragment::FragmentForwardRange(_fragments))
      if (const auto &text = f->text(); !text.isEmpty()) {
        if (first)
          first = false;
        else
          s += ' ';
        s += text;
      }
    return s;
  }
  [[nodiscard]] inline Utf8String content_as_binary() const {
    QByteArray content;
    bool previous_was_text = false;
    for (auto f: Fragment::FragmentForwardRange(_fragments)) {
      auto type = f->type();
      if (previous_was_text && type == Text)
        content += ' ';
      if (const auto &data = f->unwrapped_data(); !data.isEmpty()) {
        content += data;
        previous_was_text = type == Text;
      }
    }
    return content;
  }
  /** syntaxic sugar for content_as_text().toNumber() */
  template <p6::arithmetic T>
  inline T content_as_number(bool *ok, T def = {}) const {
    return content_as_text().toNumber<T>(ok, def); }
  /** syntaxic sugar for content_as_text().toNumber() */
  template <p6::arithmetic T>
  inline T content_as_number(T def = {}) const {
    return content_as_text().toNumber<T>(def); }
  /** set the whole content to text, removing any other payload fragment (text,
   *  and binary) or comment fragment. */
  inline PfNode &set_text(const Utf8String &text) {
      Fragment::delete_if(&_fragments, [](Fragment *f) STATIC_LAMBDA {
        return f->type() != Fragment::Child; });
      Fragment::push_back(&_fragments, new TextFragment(text));
      return *this;
  }
  /** append text fragment, i.e. either add a new text fragment or if possible
   *  append " "+text (with separating space) at the end of last fragment
   *  (last fragment must be a text fragment for the merge to occur), but if
   *  add_space_before_text is set to false */
  inline PfNode &append_text_fragment(const Utf8String &text) {
    if (_fragments) {
      // bool merged =
      _fragments->merge_text_on_last_fragment(text);
      // qDebug() << "*** merged text on last fragment:" << text << merged;
    } else {
      _fragments = new TextFragment(text);
      // qDebug() << "*** appended as first fragment:" << text;
    }
    return *this;
  }
  inline PfNode &append_comment_fragment(const Utf8String &comment) {
    if (_fragments)
      Fragment::push_back(&_fragments, new CommentFragment(comment));
    else
      _fragments = new CommentFragment(comment);
    return *this;
  }
  inline PfNode &append_loaded_binary_fragment(
      const QByteArray &unwrapped_data, const Utf8String &wrappings = {}) {
    if (_fragments)
      Fragment::push_back(
            &_fragments, new LoadedBinaryFragment(unwrapped_data, wrappings));
    else
      _fragments = new LoadedBinaryFragment(unwrapped_data, wrappings);
    return *this;
  }
  inline PfNode &append_deferred_binary_fragment(
      QIODevice *file, qsizetype pos, qsizetype len, bool should_cache) {
    if (_fragments)
      Fragment::push_back(
            &_fragments, new DeferredBinaryFragment(
              file, pos, len, should_cache));
    else
      _fragments = new DeferredBinaryFragment(file, pos, len, should_cache);
    return *this;
  }
  template <bool even_on_text_fragments = false, bool recursive = true>
  inline PfNode &set_wrappings(const Utf8String &wrappings) {
    for (auto f = _fragments; f; f = f->_next) {
      if (even_on_text_fragments || f->type() != Fragment::Text)
        f->set_wrappings(wrappings);
      if (recursive)
        if (auto child = f->child(); !!child)
          child->set_wrappings<even_on_text_fragments>(wrappings);
    }
    return *this;
  }
  [[nodiscard]] static bool is_registered_wrapping(const Utf8String &wrapping);
  [[nodiscard]] static Utf8String normalized_wrappings(
      const Utf8String &wrappings);
  static void unwrap_binary(QByteArray *wrapped_data, Utf8String &wrappings,
                            const PfOptions &options);
  static void enwrap_binary(QByteArray *unwrapped_data, Utf8String &wrappings,
                            const PfOptions &options);

  // content: as attributes ///////////////////////////////////////////////////

  /** Return a child content knowing the child name.
    * def if no such child exists.
    * "" if child exists but has no text content.
    * If several children have the same name the first one is choosen.
    * The intent is to emulate XML attributes, hence the name. */
  [[nodiscard]] inline Utf8String attribute(
      const Utf8String &name, const Utf8String &def = {}) const {
    const auto &child = first_child(name);
    return !!child ? child.content_as_text() : def; }
  /** Syntaxic sugar: node["foo"] === node.attribute("foo") */
  [[nodiscard]] inline Utf8String operator[](const Utf8String &name) const {
    return attribute(name);
  }
#if __cpp_multidimensional_subscript >= 202110L
  // C++23 gcc 12 (13 w/ default arg)
  /** Syntaxic sugar: node["foo","bar"] === node.attribute("foo","bar") */
  [[nodiscard]] inline Utf8String operator[](const Utf8String &name,
                                             const Utf8String &def) const {
    return attribute(name, def);
  }
#endif
  [[deprecated("use attribute() or operator[] instead")]]
  inline QString utf16attribute(
      const Utf8String &name, const Utf8String &def = {}) const {
    return attribute(name, def);
  }
  /** Set a first child named 'name' content to 'value' and remove any other
    * child named 'name'. If no child is named 'name' append it. */
  inline PfNode &set_attribute(const Utf8String &name, const Utf8String &value) {
    Fragment::set_attribute(&_fragments, name, value);
    return *this;
  }
  inline PfNode &set_attribute(
      const Utf8String &name, p6::arithmetic auto number) {
    Fragment::set_attribute(&_fragments, name, Utf8String::number(number));
    return *this;
  }
  [[deprecated("use set_attribute() instead")]]
  inline PfNode &setAttribute(const Utf8String &name, const Utf8String &value) {
    return set_attribute(name, value);
  }
  [[deprecated("use set_attribute() instead")]]
  inline PfNode &setAttribute(
      const Utf8String &name, p6::arithmetic auto number) {
    return set_attribute(name, number);
  }

  // content: as high level types /////////////////////////////////////////////

  /** Text content splited into std::pair<Utf8String,Utf8String> at the first
   *  whitespace.
   *  @see Utf8String::split_as_pair() */
  inline auto content_as_text_pair() const {
    return content_as_text().split_as_pair(); }
  /** Range of children content splited into std::pair<Utf8String,Utf8String>
   *  at the first whitespace, one pair per matching child.
   *  @see Utf8String::split_as_pair() */
  inline auto children_as_text_pairs_range(const Utf8String &name) const {
    return std::views::all(Fragment::FragmentForwardRange(_fragments))
        | std::views::filter([name](const Fragment *f) { auto child = f->child(); return child && *child^name; })
        | std::views::transform([](const Fragment *f) STATIC_LAMBDA { return f->child()->content_as_text().split_as_pair(); });
  }
  /** Syntaxic sugar for content_as_text().spit() using ascii whitespace and
   *  keeping empty parts. */
  [[nodiscard]] inline Utf8StringList content_as_strings() const {
    return content_as_text()
        .split(Utf8String::AsciiWhitespace, Qt::KeepEmptyParts); }
  [[nodiscard]] inline QStringList content_as_utf16strings() const {
    return content_as_strings().toUtf16StringList(); }
  [[deprecated("use content_as_text() instead")]]
  inline Utf8String contentAsUtf8() const {
    return content_as_text(); }
  [[deprecated("use content_as_text() instead")]]
  inline QString contentAsUtf16() const {
    return content_as_text(); }
  [[deprecated("use content_as_strings() or content_as_utf16list() instead")]]
  inline QStringList contentAsStringList() const {
    return content_as_strings().toUtf16StringList(); }
  [[deprecated("use content_as_strings() instead")]]
  inline Utf8StringList contentAsUtf8List() const {
    return content_as_strings(); }
  [[deprecated("use content_as_text_pair() instead")]]
  inline QStringList contentAsTwoStringsList() const {
    auto pair = content_as_text_pair();
    return {pair.first, pair.second};
  }

  // formating ////////////////////////////////////////////////////////////////

  /** Write the whole PfNode tree in PF format. */
  qint64 write_pf(QIODevice *target, const PfOptions &options = {}) const;
  /** Convert the whole PfNode tree to PF format.
   *  Internaly uses write_pf(). */
  Utf8String as_pf(const PfOptions &options = {}) const;
  [[deprecated("use as_pf instead")]]
  inline Utf8String toPf(const PfOptions &options = {}) const {
    return as_pf(options); }
  /** Convert the whole PfNode tree to PF format.
   *  Using human readable options (indentation + comments).
   *  Internaly uses write_pf(). */
  Utf8String as_text() const {
    return as_pf(PfOptions().with_indent(2).with_comments()); }
  [[deprecated("use as_text instead")]]
  Utf8String toString() const { return as_text(); }
  /** Is c a PF whitespace char? i.e. among: space \t \r \n */
  static inline bool is_pf_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  }
  /** Is c a PF reserved char? i.e. among: whitesppace ( ) # | \ ' " */
  static inline bool is_pf_reserved_char(char c) {
    return c == '(' || c == ')' || c == '#' || c == '|' || c == '\\'
        || c == '\'' || c == '"' || is_pf_whitespace(c);
  }
  /** protect with a backslash any character in input that is reserved (has a
   *  special meanging) in PF so that the returned string can be written as is
   *  as a text fragment in a PF file. */
  static inline Utf8String escaped_text(const Utf8String &input) {
    const char *begin = input.constData(), *s = begin;
    for (; *s; ++s)
      if (must_escape_char_within_text(*s, s[1]))
        [[unlikely]] goto reserved_char_found;
    return input; // short path: copy nothing because there was nothing to do
  reserved_char_found:;
    Utf8String output = input.sliced(0, s-begin);
    for (; *s; ++s)
      if (must_escape_char_within_text(*s, s[1]))
        output = output + '\\' + *s;
      else
        output += *s;
    return output;
  }

  // position ////////////////////////////////////////////////////////////////

  /** return node position in input parsed data, if availlable.
   *  position returned is the character just after node name. */
  Utf8String position() const;
  bool has_position() const { return _line; }
  quint64 line() const { return _line; }
  quint64 column() const { return _column; }
  PfNode &set_pos(quint64 line, quint64 column) {
    _line = line;
    _column = column;
    return *this;
  }

private:
  struct PfWriter;
  qint64 write_pf(size_t depth, PfWriter *writer,
                  const PfOptions &options) const;
  /** Escaping a char within text fragment depends on the char but also the
   *  next one because space should only be escaped when followed by other
   *  whitespace chars. The first one is free ;-) */
  static inline bool must_escape_char_within_text(char c, char next) {
    if (c == ' ') // special case: escape space only when not alone or at end
      return Utf8String::is_ascii_whitespace(next) || next == '\0';
    return is_pf_reserved_char(c); // general case: escape reserved chars
  }
  /** Build an indentation string depending on the depth in the tree and
   *  indentation options. */
  static inline Utf8String indentation_string(
      int depth, const PfOptions &options) {
   if (options._indent_size == 0)
     return {};
   if (options._indent_with_tabs)
     return "\t"_ba.repeated(options._indent_size*depth);
   return " "_ba.repeated(options._indent_size*depth);
  }
  [[nodiscard]] inline QList<const Fragment*> fragments_as_list() const {
    QList<const Fragment*> list;
    for (auto f: Fragment::FragmentForwardRange(_fragments))
      list += f;
    return list;
  }
  static_assert(sizeof(Fragment) == 16);
  static_assert(sizeof(Utf8String) == 24); // _text
  static_assert(sizeof(TextFragment) == 40); // 16+24
};

static_assert(sizeof(Utf8String) == 24); // _name
static_assert(sizeof(PfNode) == 48); // 24+8×3

struct LIBP6CORESHARED_EXPORT PfNode::ChildFragment : Fragment {
  PfNode _child;
  inline ChildFragment(const PfNode &child) : _child(child) { }
  inline ChildFragment(PfNode &&child) : _child(std::move(child)) { }
  FragmentType type() const override;
  Fragment *deep_copy() const override;
  PfNode *child() override;
};

PfNode &PfNode ::append_child(const PfNode &child) {
  // qDebug() << "append_child&";
  Fragment::push_back(&_fragments, new ChildFragment(child));
  return *this;
}

PfNode &PfNode::append_child(PfNode &&child) {
  // qDebug() << "append_child&&";
  Fragment::push_back(&_fragments, new ChildFragment(std::move(child)));
  return *this;
}

PfNode &PfNode::prepend_child(const PfNode &child) {
  Fragment::push_front(&_fragments, new ChildFragment(child));
  return *this;
}

PfNode &PfNode::prepend_child(PfNode &&child) {
  Fragment::push_front(&_fragments, new ChildFragment(child));
  return *this;
}

inline uint qHash(const PfNode &n) { return qHash(n.name()); }

#endif // PFNODE_H
