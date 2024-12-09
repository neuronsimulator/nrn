#pragma once

#include <cstdlib>
#include <utility>
#include <ostream>

#include "nrnpython/nrn_export.hpp"

namespace neuron {

/** A RAII wrapper for C-style strings.
 *
 *  The string must be null-terminated and allocated with `malloc`. The lifetime of the string is
 * bound to the life time of the `unique_cstr`. Certain patterns in NRN require passing on
 * ownership, this is achieved using `.release()`, which returns the contained C-string and makes
 * this object invalid.
 */
class NRN_EXPORT unique_cstr {
  public:
    unique_cstr(const unique_cstr&) = delete;
    unique_cstr(unique_cstr&& other) noexcept {
        *this = std::move(other);
    }

    unique_cstr& operator=(const unique_cstr&) = delete;
    unique_cstr& operator=(unique_cstr&& other) noexcept {
        std::free(this->str_);
        this->str_ = std::exchange(other.str_, nullptr);
        return *this;
    }

    explicit unique_cstr(char* cstr)
        : str_(cstr) {}

    ~unique_cstr() {
        std::free((void*) str_);
    }

    /** Releases ownership of the string.
     *
     *  Returns the string and makes this object invalid.
     */
    [[nodiscard]] char* release() {
        return std::exchange(str_, nullptr);
    }

    char* c_str() const {
        return str_;
    }
    bool is_valid() const {
        return str_ != nullptr;
    }

  private:
    char* str_ = nullptr;
};

}  // namespace neuron
