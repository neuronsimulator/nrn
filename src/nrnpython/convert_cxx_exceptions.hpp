#pragma once
#include <exception>
#include<stdexcept>

namespace nrn {
namespace detail {
template <class T>
T error_value();

template <>
inline PyObject* error_value<PyObject*>() {
    return nullptr;
}

template <>
inline int error_value<int>() {
    return -1;
}

template <>
inline long error_value<long>() {
    return -1;
}

// template<> ssize_t error_value<ssize_t>() {
//   return -1;
// }
}  // namespace detail

template <class F, class... Args>
struct convert_cxx_exceptions_trait {
    using return_type = typename std::result_of<F(Args...)>::type;

    static return_type error_value() {
        return detail::error_value<return_type>();
    }
};

template <class F, class... Args>
static typename convert_cxx_exceptions_trait<F, Args...>::return_type convert_cxx_exceptions(
    F f,
    Args&&... args) {
    try {
        return f(std::forward<Args>(args)...);
    } catch (const std::runtime_error& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
    } catch (std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
    } catch (...) {
        PyErr_SetString(PyExc_Exception, "Unknown C++ exception.");
    }

    return convert_cxx_exceptions_trait<F, Args...>::error_value();
}
}  // namespace nrn

