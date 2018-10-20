#ifndef ENUM_CLASS_ITERATOR_HPP
#define ENUM_CLASS_ITERATOR_HPP


#include "flagsfwd.hpp"

#include <iterator>


namespace flags {


template <class E>
class FlagsIterator {
public:
  using flags_type = flags<E>;
  using difference_type = std::ptrdiff_t;
  using value_type = E;
  using pointer = value_type *;
  using reference = const value_type;
  using iterator_category = std::forward_iterator_tag;


  constexpr FlagsIterator() noexcept : uvalue_(0), mask_(0) {}

  constexpr FlagsIterator(const FlagsIterator &other) noexcept
  : uvalue_(other.uvalue_), mask_(other.mask_) {}


  FlagsIterator &operator++() noexcept {
    nextMask();
    return *this;
  }
  FlagsIterator operator++(int) noexcept {
    auto copy = *this;
    ++(*this);
    return copy;
  }


  constexpr reference operator*() const noexcept {
    return static_cast<value_type>(mask_);
  }


  friend inline constexpr bool operator==(const FlagsIterator &i,
                                          const FlagsIterator &j) noexcept {
    return i.mask_ == j.mask_;
  }

  friend inline constexpr bool operator!=(const FlagsIterator &i,
                                          const FlagsIterator &j) noexcept {
    return i.mask_ != j.mask_;
  }


private:
  template <class E_> friend class flags;

  using impl_type = typename flags_type::impl_type;


  explicit FlagsIterator(impl_type uv) noexcept : uvalue_(uv), mask_(1) {
    if (!(mask_ & uvalue_)) { nextMask(); }
  }

  constexpr FlagsIterator(impl_type uv, E e) noexcept
  : uvalue_(uv)
  , mask_(static_cast<impl_type>(static_cast<impl_type>(e) & uv))
  {}


  void nextMask() noexcept {
    do { mask_ <<= 1; } while (mask_ && !(mask_ & uvalue_));
  }


  impl_type uvalue_;
  impl_type mask_;
};


} // namespace flags


#endif // ENUM_CLASS_ITERATOR_HPP
