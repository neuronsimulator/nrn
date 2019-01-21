#include "parser/nmodl_driver.hpp"
#include <memory>
#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>


namespace pybind11 {
namespace detail {

template <typename StringType>
struct CopyFromPython {
    void operator()(char* start, size_t n, StringType data) {
        char* buffer;
        ssize_t length;
        if (PYBIND11_BYTES_AS_STRING_AND_SIZE(data.ptr(), &buffer, &length))
            pybind11_fail("Unable to extract string contents! (invalid type)");
        std::memcpy(start, buffer, n);
    }
};

template <>
struct CopyFromPython<str> {
    void operator()(char* start, size_t n, str data) {
        if (PyUnicode_Check(data.ptr())) {
            data = reinterpret_steal<object>(PyUnicode_AsUTF8String(data.ptr()));
            if (!data)
                pybind11_fail("Unable to extract string contents! (encoding issue)");
        }
        CopyFromPython<bytes>()(start, n, data);
    }
};


template <typename StringType>
class pythonibuf: public std::streambuf {
  private:
    using traits_type = std::streambuf::traits_type;

    const static std::size_t put_back_ = 1;
    const static std::size_t buf_sz = 1024 + put_back_;
    char d_buffer[buf_sz];

    object pyistream;
    object pyread;

    // copy ctor and assignment not implemented;
    // copying not allowed
    pythonibuf(const pythonibuf&);
    pythonibuf& operator=(const pythonibuf&);

    int_type underflow() {
        if (gptr() < egptr()) {  // buffer not exhausted
            return traits_type::to_int_type(*gptr());
        }

        char* base = d_buffer;
        char* start = base;
        if (eback() == base) {
            std::memmove(base, egptr() - put_back_, put_back_);
            start += put_back_;
        }
        StringType data = pyread(buf_sz - (start - base));
        size_t n = len(data);
        if (n == 0) {
            return traits_type::eof();
        }
        CopyFromPython<StringType>()(start, n, data);
        setg(base, start, start + n);
        return traits_type::to_int_type(*gptr());
    }


  public:
    pythonibuf(object pyistream)
        : pyistream(pyistream)
        , pyread(pyistream.attr("read")) {
        char* end = d_buffer + buf_sz;
        setg(end, end, end);
    }
};
}  // namespace detail
}  // namespace pybind11

namespace py = pybind11;


class PyDriver: public nmodl::Driver {
  public:
    using nmodl::Driver::Driver;

    bool parse_stream(py::object object) {
        py::object tiob = py::module::import("io").attr("TextIOBase");
        if (py::isinstance(object, tiob)) {
            py::detail::pythonibuf<py::str> buf(object);
            std::istream istr(&buf);
            return nmodl::Driver::parse_stream(istr);
        } else {
            py::detail::pythonibuf<py::bytes> buf(object);
            std::istream istr(&buf);
            return nmodl::Driver::parse_stream(istr);
        }
    }
};

// forward declaration of submodule init functions
void init_visitor_module(py::module& m);
void init_ast_module(py::module& m);

PYBIND11_MODULE(_nmodl, m_nmodl) {
    py::class_<PyDriver> nmodl_driver(m_nmodl, "Driver");
    nmodl_driver.def(py::init<bool, bool>());
    nmodl_driver.def(py::init<>())
        .def("parse_string", &PyDriver::parse_string)
        .def("parse_file", &PyDriver::parse_file)
        .def("parse_stream", &PyDriver::parse_stream)
        .def("ast", &PyDriver::ast);

    init_visitor_module(m_nmodl);
    init_ast_module(m_nmodl);
}
