#ifndef nrnpy_utils_h
#define nrnpy_utils_h

#include <Python.h>

inline bool is_python_string(PyObject* python_string) {
  return PyUnicode_Check(python_string) || PyBytes_Check(python_string);
}

class Py2NRNString {
 public:
  Py2NRNString(PyObject* python_string, bool disable_release = false) {
    disable_release_ = disable_release;
    str_ = NULL;
    if (PyUnicode_Check(python_string)) {
      PyObject* py_bytes = PyUnicode_AsASCIIString(python_string);
      str_ = strdup(PyBytes_AsString(py_bytes));
      Py_XDECREF(py_bytes);
    } else if (PyBytes_Check(python_string)) {
      str_ = strdup(PyBytes_AsString(python_string));
    }
  }

  ~Py2NRNString() {
    if (!disable_release_) {
      free(str_);
    }
  }
  inline char* c_str() const { return str_; }

 private:
  Py2NRNString();
  Py2NRNString(const Py2NRNString&);
  Py2NRNString& operator=(const Py2NRNString&);

  char* str_;
  bool disable_release_;
};

#endif /* end of include guard: nrnpy_utils_h */
