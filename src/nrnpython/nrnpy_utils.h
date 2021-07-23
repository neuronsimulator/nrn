#ifndef nrnpy_utils_h
#define nrnpy_utils_h

#include <nrnwrap_Python.h>
#include <cassert>

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
      if (py_bytes) {
        str_ = strdup(PyBytes_AsString(py_bytes));
        if (!str_) { // errno is ENOMEM
          PyErr_SetString(PyExc_MemoryError, "strdup in Py2NRNString");
        }
      }
      Py_XDECREF(py_bytes);
    } else if (PyBytes_Check(python_string)) {
      str_ = strdup(PyBytes_AsString(python_string));
      // assert(strlen(str_) == PyBytes_Size(python_string))
      // not checking for embedded '\0'
      if (!str_) { // errno is ENOMEM
        PyErr_SetString(PyExc_MemoryError, "strdup in Py2NRNString");
      }
    } else { // Neither Unicode or PyBytes
      PyErr_SetString(PyExc_TypeError, "Neither Unicode or PyBytes");
    }
  }

  ~Py2NRNString() {
    if (!disable_release_ && str_) {
      free(str_);
    }
  }
  inline char* c_str() const { return str_; }
  inline bool err() const { return str_ == NULL; }
  inline void set_pyerr(PyObject* type, const char* message) {
    PyObject* ptype = NULL;
    PyObject* pvalue = NULL;
    PyObject* ptraceback = NULL;
    if (err()) {
      PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    }
    if (pvalue && ptype) {
      PyErr_SetObject(type, PyUnicode_FromFormat("%s (Note: %S: %S)", message, ptype, pvalue));
    }else{
      PyErr_SetString(type, message);
    }
    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);
  }
  inline char* get_pyerr() {
    PyObject* ptype = NULL;
    PyObject* pvalue = NULL;
    PyObject* ptraceback = NULL;
    PyObject* pstr = NULL;
    if (err()) {
      PyErr_Fetch(&ptype, &pvalue, &ptraceback);
      if (pvalue) {
        PyObject* pstr = PyObject_Str(pvalue);
        if (pstr) {
          const char* err_msg = PyUnicode_AsUTF8(pstr);
          if (err_msg) {
            str_ = strdup(err_msg);
          } else {
            str_ = strdup("get_pyerr failed at PyUnicode_AsUTF8");
          }
        } else {
          str_ = strdup("get_pyerr failed at PyObject_Str");
        }
      } else {
        str_ = strdup("get_pyerr failed at PyErr_Fetch");
      }
    }
    PyErr_Clear(); // in case could not turn pvalue into c_str.
    Py_XDECREF(pstr);
    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);
    return str_;
  }
 private:
  Py2NRNString();
  Py2NRNString(const Py2NRNString&);
  Py2NRNString& operator=(const Py2NRNString&);

  char* str_;
  bool disable_release_;
};


class PyLockGIL
{
public:
  PyLockGIL()
      : state_(PyGILState_Ensure())
      , locked_(true)
  {}

  /* This is mainly used to unlock manually prior to a hoc_execerror() call
   * since this uses longjmp()
   */
  void release() {
    assert(locked_);
    locked_ = false;
    PyGILState_Release(state_);
  }

  ~PyLockGIL() {
    release();
  }

private:
  PyLockGIL(const PyLockGIL&);
  PyLockGIL& operator=(const PyLockGIL&);

  PyGILState_STATE state_;
  bool locked_; /* check if double unlocking */
};

extern void nrnpy_sec_referr();
#define CHECK_SEC_INVALID(sec) {if (!sec->prop) { nrnpy_sec_referr(); return NULL;}}

#endif /* end of include guard: nrnpy_utils_h */
