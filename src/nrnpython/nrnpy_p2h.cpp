#include <../../nrnconf.h>

#include <cstdio>

#include <InterViews/resource.h>
#include <nrnoc2iv.h>
#include <classreg.h>
#include "nrnpython.h"
#include "hoccontext.h"
#include "nrnpy.h"
#include "nrnpy_utils.h"
#include "oc_ansi.h"

#include "parse.hpp"

#include <nanobind/nanobind.h>

namespace nb = nanobind;

static char* nrnpyerr_str();
static nb::object nrnpy_pyCallObject(nb::callable, nb::object);
static PyObject* main_module;
static PyObject* main_namespace;

struct Py2Nrn final {
    ~Py2Nrn() {
        nanobind::gil_scoped_acquire lock{};
        Py_XDECREF(po_);
    }
    int type_{};  // 0 toplevel
    PyObject* po_{};
};

static void call_python_with_section(Object* pyact, Section* sec) {
    nb::callable po = nb::borrow<nb::callable>(((Py2Nrn*) pyact->u.this_pointer)->po_);
    nanobind::gil_scoped_acquire lock{};

    nb::tuple args = nb::make_tuple(reinterpret_cast<PyObject*>(newpysechelp(sec)));
    nb::object r = nrnpy_pyCallObject(po, args);
    if (!r.is_valid()) {
        char* mes = nrnpyerr_str();
        if (mes) {
            Fprintf(stderr, "%s\n", mes);
            free(mes);
            hoc_execerror("Call of Python Callable failed", NULL);
        }
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
    }
}

static void* opaque_obj2pyobj(Object* ho) {
    assert(ho && ho->ctemplate->sym == nrnpy_pyobj_sym_);
    PyObject* po = ((Py2Nrn*) ho->u.this_pointer)->po_;
    assert(po);
    return po;
}

int nrnpy_ho_eq_po(Object* ho, PyObject* po) {
    if (ho->ctemplate->sym == nrnpy_pyobj_sym_) {
        return ((Py2Nrn*) ho->u.this_pointer)->po_ == po;
    }
    return 0;
}

// contain same Python object
static int pysame(Object* o1, Object* o2) {
    if (o2->ctemplate->sym == nrnpy_pyobj_sym_) {
        return nrnpy_ho_eq_po(o1, ((Py2Nrn*) o2->u.this_pointer)->po_);
    }
    return 0;
}

PyObject* nrnpy_hoc2pyobject(Object* ho) {
    PyObject* po = ((Py2Nrn*) ho->u.this_pointer)->po_;
    if (!po) {
        if (!main_module) {
            main_module = PyImport_AddModule("__main__");
            main_namespace = PyModule_GetDict(main_module);
            Py_INCREF(main_module);
            Py_INCREF(main_namespace);
        }
        po = main_module;
    }
    return po;
}

Object* nrnpy_pyobject_in_obj(PyObject* po) {
    Py2Nrn* pn = new Py2Nrn();
    pn->po_ = po;
    Py_INCREF(po);
    pn->type_ = 1;
    Object* on = hoc_new_object(nrnpy_pyobj_sym_, (void*) pn);
    hoc_obj_ref(on);
    return on;
}

static nb::object nrnpy_pyCallObject(nb::callable callable, nb::object args) {
    // When hoc calls a PythonObject method, then in case python
    // calls something back in hoc, the hoc interpreter must be
    // at the top level
    auto interp = HocTopContextManager();
    nb::tuple tup(args);
    nb::object p = nb::steal(PyObject_CallObject(callable.ptr(), tup.ptr()));
#if 0
printf("PyObject_CallObject callable\n");
PyObject_Print(callable, stdout, 0);
printf("\nargs\n");
PyObject_Print(args, stdout, 0);
printf("\nreturn %p\n", p);
#endif
    // It would be nice to handle the error here, ending with a hoc_execerror
    // for any Exception (note, that does not include SystemExit). However
    // since many, but not all, of the callers need to clean up and
    // release the GIL, errors get handled by the caller or higher up.
    // The almost generic idiom is:
    /**
    if (!p) {
      char* mes = nrnpyerr_str();
      if (mes) {
        Fprintf(stderr, "%s\n", mes);
        free(mes);
        hoc_execerror("Call of Python Callable failed", NULL);
      }
      if (PyErr_Occurred()) {
        PyErr_Print(); // Python process will exit with the error code specified by the
    SystemExit instance.
      }
    }
    **/
    return p;
}

static void py2n_component(Object* ob, Symbol* sym, int nindex, int isfunc) {
#if 0
	if (isfunc) {
	printf("py2n_component %s.%s(%d)\n", hoc_object_name(ob), sym->name, nindex);
	}else{
	printf("py2n_component %s.%s[%d]\n", hoc_object_name(ob), sym->name, nindex);
	}
#endif
    int i;
    Py2Nrn* pn = (Py2Nrn*) ob->u.this_pointer;
    auto head = nb::borrow(pn->po_);
    nb::object tail;
    nanobind::gil_scoped_acquire lock{};
    if (pn->type_ == 0) {  // top level
        if (!main_module) {
            main_module = PyImport_AddModule("__main__");
            main_namespace = PyModule_GetDict(main_module);
            Py_INCREF(main_module);
            Py_INCREF(main_namespace);
        }
        tail = nb::steal(PyRun_String(sym->name, Py_eval_input, main_namespace, main_namespace));
    } else {
        if (strcmp(sym->name, "_") == 0) {
            tail = head;
        } else {
            tail = head.attr(sym->name);
        }
    }
    if (!tail) {
        PyErr_Print();
        hoc_execerror("No attribute:", sym->name);
    }
    Object* on;
    nb::object result;
    if (isfunc) {
        nb::list args{};
        for (i = 0; i < nindex; ++i) {
            nb::object arg = nb::steal(nrnpy_hoc_pop("isfunc py2n_component"));
            if (!arg) {
                auto err = Py2NRNString::get_pyerr();
                hoc_execerr_ext("arg %d error: %s", i, err.c_str());
            }
            args.append(arg);
        }
        args.reverse();
        // printf("PyObject_CallObject %s %p\n", sym->name, tail);
        result = nrnpy_pyCallObject(nb::borrow<nb::callable>(tail), args);
        // PyObject_Print(result, stdout, 0);
        // printf("  result of call\n");
        if (!result) {
            char* mes = nrnpyerr_str();
            if (mes) {
                Fprintf(stderr, "%s\n", mes);
                free(mes);
                hoc_execerror("PyObject method call failed:", sym->name);
            }
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
            return;
        }
    } else if (nindex) {
        nb::object arg;
        int n = hoc_pop_ndim();
        if (n > 1) {
            hoc_execerr_ext(
                "%d dimensional python objects "
                "can't be accessed from hoc with var._[i1][i2]... syntax. "
                "Must use var._[i1]._[i2]... hoc syntax.",
                n);
        }
        if (hoc_stack_type() == NUMBER) {
            arg = nb::int_((long) hoc_xpop());
        } else {
            // I don't think it is syntactically possible
            // for this to be a VAR. It is possible for it to
            // be an Object but the GetItem below will raise
            // TypeError: list indices must be integers or slices, not hoc.HocObject
            arg = nb::steal(nrnpy_hoc_pop("nindex py2n_component"));
        }
        result = tail[arg];
        if (!result) {
            PyErr_Print();
            hoc_execerror("Python get item failed:", hoc_object_name(ob));
        }
    } else {
        result = tail;
    }
    // printf("py2n_component %s %d %s result refcount=%d\n", hoc_object_name(ob),
    // ob->refcount, sym->name, result->ob_refcnt);
    // if numeric, string, or python HocObject return those
    if (nrnpy_numbercheck(result.ptr())) {
        hoc_pop_defer();
        double d = static_cast<double>(nb::float_(result));
        hoc_pushx(d);
    } else if (is_python_string(result.ptr())) {
        char** ts = hoc_temp_charptr();
        // TODO double check that this doesn't leak.
        *ts = Py2NRNString::as_ascii(result.ptr()).release();
        hoc_pop_defer();
        hoc_pushstr(ts);
    } else {
        // PyObject_Print(result, stdout, 0);
        on = nrnpy_po2ho(result.ptr());
        hoc_pop_defer();
        hoc_push_object(on);
        if (on) {
            on->refcount--;
        }
    }
}

static void hpoasgn(Object* o, int type) {
    int err = 0;
    int nindex;
    Symbol* sym;
    nb::object poright;
    if (type == NUMBER) {
        poright = nb::steal(PyFloat_FromDouble(hoc_xpop()));
    } else if (type == STRING) {
        poright = nb::steal(Py_BuildValue("s", *hoc_strpop()));
    } else if (type == OBJECTVAR || type == OBJECTTMP) {
        Object** po2 = hoc_objpop();
        poright = nb::steal(nrnpy_ho2po(*po2));
        hoc_tobj_unref(po2);
    } else {
        hoc_execerror("Cannot assign that type to PythonObject", (char*) 0);
    }
    auto stack_value = hoc_pop_object();
    assert(o == stack_value.get());
    auto poleft = nb::borrow(nrnpy_hoc2pyobject(o));
    sym = hoc_spop();
    nindex = hoc_ipop();
    // printf("hpoasgn %s %s %d\n", hoc_object_name(o), sym->name, nindex);
    if (nindex == 0) {
        err = PyObject_SetAttrString(poleft.ptr(), sym->name, poright.ptr());
    } else if (nindex == 1) {
        int ndim = hoc_pop_ndim();
        assert(ndim == 1);
        auto key = nb::steal(PyLong_FromDouble(hoc_xpop()));
        nb::object a;
        if (strcmp(sym->name, "_") == 0) {
            a = nb::borrow(poleft);
        } else {
            a = nb::steal(PyObject_GetAttrString(poleft.ptr(), sym->name));
        }
        if (a) {
            err = PyObject_SetItem(a.ptr(), key.ptr(), poright.ptr());
        } else {
            err = -1;
        }
    } else {
        hoc_execerr_ext(
            "%d dimensional python objects "
            "can't be accessed from hoc with var._[i1][i2]... syntax. "
            "Must use var._[i1]._[i2]... hoc syntax.",
            nindex);
    }
    if (err) {
        PyErr_Print();
        hoc_execerror("Assignment to PythonObject failed", NULL);
    }
}

static nb::object hoccommand_exec_help1(nb::object po) {
    if (nb::tuple::check_(po)) {
        nb::object args = po[1];
        if (!nb::tuple::check_(args)) {
            args = nb::make_tuple(args);
        }
        return nrnpy_pyCallObject(po[0], args);
    } else {
        return nrnpy_pyCallObject(nb::borrow<nb::callable>(po), nb::tuple());
    }
}

static nb::object hoccommand_exec_help(Object* ho) {
    PyObject* po = ((Py2Nrn*) ho->u.this_pointer)->po_;
    // printf("%s\n", hoc_object_name(ho));
    return hoccommand_exec_help1(nb::borrow(po));
}

static double praxis_efun(Object* ho, Object* v) {
    nanobind::gil_scoped_acquire lock{};

    auto pc = nb::steal(nrnpy_ho2po(ho));
    auto pv = nb::steal(nrnpy_ho2po(v));
    auto po = nb::steal(Py_BuildValue("(OO)", pc.ptr(), pv.ptr()));
    nb::object r = hoccommand_exec_help1(po);
    if (!r.is_valid()) {
        char* mes = nrnpyerr_str();
        if (mes) {
            Fprintf(stderr, "%s\n", mes);
            free(mes);
            hoc_execerror("Call of Python Callable failed in praxis_efun", NULL);
        }
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
        return 1e9;  // SystemExit?
    }
    return static_cast<double>(nb::float_(r));
}

static int hoccommand_exec(Object* ho) {
    nanobind::gil_scoped_acquire lock{};

    nb::object r = hoccommand_exec_help(ho);
    if (!r.is_valid()) {
        char* mes = nrnpyerr_str();
        if (mes) {
            std::string tmp{"Python Callback failed [hoccommand_exec]:\n"};
            tmp.append(mes);
            free(mes);
            hoc_execerror(tmp.c_str(), nullptr);
        }
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
    }
    return r.is_valid();
}

static int hoccommand_exec_strret(Object* ho, char* buf, int size) {
    nanobind::gil_scoped_acquire lock{};

    nb::object r = hoccommand_exec_help(ho);
    if (r.is_valid()) {
        nb::str pn(r);
        auto str = Py2NRNString::as_ascii(pn.ptr());
        strncpy(buf, str.c_str(), size);
        buf[size - 1] = '\0';
    } else {
        char* mes = nrnpyerr_str();
        if (mes) {
            Fprintf(stderr, "%s\n", mes);
            free(mes);
            hoc_execerror("Python Callback failed", 0);
        }
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
    }
    return r.is_valid();
}

static void grphcmdtool(Object* ho, int type, double x, double y, int key) {
    nb::callable po = nb::borrow<nb::callable>(((Py2Nrn*) ho->u.this_pointer)->po_);
    nanobind::gil_scoped_acquire lock{};

    nb::tuple args = nb::make_tuple(type, x, y, key);
    nb::object r = nrnpy_pyCallObject(po, args);
    if (!r.is_valid()) {
        char* mes = nrnpyerr_str();
        if (mes) {
            Fprintf(stderr, "%s\n", mes);
            free(mes);
            hoc_execerror("Python Callback failed", 0);
        }
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
    }
}

static Object* callable_with_args(Object* ho, int narg) {
    auto po = nb::borrow(((Py2Nrn*) ho->u.this_pointer)->po_);
    nanobind::gil_scoped_acquire lock{};

    auto args = nb::steal(PyTuple_New((Py_ssize_t) narg));
    if (!args) {
        hoc_execerror("PyTuple_New failed", 0);
    }
    for (int i = 0; i < narg; ++i) {
        // not used with datahandle args.
        auto item = nb::steal(nrnpy_hoc_pop("callable_with_args"));
        if (!item) {
            hoc_execerror("nrnpy_hoc_pop failed", 0);
        }
        if (PyTuple_SetItem(args.ptr(), (Py_ssize_t) (narg - i - 1), item.release().ptr()) != 0) {
            hoc_execerror("PyTuple_SetItem failed", 0);
        }
    }

    nb::tuple r = nb::make_tuple(po, args);

    Object* hr = nrnpy_po2ho(r.release().ptr());

    return hr;
}

static double func_call(Object* ho, int narg, int* err) {
    nb::callable po = nb::borrow<nb::callable>(((Py2Nrn*) ho->u.this_pointer)->po_);
    nanobind::gil_scoped_acquire lock{};

    nb::list args{};
    for (int i = 0; i < narg; ++i) {
        nb::object item = nb::steal(nrnpy_hoc_pop("func_call"));
        if (!item) {
            hoc_execerror("nrnpy_hoc_pop failed", 0);
        }
        args.append(item);
    }
    args.reverse();

    nb::object r = nrnpy_pyCallObject(po, args);
    double rval = 0.0;
    if (!r.is_valid()) {
        if (!err || *err) {
            char* mes = nrnpyerr_str();
            if (mes) {
                Fprintf(stderr, "%s\n", mes);
                free(mes);
            }
            if (PyErr_Occurred()) {
                PyErr_Print();
            }
        } else {
            PyErr_Clear();
        }
        if (!err || *err) {
            hoc_execerror("func_call failed", NULL);
        }
        if (err) {
            *err = 1;
        }
    } else {
        if (nrnpy_numbercheck(r.ptr())) {
            rval = static_cast<double>(nb::float_(r));
        }
        if (err) {
            *err = 0;
        }  // success
    }
    return rval;
}

static double guigetval(Object* ho) {
    nb::gil_scoped_acquire lock{};
    nb::tuple po(((Py2Nrn*) ho->u.this_pointer)->po_);
    if (nb::sequence::check_(po[0]) || nb::mapping::check_(po[0])) {
        return nb::cast<double>(po[0][po[1]]);
    } else {
        return nb::cast<double>(po[0].attr(po[1]));
    }
}

static void guisetval(Object* ho, double x) {
    nb::gil_scoped_acquire lock{};
    nb::tuple po(((Py2Nrn*) ho->u.this_pointer)->po_);
    if (nb::sequence::check_(po[0]) || nb::mapping::check_(po[0])) {
        po[0][po[1]] = x;
    } else {
        po[0].attr(po[1]) = x;
    }
}

static int guigetstr(Object* ho, char** cpp) {
    nb::gil_scoped_acquire lock{};
    nb::tuple po(((Py2Nrn*) ho->u.this_pointer)->po_);
    auto name = nb::cast<std::string>(po[0].attr(po[1]));
    if (*cpp && name == *cpp) {
        return 0;
    }
    if (*cpp) {
        delete[] * cpp;
    }
    *cpp = new char[name.size() + 1];
    strcpy(*cpp, name.c_str());
    return 1;
}

static nb::callable loads;
static nb::callable dumps;

static void setpickle() {
    if (dumps) {
        return;
    }
    nb::module_ pickle = nb::module_::import_("pickle");
    dumps = pickle.attr("dumps");
    loads = pickle.attr("loads");
    if (!dumps || !loads) {
        hoc_execerror("Neither Python cPickle nor pickle are available", 0);
    }
    // We intentionally leak these, because if we don't
    // we observe SEGFAULTS during application shutdown.
    // Likely because "~nb::callable" runs after the Python
    // library cleaned up.
    dumps.inc_ref();
    loads.inc_ref();
}

// note that *size includes the null terminating character if it exists
static std::vector<char> pickle(PyObject* p) {
    auto r = nb::borrow<nb::bytes>(dumps(nb::borrow(p)));
    if (!r && PyErr_Occurred()) {
        PyErr_Print();
    }
    assert(r);
    return std::vector<char>(r.c_str(), r.c_str() + r.size());
}

static std::vector<char> po2pickle(Object* ho) {
    setpickle();
    if (ho && ho->ctemplate->sym == nrnpy_pyobj_sym_) {
        PyObject* po = nrnpy_hoc2pyobject(ho);
        return pickle(po);
    } else {
        return {};
    }
}

static nb::object unpickle(const char* s, std::size_t len) {
    return loads(nb::bytes(s, len));
}

static nb::object unpickle(const std::vector<char>& s) {
    return unpickle(s.data(), s.size());
}

static Object* pickle2po(const std::vector<char>& s) {
    setpickle();
    nb::object po = unpickle(s);
    Object* ho = nrnpy_pyobject_in_obj(po.ptr());
    return ho;
}

/** Full python traceback error message returned as string.
 *  Caller should free the return value if not NULL
 **/
static char* nrnpyerr_str() {
    if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_Exception)) {
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

        auto type = nb::steal(ptype);
        auto value = nb::steal(pvalue);
        auto traceback = nb::steal(ptraceback);

        // try for full backtrace
        nb::str py_str;
        char* cmes = nullptr;

        // Since traceback.format_exception returns list of strings, wrap
        // in neuron.format_exception that returns a string.
        if (!traceback) {
            traceback = nb::none();
        }
        nb::module_ pyth_module = nb::module_::import_("neuron");
        if (pyth_module) {
            nb::callable pyth_func = pyth_module.attr("format_exception");
            if (pyth_func) {
                py_str = nb::str(pyth_func(type, value, traceback));
            }
        }
        if (py_str) {
            cmes = strdup(py_str.c_str());
            if (!cmes) {
                Fprintf(stderr, "nrnpyerr_str: strdup failed\n");
            }
        }

        if (!py_str) {
            PyErr_Print();
            Fprintf(stderr, "nrnpyerr_str failed\n");
        }

        return cmes;
    }
    return nullptr;
}

std::vector<char> call_picklef(const std::vector<char>& fname, int narg) {
    // fname is a pickled callable, narg is the number of args on the
    // hoc stack with types double, char*, hoc Vector, and PythonObject
    // callable return must be pickleable.
    setpickle();
    nb::bytes ps(fname.data(), fname.size());

    auto callable = nb::borrow<nb::callable>(loads(ps));
    assert(callable);

    nb::list args{};
    for (int i = 0; i < narg; ++i) {
        nb::object arg = nb::steal(nrnpy_hoc_pop("call_picklef"));
        args.append(arg);
    }
    nb::object result = callable(*args);
    if (!result) {
        char* mes = nrnpyerr_str();
        if (mes) {
            Fprintf(stderr, fmt::format("{}\n", mes).c_str());
            free(mes);
            hoc_execerror("PyObject method call failed:", NULL);
        }
        if (PyErr_Occurred()) {
            PyErr_Print();
        }
    }
    return pickle(result.ptr());
}

#include "nrnmpi.h"

static std::vector<int> mk_displ(int* cnts) {
    std::vector<int> displ(nrnmpi_numprocs + 1);
    displ[0] = 0;
    for (int i = 0; i < nrnmpi_numprocs; ++i) {
        displ[i + 1] = displ[i] + cnts[i];
    }
    return displ;
}

static nb::list char2pylist(const std::vector<char>& buf,
                            const std::vector<int>& cnt,
                            const std::vector<int>& displ) {
    nb::list plist{};
    for (int i = 0; i < cnt.size(); ++i) {
        if (cnt[i] == 0) {
            plist.append(nb::none());
        } else {
            plist.append(unpickle(buf.data() + displ[i], cnt[i]));
        }
    }
    return plist;
}

#if NRNMPI
// Returns a new reference.
static PyObject* py_allgather(PyObject* psrc) {
    int np = nrnmpi_numprocs;
    auto sbuf = pickle(psrc);
    // what are the counts from each rank
    std::vector<int> rcnt(np);
    rcnt[nrnmpi_myid] = static_cast<int>(sbuf.size());
    nrnmpi_int_allgather_inplace(rcnt.data(), 1);
    auto rdispl = mk_displ(rcnt.data());
    std::vector<char> rbuf(rdispl[np]);

    nrnmpi_char_allgatherv(sbuf.data(), rbuf.data(), rcnt.data(), rdispl.data());

    return char2pylist(rbuf, rcnt, rdispl).release().ptr();
}

// Returns a new reference.
static PyObject* py_gather(PyObject* psrc, int root) {
    int np = nrnmpi_numprocs;
    auto sbuf = pickle(psrc);
    // what are the counts from each rank
    int scnt = static_cast<int>(sbuf.size());
    std::vector<int> rcnt;
    if (root == nrnmpi_myid) {
        rcnt.resize(np);
    }
    nrnmpi_int_gather(&scnt, rcnt.data(), 1, root);
    std::vector<int> rdispl;
    std::vector<char> rbuf;
    if (root == nrnmpi_myid) {
        rdispl = mk_displ(rcnt.data());
        rbuf.resize(rdispl[np]);
    }

    nrnmpi_char_gatherv(sbuf.data(), scnt, rbuf.data(), rcnt.data(), rdispl.data(), root);

    nb::object pdest = nb::none();
    if (root == nrnmpi_myid) {
        pdest = char2pylist(rbuf, rcnt, rdispl);
    }
    return pdest.release().ptr();
}

// Returns a new reference.
static PyObject* py_broadcast(PyObject* psrc, int root) {
    // Note: root returns reffed psrc.
    std::vector<char> buf{};
    int cnt = 0;
    if (root == nrnmpi_myid) {
        buf = pickle(psrc);
        cnt = static_cast<int>(buf.size());
    }
    nrnmpi_int_broadcast(&cnt, 1, root);
    if (root != nrnmpi_myid) {
        buf.resize(cnt);
    }
    nrnmpi_char_broadcast(buf.data(), cnt, root);
    nb::object pdest;
    if (root != nrnmpi_myid) {
        pdest = unpickle(buf);
    } else {
        pdest = nb::borrow(psrc);
    }
    return pdest.release().ptr();
}
#endif

// type 1-alltoall, 2-allgather, 3-gather, 4-broadcast, 5-scatter
// size for 3, 4, 5 refer to rootrank.
static Object* py_alltoall_type(int size, int type) {
    int np = nrnmpi_numprocs;  // of subworld communicator
    nb::object psrc;

    if (type == 1 || type == 5) {  // alltoall, scatter
        Object* o = *hoc_objgetarg(1);
        if (type == 1 || nrnmpi_myid == size) {  // if scatter only root must be a list
            psrc = nb::borrow(nrnpy_hoc2pyobject(o));
            if (!PyList_Check(psrc.ptr())) {
                hoc_execerror("Argument must be a Python list", 0);
            }
            if (PyList_Size(psrc.ptr()) != np) {
                if (type == 1) {
                    hoc_execerror("py_alltoall list size must be nhost", 0);
                } else {
                    hoc_execerror("py_scatter list size must be nhost", 0);
                }
            }
        }
        if (np == 1) {
            if (type == 1) {
                return o;
            } else {  // return psrc[0]
                auto pdest = nb::borrow(PyList_GetItem(psrc.ptr(), 0));
                Object* ho = nrnpy_po2ho(pdest.ptr());
                if (ho) {
                    --ho->refcount;
                }
                return ho;
            }
        }
    } else {
        // Get the raw PyObject* arg. So things like None, int, bool are preserved.
        psrc = nb::borrow(hocobj_call_arg(0));

        if (np == 1) {
            nb::object pdest;
            if (type == 4) {  // broadcast is just the PyObject
                pdest = psrc;
            } else {  // allgather and gather must wrap psrc in list
                pdest = nb::steal(PyList_New(1));
                PyList_SetItem(pdest.ptr(), 0, psrc.release().ptr());
            }
            Object* ho = nrnpy_po2ho(pdest.ptr());
            if (ho) {
                --ho->refcount;
            }
            return ho;
        }
    }

#if NRNMPI
    setpickle();
    int root;
    nb::object pdest;

    if (type == 2) {
        pdest = nb::steal(py_allgather(psrc.ptr()));
    } else if (type != 1 && type != 5) {
        root = size;
        if (root < 0 || root >= np) {
            hoc_execerror("root rank must be >= 0 and < nhost", 0);
        }
        if (type == 3) {
            pdest = nb::steal(py_gather(psrc.ptr(), root));
        } else if (type == 4) {
            pdest = nb::steal(py_broadcast(psrc.ptr(), root));
        }
    } else {
        if (type == 5) {  // scatter
            root = size;
            size = 0;  // calculate dest size (cannot be -1 so cannot return it)
        }

        std::vector<char> s{};
        std::vector<int> scnt{};

        // setup source buffer for transfer s, scnt, sdispl
        // for alltoall, each rank handled identically
        // for scatter, root handled as list all, other ranks handled as None
        if (type == 1 || nrnmpi_myid == root) {  // psrc is list of nhost items
            scnt.reserve(np);
            for (const nb::handle& p: nb::list(psrc)) {
                if (p.is_none()) {
                    scnt.push_back(0);
                    continue;
                }
                const std::vector<char> b = pickle(p.ptr());
                if (size >= 0) {
                    s.insert(std::end(s), std::begin(b), std::end(b));
                }
                scnt.push_back(static_cast<int>(b.size()));
            }

            // scatter equivalent to alltoall NONE list for not root ranks.
        } else if (type == 5 && nrnmpi_myid != root) {
            // nothing to setup, s, scnt, sdispl already NULL
        }

        // destinations need to know receive counts. Then transfer data.
        if (type == 1) {  // alltoall

            // what are destination counts
            std::vector<int> ones(np, 1);
            auto sdispl = mk_displ(ones.data());
            std::vector<int> rcnt(np);
            nrnmpi_int_alltoallv(
                scnt.data(), ones.data(), sdispl.data(), rcnt.data(), ones.data(), sdispl.data());

            // exchange
            sdispl = mk_displ(scnt.data());
            auto rdispl = mk_displ(rcnt.data());
            if (size < 0) {
                pdest = nb::make_tuple(sdispl[np], rdispl[np]);
            } else {
                std::vector<char> r(rdispl[np] + 1);  // force > 0 for all None case
                nrnmpi_char_alltoallv(
                    s.data(), scnt.data(), sdispl.data(), r.data(), rcnt.data(), rdispl.data());

                pdest = char2pylist(r, rcnt, rdispl);
            }

        } else {  // scatter

            // destination counts
            int rcnt = -1;
            nrnmpi_int_scatter(scnt.data(), &rcnt, 1, root);
            std::vector<char> r(rcnt + 1);  // rcnt can be 0
            std::vector<int> sdispl;

            // exchange
            if (nrnmpi_myid == root) {
                sdispl = mk_displ(scnt.data());
            }
            nrnmpi_char_scatterv(s.data(), scnt.data(), sdispl.data(), r.data(), rcnt, root);

            if (rcnt) {
                pdest = unpickle(r);
            } else {
                pdest = nb::none();
            }
        }
    }

    Object* ho = nrnpy_po2ho(pdest.ptr());

    // The problem here is when `pdest` is a HOC object. In that case `ho` has `refcount == 2` but
    // must be returned with refcount 0. The `ho` is contained in the `pdest` (which is why the HOC
    // refcount is 2) and will be destroyed along with the `pdest` if its reference count is 0.
    //
    // Therefore, we must first DECREF `pdest` and then decrement `ho->refcount`. If we do
    // it the other way around the `Py_DECREF` causes the Python object to be deallocated,
    // as part of that deallocation method, the contained HOC object is dereferenced, and because
    // it'll have a reference count of 0 it's also be deallocated.
    pdest.dec_ref();
    pdest.release();

    if (ho) {
        --ho->refcount;
    }
    return ho;
#else
    assert(0);
    return NULL;
#endif
}

static PyThreadState* save_thread() {
    return PyEval_SaveThread();
}
static void restore_thread(PyThreadState* g) {
    PyEval_RestoreThread(g);
}

void nrnpython_reg_real_nrnpython_cpp(neuron::python::impl_ptrs* ptrs);
void nrnpython_reg_real_nrnpy_hoc_cpp(neuron::python::impl_ptrs* ptrs);

static void* p_cons(Object*) {
    return new Py2Nrn{};
}

static void p_destruct(void* v) {
    delete static_cast<Py2Nrn*>(v);
}

/**
 * @brief Populate NEURON state with information from a specific Python.
 * @param ptrs Logically a return value; avoidi
 */
extern "C" NRN_EXPORT void nrnpython_reg_real(neuron::python::impl_ptrs* ptrs) {
    assert(ptrs);
    class2oc("PythonObject", p_cons, p_destruct, nullptr, nullptr, nullptr);
    nrnpy_pyobj_sym_ = hoc_lookup("PythonObject");
    assert(nrnpy_pyobj_sym_);
    ptrs->callable_with_args = callable_with_args;
    ptrs->call_func = func_call;
    ptrs->call_picklef = call_picklef;
    ptrs->call_python_with_section = call_python_with_section;
    ptrs->cmdtool = grphcmdtool;
    ptrs->guigetstr = guigetstr;
    ptrs->guigetval = guigetval;
    ptrs->guisetval = guisetval;
    ptrs->hoccommand_exec = hoccommand_exec;
    ptrs->hoccommand_exec_strret = hoccommand_exec_strret;
    ptrs->ho2po = nrnpy_ho2po;
    ptrs->hpoasgn = hpoasgn;
    ptrs->mpi_alltoall_type = py_alltoall_type;
    ptrs->opaque_obj2pyobj = opaque_obj2pyobj;
    ptrs->pickle2po = pickle2po;
    ptrs->po2ho = nrnpy_po2ho;
    ptrs->po2pickle = po2pickle;
    ptrs->praxis_efun = praxis_efun;
    ptrs->pysame = pysame;
    ptrs->py2n_component = py2n_component;
    ptrs->restore_thread = restore_thread;
    ptrs->save_thread = save_thread;
    // call a function in nrnpython.cpp to register the functions defined there
    nrnpython_reg_real_nrnpython_cpp(ptrs);
    // call a function in nrnpy_hoc.cpp to register the functions defined there
    nrnpython_reg_real_nrnpy_hoc_cpp(ptrs);
}
