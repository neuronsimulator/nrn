#ifndef ENUM_CLASS_FLAGS_HPP
#define ENUM_CLASS_FLAGS_HPP


#include "allow_flags.hpp"
#include "iterator.hpp"

#include <bitset>
#include <initializer_list>
#include <numeric>
#include <utility>


namespace flags {


constexpr struct empty_t {
  constexpr empty_t() noexcept {}
} empty;


template <class E> class flags {
public:
  static_assert(is_flags<E>::value,
                "flags::flags is disallowed for this type; "
                "use ALLOW_FLAGS_FOR_ENUM macro.");

  using enum_type = typename std::decay<E>::type;
  using underlying_type = typename std::underlying_type<enum_type>::type;
  using impl_type = typename std::make_unsigned<underlying_type>::type;

  using iterator = FlagsIterator<enum_type>;
  using const_iterator = iterator;
  using value_type = typename iterator::value_type;
  using reference = typename iterator::reference;
  using const_reference = typename iterator::reference;
  using pointer = enum_type *;
  using const_pointer = const enum_type *;
  using size_type = std::size_t;
  using difference_type = typename iterator::difference_type;


  constexpr static std::size_t bit_size() { return sizeof(impl_type) * 8; }


private:
  template <class T, class Res = std::nullptr_t>
  using convertible = std::enable_if<std::is_convertible<T, enum_type>::value,
                                     Res>;


public:
  flags() noexcept = default;
  flags(const flags &fl) noexcept = default;
  flags &operator=(const flags &fl) noexcept = default;
  flags(flags &&fl) noexcept = default;
  flags &operator=(flags &&fl) noexcept= default;


  explicit constexpr flags(empty_t) noexcept : val_(0) {}


#ifdef ENUM_CLASS_FLAGS_FORBID_IMPLICT_CONVERSION
  explicit
#endif
  constexpr flags(enum_type e) noexcept
  : val_(static_cast<impl_type>(e)) {}

  flags &operator=(enum_type e) noexcept {
    val_ = static_cast<impl_type>(e);
    return *this;
  }


  flags(std::initializer_list<enum_type> il) noexcept : val_(0) { insert(il); }

  flags &operator=(std::initializer_list<enum_type> il) noexcept {
    clear();
    insert(il);
    return *this;
  }

  template <class ... Args>
  flags(enum_type e, Args ... args) noexcept : flags{e, args...} {}


  template <class FwIter>
  flags(FwIter b, FwIter e,
        typename convertible<decltype(*b)>::type = nullptr)
  noexcept(noexcept(std::declval<flags>().insert(std::declval<FwIter>(),
                                                 std::declval<FwIter>())))
  : val_(0)
  { insert(b, e); }


  constexpr explicit operator bool() const noexcept { return val_; }

  constexpr bool operator!() const noexcept { return !val_; }

  friend constexpr bool operator==(flags fl1, flags fl2) {
    return fl1.val_ == fl2.val_;
  }

  friend constexpr bool operator!=(flags fl1, flags fl2) {
    return fl1.val_ != fl2.val_;
  }


  constexpr flags operator~() const noexcept { return flags(~val_); }

  flags &operator|=(const flags &fl) noexcept {
    val_ |= fl.val_;
    return *this;
  }

  flags &operator&=(const flags &fl) noexcept {
    val_ &= fl.val_;
    return *this;
  }

  flags &operator^=(const flags &fl) noexcept {
    val_ ^= fl.val_;
    return *this;
  }


  flags &operator|=(enum_type e) noexcept {
    val_ |= static_cast<impl_type>(e);
    return *this;
  }

  flags &operator&=(enum_type e) noexcept {
    val_ &= static_cast<impl_type>(e);
    return *this;
  }

  flags &operator^=(enum_type e) noexcept {
    val_ ^= static_cast<impl_type>(e);
    return *this;
  }

  friend constexpr flags operator|(flags f1, flags f2) noexcept {
    return flags{static_cast<impl_type>(f1.val_ | f2.val_)};
  }

  friend constexpr flags operator&(flags f1, flags f2) noexcept {
    return flags{static_cast<impl_type>(f1.val_ & f2.val_)};
  }

  friend  constexpr flags operator^(flags f1, flags f2) noexcept {
    return flags{static_cast<impl_type>(f1.val_ ^ f2.val_)};
  }


  void swap(flags &fl) noexcept { std::swap(val_, fl.val_); }


  constexpr underlying_type underlying_value() const noexcept {
    return static_cast<underlying_type>(val_);
  }

  void set_underlying_value(underlying_type newval) noexcept {
    val_ = static_cast<impl_type>(newval);
  }


  constexpr explicit operator std::bitset<bit_size()>() const noexcept {
    return to_bitset();
  }

  constexpr std::bitset<bit_size()> to_bitset() const noexcept {
    return {val_};
  }


  constexpr bool empty() const noexcept { return !val_; }

  size_type size() const noexcept {
    return std::distance(this->begin(), this->end());
  }

  constexpr size_type max_size() const noexcept { return bit_size(); }


  iterator begin() const noexcept { return cbegin(); }
  iterator cbegin() const noexcept { return iterator{val_}; }

  constexpr iterator end() const noexcept { return cend(); }
  constexpr iterator cend() const noexcept { return {}; }


  constexpr iterator find(enum_type e) const noexcept { return {val_, e}; }

  constexpr size_type count(enum_type e) const noexcept {
    return find(e) != end() ? 1 : 0;
  }


  std::pair<iterator, iterator> equal_range(enum_type e) const noexcept {
    auto i = find(e);
    auto j = i;
    return {i, ++j};
  }


  template <class... Args>
  std::pair<iterator, bool> emplace(Args && ... args) noexcept {
    return insert(enum_type{args...});
  }

  template <class... Args>
  iterator emplace_hint(iterator, Args && ... args) noexcept {
    return emplace(args...).first;
  }


  std::pair<iterator, bool> insert(enum_type e) noexcept {
    auto i = find(e);
    if (i == end()) {
      i.mask_ = static_cast<impl_type>(e);
      val_ |= i.mask_;
      update_uvalue(i);
      return {i, true};
    }
    return {i, false};
  }

  std::pair<iterator, bool> insert(iterator, enum_type e) noexcept {
    return insert(e);
  }

  template <class FwIter>
  auto insert(FwIter i1, FwIter i2)
  noexcept(noexcept(++i1) && noexcept(*i1) && noexcept(i1 == i2))
  -> typename convertible<decltype(*i1), void>::type {
    val_ |= std::accumulate(i1, i2, impl_type{0}, [](impl_type i, enum_type e) {
      return i | static_cast<impl_type>(e);
    });
  }

  template <class Container>
  auto insert(const Container &ctn) noexcept
  -> decltype(std::begin(ctn), std::end(ctn), void()) {
    insert(std::begin(ctn), std::end(ctn));
  }


  iterator erase(iterator i) noexcept {
    val_ ^= i.mask_;
    update_uvalue(i);
    return ++i;
  }

  size_type erase(enum_type e) noexcept {
    auto e_count = count(e);
    val_ ^= static_cast<impl_type>(e);
    return e_count;
  }

  iterator erase(iterator i1, iterator i2) noexcept {
    val_ ^= flags(i1, i2).val_;
    update_uvalue(i2);
    return ++i2;
  }


  void clear() noexcept { val_ = 0; }

private:
  constexpr explicit flags(impl_type val) noexcept : val_(val) {}

  void update_uvalue(iterator &it) const noexcept { it.uvalue_ = val_; }

  impl_type val_;
};


template <class E>
void swap(flags<E> &fl1, flags<E> &fl2) noexcept { fl1.swap(fl2); }


} // namespace flags


template <class E>
constexpr auto operator|(E e1, E e2) noexcept
-> typename std::enable_if<flags::is_flags<E>::value,
                           flags::flags<E>>::type {
  return flags::flags<E>(e1) | e2;
}


template <class E>
constexpr auto operator&(E e1, E e2) noexcept
-> typename std::enable_if<flags::is_flags<E>::value,
                           flags::flags<E>>::type {
  return flags::flags<E>(e1) & e2;
}


template <class E>
constexpr auto operator^(E e1, E e2) noexcept
-> typename std::enable_if<flags::is_flags<E>::value,
                           flags::flags<E>>::type {
  return flags::flags<E>(e1) ^ e2;
}


#endif // ENUM_CLASS_FLAGS_HPP
