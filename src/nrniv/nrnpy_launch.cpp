// Start CPython under nrniv or special, and run Python code for them: scripts, -c, the prompt.
#include <../../nrnconf.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

#include <InterViews/resource.h>
#include "hocstr.h"
#include "nrnoc2iv.h"
#include "nrnpy_capi.h"
#include "nrnpy_launch.h"
#include "oc_ansi.h"
#include "ocfile.h"

extern HocStr* hoc_cbufstr;
extern char* hoc_ctp;
extern const char* hoc_promptstr;
extern int nrn_global_argc;
extern char** nrn_global_argv;
extern int nrn_istty_;
extern char* hoc_xopen_file_;
extern std::size_t hoc_xopen_file_size_;
extern char* neuronhome_forward();
#if DARWIN || defined(__linux__)
extern const char* path_prefix_to_libnrniv();
#endif

namespace neuron::python {
namespace {

// The part of the launcher that Python says better. It runs in its own namespace, where the C++
// code has set executable, argv and neuron_parent.
constexpr auto bootstrap = R"py(
import importlib.machinery, os, sys

sys.executable = executable
sys.argv = argv
if neuron_parent and neuron_parent not in sys.path:
    sys.path.append(neuron_parent)
basic_path = tuple(sys.path)


def set_path(filename):
    # As python: a script's directory, links resolved, or '' for anything else.
    first = os.path.dirname(os.path.realpath(filename)) if filename else ""
    sys.path[:] = (first, *basic_path)


def compile_file(filename):
    set_path(filename)
    with open(filename, "rb") as file:
        code = compile(file.read(), filename, "exec", dont_inherit=True)
    sys.modules["__main__"].__file__ = filename
    return code


def flush():
    # stdout is buffered when it is not a terminal; HOC's output comes next.
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.flush()
        except Exception:
            pass


def import_readline():
    # Imported later, it would replace NEURON's PyOS_ReadlineFunctionPointer.
    try:
        import readline
    except ImportError:
        pass


def interact():
    # The console is the standard library's: a code.py in the working directory must not replace
    # it, also when PYTHONPATH names that directory (an empty entry, as in PYTHONPATH=:dir).
    here = os.path.realpath(os.getcwd())
    path = sys.path[:]
    sys.path[:] = [p for p in basic_path if os.path.realpath(p or os.curdir) != here]
    try:
        import code
    finally:
        sys.path[:] = path
    code.InteractiveConsole(vars(sys.modules["__main__"])).interact("", exitmsg="")


class NeuronModules:
    # hoc, nrn and _neuron_section, which the neuron package makes, before it is imported.
    @staticmethod
    def find_spec(name, path=None, target=None):
        if name in ("hoc", "nrn", "_neuron_section") and "neuron" not in sys.modules:
            import neuron

            return importlib.machinery.ModuleSpec(name, NeuronModules)

    @staticmethod
    def create_module(spec):
        return sys.modules[spec.name]

    @staticmethod
    def exec_module(module):
        pass


sys.meta_path.append(NeuronModules)
set_path("")
)py";

PyObject* launcher{};  // the namespace of the code above
wchar_t* program_name{};

std::string directory_of_neuron_package() {
    std::string path{neuronhome_forward()};
    if (path.empty()) {
        return {};
    }
#if DARWIN || defined(__linux__)
    path = path_prefix_to_libnrniv();
#else
    path += "/lib/";
#endif
    path += "python";
    return isDirExist(path + "/neuron") ? path : std::string{};
}

/// An argument of main(), or a path made of one.
Ref decode(const std::string& s) {
#if defined(WIN32)
    // main() gets the ANSI code page, and Python's filesystem encoding is UTF-8.
    return Ref{py.PyUnicode_DecodeLocale(s.c_str(), "surrogateescape")};
#else
    return Ref{py.PyUnicode_DecodeFSDefault(s.c_str())};
#endif
}

Ref launcher_function(const char* function, const char* arg = nullptr) {
    auto* const f = py.PyDict_GetItemString(launcher, function);
    if (!arg) {
        return call(f);
    }
    Ref const a = decode(arg);
    return a ? call(f, a) : Ref{};
}

/// Print the error, as python does: SystemExit ends the process.
bool succeeded(Ref const& result) {
    if (!result) {
        py.PyErr_Print();
    }
    return static_cast<bool>(result);
}

// PyConfig is not in the stable ABI. Py_Initialize reads these globals, if they are there.
bool set_flag(const char* name) {
    auto* const flag = static_cast<int*>(py.optional(name));
    if (flag) {
        *flag = 1;
    }
    return flag;
}

void set_no_site_flag() {
    if (!set_flag("Py_NoSiteFlag")) {
        Fprintf(stderr,
                "Warning: -Py_NoSiteFlag is ignored. Py_Initialize() imports site, "
                "so sitecustomize and usercustomize still run.\n");
    }
}

// As python decides it.
bool utf8_mode() {
    auto const variable = [](const char* name) {
        auto const* const value = std::getenv(name);
        return std::string_view{value ? value : ""};
    };
    if (auto const option = variable("PYTHONUTF8"); !option.empty()) {
        return option == "1";
    }
#if defined(WIN32)
    return false;
#else
    for (auto const* name: {"LC_ALL", "LC_CTYPE", "LANG"}) {
        if (auto const locale = variable(name); !locale.empty()) {
            return locale == "C" || locale == "POSIX";
        }
    }
    return true;
#endif
}

void initialize() {
    if (nrnpy_nositeflag) {
        set_no_site_flag();
    }
    if (utf8_mode()) {
        set_flag("Py_UTF8Mode");
    }
    std::string executable{nrnpy_pyexe};
#ifndef NRNPYTHON_DYNAMICLOAD
    // Python is linked. -pyexe changes what Python takes as its executable, not which Python it is.
    if (executable.empty()) {
        executable = neuron::config::default_python_executable;
    }
#endif
    // Python finds pyvenv.cfg, and with it a virtual environment, from its executable. Left alone
    // it would be nrniv or x86_64/special.
    if (!std::filesystem::path{executable}.is_absolute()) {
        throw std::runtime_error("Setting program name to a non-absolute path (" + executable +
                                 ") is not portable; try passing an absolute path to -pyexe or "
                                 "NRN_PYTHONEXE");
    }
    // Before Py_Initialize on purpose: the locale is still "C", so what it cannot represent becomes
    // surrogates, which encode back to the original bytes whatever the filesystem encoding will be.
    // Py_PreInitialize is not in the stable ABI. test/pyinit/c_locale_pyexe.py.
    program_name = py.Py_DecodeLocale(executable.c_str(), nullptr);
    if (!program_name) {
        throw std::runtime_error(
            "Could not decode the Python executable path from the locale encoding");
    }
    py.Py_SetProgramName(program_name);  // keeps the pointer
    py.Py_Initialize();

    launcher = py.PyDict_New();
    auto const set = [](const char* name, Ref const& value) {
        py.PyDict_SetItemString(launcher, name, value.get());
    };
    set("__builtins__", Ref::borrow(py.PyEval_GetBuiltins()));
    set("executable", decode(executable));
    set("neuron_parent", decode(directory_of_neuron_package()));
    Ref const argv{py.PyList_New(0)};
    for (int i = 0; i < nrn_global_argc; ++i) {
        py.PyList_Append(argv.get(), decode(nrn_global_argv[i]).get());
    }
    set("argv", argv);
    Ref const code{py.Py_CompileString(bootstrap, "<nrniv>", file_input)};
    if (!succeeded(code ? Ref{py.PyEval_EvalCode(code.get(), launcher, launcher)} : Ref{})) {
        throw std::runtime_error("Could not set up Python");
    }
}

bool launch_python() {
    static bool failed{};
    if (failed) {
        return false;
    }
    try {
        auto* const library = nrnpy_open_libpython();
        std::string missing;
        if (!py.bind(library, missing)) {
            throw std::runtime_error("libpython has no " + missing);
        }
        initialize();
        return true;
    } catch (std::exception const& e) {
        Fprintf(stderr, "Could not start Python: %s\n", e.what());
        failed = true;
        return false;
    }
}

/// Read a line at a Python prompt with NEURON's reader, which runs its events while it waits.
char* read_line(std::FILE*, std::FILE*, const char* prompt) {
    hoc_cbufstr->buf[0] = '\0';
    hoc_promptstr = prompt;
    int const r = hoc_get_line();
    if (r != 1 && r != EOF) {
        return nullptr;
    }
    auto const size = std::strlen(hoc_cbufstr->buf) + 1;
    if (r == 1) {
        hoc_ctp = hoc_cbufstr->buf + size - 1;
    }
    // Python frees it.
    auto* const raw_malloc = reinterpret_cast<void* (*) (std::size_t)>(
        py.optional("PyMem_RawMalloc"));
    auto* const line = static_cast<char*>(raw_malloc ? raw_malloc(size) : std::malloc(size));
    if (line) {
        std::memcpy(line, hoc_cbufstr->buf, size);
    }
    return line;
}

/// load_file("rel.hoc") in a script looks next to the script.
class CurrentFile {
    std::string saved_{hoc_xopen_file_ ? hoc_xopen_file_ : ""};
    static void set(const std::string& name) {
        if (!hoc_xopen_file_ || name.size() >= hoc_xopen_file_size_) {
            hoc_xopen_file_size_ = name.size() + 100;
            hoc_xopen_file_ = static_cast<char*>(erealloc(hoc_xopen_file_, hoc_xopen_file_size_));
        }
        std::memcpy(hoc_xopen_file_, name.c_str(), name.size() + 1);
    }

  public:
    explicit CurrentFile(std::string name) {
#if defined(WIN32)
        // fopen accepts /, and HOC takes \ in a string as an escape.
        std::replace(name.begin(), name.end(), '\\', '/');
#endif
        set(name);
    }
    ~CurrentFile() {
        set(saved_);
    }
};

/// Returns 1 on success, 0 on failure.
int run_file(const char* filename) {
    if (auto* const file = std::fopen(filename, "r")) {
        std::fclose(file);
    } else {
        Fprintf(stderr, "Could not open %s\n", filename);
        return 0;
    }
    if (!start()) {
        return 0;
    }
    Gil const gil{};
    CurrentFile const current{filename};
    Ref const code = launcher_function("compile_file", filename);
    auto* const vars = code ? main_namespace() : nullptr;
    bool const ok = succeeded(vars ? Ref{py.PyEval_EvalCode(code.get(), vars, vars)} : Ref{});
    succeeded(launcher_function("flush"));
    std::fflush(stdout);
    std::fflush(stderr);
    return ok;
}

void set_path(std::string_view filename) {
    if (launcher) {  // otherwise no Python code has run yet
        Gil const gil{};
        succeeded(launcher_function("set_path", std::string{filename}.c_str()));
    }
}

bool is_python_file(const char* arg) {
    auto extension = std::filesystem::path{arg}.extension().string();
#if defined(WIN32)
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
#endif
    return extension == ".py";
}

/// nrniv -python: the -c "command" or the first script, then a prompt if there is a terminal.
/// Returns whether there was an error.
int run_command_line() {
    if (!start()) {
        Fprintf(stderr, "Python not available\n");
        return 1;
    }
    Gil const gil{};
    succeeded(launcher_function("import_readline"));
    if (auto* const reader = static_cast<char* (**) (std::FILE*, std::FILE*, const char*)>(
            py.optional("PyOS_ReadlineFunctionPointer"))) {
        *reader = read_line;
    }
    bool ok{true};
    for (int i = 1; i < nrn_global_argc; ++i) {
        if (std::strcmp(nrn_global_argv[i], "-c") == 0 && i + 1 < nrn_global_argc) {
            set_path("");
            ok = succeeded(run(nrn_global_argv[i + 1]));
            break;
        }
        if (is_python_file(nrn_global_argv[i])) {
            ok = run_file(nrn_global_argv[i]);
            break;
        }
    }
    if (nrn_istty_) {
        ok = succeeded(launcher_function("interact")) && ok;
    }
    return !ok;
}

// If NEURON started Python; otherwise it is the program's.
void finalize_interpreter() {
    if (std::exchange(launcher, nullptr)) {
        finalize();
    }
}
}  // namespace

void register_launcher() {
    launch = launch_python;
    methods.interpreter_finalize = finalize_interpreter;
    methods.interpreter_run = run_command_line;
    methods.interpreter_set_path = set_path;
    p_nrnpy_pyrun = run_file;
}

}  // namespace neuron::python
