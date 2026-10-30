/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include <cstring>
#include <istream>
#include <memory>
#include <ostream>
#include <streambuf>
#include <string>
#include <vector>

namespace nb = nanobind;

namespace nmodl {
namespace pybind_util {

template <typename StringType>
class pythonibuf: public std::streambuf {
  private:
    using traits_type = std::streambuf::traits_type;

    // One byte so sputbackc can see the last byte of the previous fill.
    static constexpr std::size_t put_back_ = 1;
    // Characters requested from read(), and the most bytes placed in one fill.
    // read() counts characters; UTF-8 can be longer, so a fill never uses that
    // count as a byte size.
    static constexpr std::size_t chunk_ = 1024;

    std::vector<char> d_buffer;
    // Encoded bytes not yet placed in the get area.
    std::string pending_;
    std::size_t pending_off_ = 0;

    nb::object pyistream;
    nb::object pyread;

    pythonibuf(const pythonibuf&) = delete;
    pythonibuf& operator=(const pythonibuf&) = delete;

    int_type underflow() override {
        if (gptr() < egptr()) {
            return traits_type::to_int_type(*gptr());
        }

        // Save the put-back byte before resizing. The vector may move.
        char put_back_ch = 0;
        const bool have_put_back = !d_buffer.empty() && eback() == d_buffer.data() &&
                                   static_cast<std::size_t>(egptr() - eback()) >= put_back_;
        if (have_put_back) {
            put_back_ch = *(egptr() - static_cast<std::ptrdiff_t>(put_back_));
        }

        if (pending_off_ >= pending_.size()) {
            pending_.clear();
            pending_off_ = 0;
            nb::object data = pyread(chunk_);
            if (data.is_none()) {
                return traits_type::eof();
            }
            if (nb::isinstance<nb::str>(data)) {
                data = data.attr("encode")("utf-8");
            }
            char* buffer = nullptr;
            Py_ssize_t length = 0;
            if (PyBytes_AsStringAndSize(data.ptr(), &buffer, &length) < 0) {
                throw nb::python_error();
            }
            if (length <= 0) {
                return traits_type::eof();
            }
            pending_.assign(buffer, buffer + static_cast<std::size_t>(length));
        }

        const std::size_t available = pending_.size() - pending_off_;
        const std::size_t n = available < chunk_ ? available : chunk_;
        d_buffer.resize(put_back_ + n);
        char* const base = d_buffer.data();
        char* start = base;
        if (have_put_back) {
            *base = put_back_ch;
            start += put_back_;
        }
        std::memcpy(start, pending_.data() + pending_off_, n);
        pending_off_ += n;
        setg(base, start, start + n);
        return traits_type::to_int_type(*gptr());
    }

  public:
    pythonibuf(nb::object pyistream)
        : pyistream(std::move(pyistream))
        , pyread(this->pyistream.attr("read")) {}
};

class pythonobuf: public std::streambuf {
  private:
    nb::object pywrite;

    pythonobuf(const pythonobuf&) = delete;
    pythonobuf& operator=(const pythonobuf&) = delete;

    std::streamsize xsputn(const char* s, std::streamsize n) override {
        pywrite(nb::str(s, static_cast<size_t>(n)));
        return n;
    }

    int_type overflow(int_type ch) override {
        if (traits_type::eq_int_type(ch, traits_type::eof())) {
            return traits_type::not_eof(ch);
        }
        char c = traits_type::to_char_type(ch);
        pywrite(nb::str(&c, size_t{1}));
        return ch;
    }

  public:
    explicit pythonobuf(nb::object pyostream)
        : pywrite(pyostream.attr("write")) {}
};

}  // namespace pybind_util
}  // namespace nmodl

class VisitorOStreamResources {
  protected:
    std::unique_ptr<nmodl::pybind_util::pythonobuf> buf;
    std::unique_ptr<std::ostream> ostream;

  public:
    VisitorOStreamResources() = default;
    VisitorOStreamResources(nb::object object)
        : buf(new nmodl::pybind_util::pythonobuf(std::move(object)))
        , ostream(new std::ostream(buf.get())) {}
    void flush() {
        if (ostream) {
            ostream->flush();
        }
    }
};
