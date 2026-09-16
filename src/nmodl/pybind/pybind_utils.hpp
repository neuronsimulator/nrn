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

namespace nb = nanobind;

namespace nmodl {
namespace pybind_util {

template <typename StringType> class pythonibuf : public std::streambuf {
private:
  using traits_type = std::streambuf::traits_type;

  const static std::size_t put_back_ = 1;
  const static std::size_t buf_sz = 1024 + put_back_;
  char d_buffer[buf_sz];

  nb::object pyistream;
  nb::object pyread;

  pythonibuf(const pythonibuf &) = delete;
  pythonibuf &operator=(const pythonibuf &) = delete;

  int_type underflow() {
    if (gptr() < egptr()) {
      return traits_type::to_int_type(*gptr());
    }

    char *base = d_buffer;
    char *start = base;
    if (eback() == base) {
      std::memmove(base, egptr() - put_back_, put_back_);
      start += put_back_;
    }
    nb::object data = pyread(buf_sz - (start - base));
    if (data.is_none()) {
      return traits_type::eof();
    }
    if (nb::isinstance<nb::str>(data)) {
      data = data.attr("encode")("utf-8");
    }
    char *buffer = nullptr;
    Py_ssize_t length = 0;
    if (PyBytes_AsStringAndSize(data.ptr(), &buffer, &length) < 0) {
      throw nb::python_error();
    }
    if (length == 0) {
      return traits_type::eof();
    }
    std::memcpy(start, buffer, static_cast<size_t>(length));
    setg(base, start, start + length);
    return traits_type::to_int_type(*gptr());
  }

public:
  pythonibuf(nb::object pyistream)
      : pyistream(std::move(pyistream)), pyread(this->pyistream.attr("read")) {
    char *end = d_buffer + buf_sz;
    setg(end, end, end);
  }
};

class pythonobuf : public std::streambuf {
private:
  nb::object pywrite;

  pythonobuf(const pythonobuf &) = delete;
  pythonobuf &operator=(const pythonobuf &) = delete;

  std::streamsize xsputn(const char *s, std::streamsize n) override {
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

} // namespace pybind_util
} // namespace nmodl

class VisitorOStreamResources {
protected:
  std::unique_ptr<nmodl::pybind_util::pythonobuf> buf;
  std::unique_ptr<std::ostream> ostream;

public:
  VisitorOStreamResources() = default;
  VisitorOStreamResources(nb::object object)
      : buf(new nmodl::pybind_util::pythonobuf(std::move(object))),
        ostream(new std::ostream(buf.get())) {}
  void flush() {
    if (ostream) {
      ostream->flush();
    }
  }
};
