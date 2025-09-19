#pragma once
#include <exception>
#include <stdexcept>
#include <type_traits>

namespace nrn {
namespace detail {

template <class T, class = void>
struct error_value_impl;

template <>
struct error_value_impl<PyObject*> {
    static PyObject* value() {
        return nullptr;
    }
};

template <class T>
struct error_value_impl<
    T,
    typename std::enable_if<std::is_integral_v<T> && std::is_signed_v<T>>::type> {
    static T value() {
        return -1;
    }
};

}  // namespace detail

template <class F, class... Args>
struct convert_cxx_exceptions_trait {
    using return_type = std::invoke_result_t<F, Args...>;

    static return_type error_value() {
        return detail::error_value_impl<return_type>::value();
    }
};

template <class F, class... Args>
static typename convert_cxx_exceptions_trait<F, Args...>::return_type convert_cxx_exceptions(
    F f,
    Args&&... args) {
    // Same mapping of C++ exceptions to Python errors that pybind11 uses.
    try {
        return f(std::forward<Args>(args)...);
    } catch (const std::bad_alloc& e) {
        PyErr_SetString(PyExc_MemoryError, e.what());
    } catch (const std::domain_error& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
    } catch (const std::invalid_argument& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
    } catch (const std::length_error& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
    } catch (const std::out_of_range& e) {
        PyErr_SetString(PyExc_IndexError, e.what());
    } catch (const std::range_error& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
    } catch (const std::overflow_error& e) {
        PyErr_SetString(PyExc_OverflowError, e.what());
    } catch (std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
    } catch (...) {
        PyErr_SetString(PyExc_Exception, "Unknown C++ exception.");
    }

    return convert_cxx_exceptions_trait<F, Args...>::error_value();
}
}  // namespace nrn
