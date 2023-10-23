#include <../../nrnconf.h>
#include "nrn_ansi.h"

#include "../utils/profile/profiler_interface.h"

long hoc_nframe, hoc_nstack;

#if !HAVE_IV
#define Session void
int hoc_main1(int, const char**, const char**);
void hoc_main1_init(const char*, const char**);
#endif

#include <stdio.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#if !defined(__APPLE__)
extern char** environ;
#else
#include <crt_externs.h>
#endif
#endif

#if HAVE_IV
#ifdef WIN32
#include <IV-Win/MWlib.h>
void iv_display_scale(float);
#endif

#include <assert.h>
#include "ivoc.h"
#include "idraw.h"
#include <InterViews/style.h>
#endif
#include "string.h"
#include "oc2iv.h"
#include "nrnmpi.h"
#include "nrnpy.h"

#if defined(IVX11_DYNAM)
#include <IV-X11/ivx11_dynam.h>
#endif

#if 1
void pr_profile();
#define PR_PROFILE pr_profile();
#else
#define PR_PROFILE /**/
#endif
/*****************************************************************************/

#if HAVE_IV
static PropertyData properties[] = {{"*gui", "sgimotif"},
                                    {"*PopupWindow*overlay", "true"},
                                    {"*PopupWindow*saveUnder", "on"},
                                    {"*TransientWindow*saveUnder", "on"},
                                    {"*background", "#ffffff"},
                                    {"*brush_width", "0"},
                                    {"*double_buffered", "on"},
                                    {"*flat", "#aaaaaa"},
#ifdef MINGW
                                    {"*font", "*Arial*bold*--12*"},
                                    {"*MenuBar*font", "*Arial*bold*--12*"},
                                    {"*MenuItem*font", "*Arial*bold*--12*"},
#endif
                                    {"*foreground", "#000000"},
                                    {"*synchronous", "off"},
                                    {"*malloc_debug", "on"},

                                    {"*Scene_background", "#ffffff"},
                                    {"*Scene_foreground", "#000000"},
                                    {"*FieldEditor*background", "#ffffff"},
                                    //{"*background", "#cfffff"},
                                    {"*default_brush", "0"},
                                    {"*view_margin", ".25"},
                                    {"*pwm_dismiss_button", "Iconify"},
                                    {"*dismiss_button", "Close"},
                                    {"*use_transient_windows", "yes"},
                                    {"*nrn_library", " $(NEURONHOME)/lib"},
                                    {"*view_pick_epsilon", "2"},
                                    {"*pwm_canvas_height", "120"},
                                    {"*pwm_paper_height", "11"},
                                    {"*pwm_paper_width", "8.5"},
                                    {"*pwm_paper_resolution", ".5"},
                                    {"*pwm_pixel_resolution", "0"},
                                    {"*window_manager_offset_x", "5."},
                                    {"*window_manager_offset_y", "26."},
                                    {"*pwm_print_file_filter", "*.ps"},
                                    {"*pwm_idraw_file_filter", "*.id"},
                                    {"*pwm_ascii_file_filter", "*"},
                                    {"*pwm_save_file_filter", "*.ses"},
                                    {"*pwm_idraw_prologue", "$(NEURONHOME)/lib/prologue.id"},
                                    {"*pwm_postscript_filter", "sed 's;/Adobe-;/;'"},
                                    {"*SlowSlider*autorepeatStart", "0."},
                                    {"*scene_print_border", "1"},
                                    {"*radioScale", ".9"},
                                    {"*stepper_size", "20."},
                                    {"*xvalue_field_size_increase", "10."},
                                    {"*xvalue_format", "%.5g"},
                                    {"*graph_axis_default", "0"},
                                    {"*shape_scale_file", "$(NEURONHOME)/lib/shape.cm2"},
                                    {"*shape_quadedge", "0"},
                                    {"*CBWidget_ncolor", "10"},
                                    {"*CBWidget_nbrush", "10"},
                                    {"*units_on_flag", "on"},
                                    {"*NFRAME", "0"},  // see src/oc/code.cpp for the default value
                                    {"*NSTACK", "0"},  // see src/oc/code.cpp for the default value
                                    {"*Py_NoSiteFlag", "0"},
                                    {"*python", "off"},
                                    {"*nopython", "off"},
                                    {"*err_dialog", "off"},
                                    {"*banner", "on"},
                                    {"*pyexe", ""},
                                    {NULL}};

static OptionDesc options[] = {{"-dismissbutton", "*dismiss_button", OptionValueImplicit, "Close"},
                               {"-extrapipeinput", "*extrapipeinput", OptionValueNext},
                               {"-dll", "*nrnmechdll", OptionValueNext},
                               {"-showwinio", "*showwinio", OptionValueImplicit, "on"},
                               {"-hidewinio", "*showwinio", OptionValueImplicit, "off"},
                               {"-isatty", "*isatty", OptionValueImplicit, "1"},
                               {"-notatty", "*isatty", OptionValueImplicit, "-1"},
                               {"-neosim", "*neosim", OptionValueImplicit, "on"},
                               {"-bbs_nhost", "*bbs_nhost", OptionValueNext},
                               {"-NSTACK", "*NSTACK", OptionValueNext},
                               {"-NFRAME", "*NFRAME", OptionValueNext},
                               {"--version", "*print_nrn_version", OptionValueImplicit, "on"},
                               {"-python", "*python", OptionValueImplicit, "on"},
                               {"-nopython", "*nopython", OptionValueImplicit, "on"},
                               {"-pyexe", "*pyexe", OptionValueNext},
                               {"-Py_NoSiteFlag", "*Py_NoSiteFlag", OptionValueImplicit, "1"},
                               {"-nobanner", "*banner", OptionValueImplicit, "off"},
#if defined(WIN32)
                               {"-mswin_scale", "*mswin_scale", OptionValueNext},
#endif
                               {NULL}};
#endif  // HAVE_IV

extern int hoc_obj_run(const char*, Object*);
extern int nrn_istty_;
extern int nrn_nobanner_;
extern void hoc_final_exit();
void ivoc_final_exit();
#if (defined(NRNMECH_DLL_STYLE) || defined(WIN32))
extern const char* nrn_mech_dll;
#endif
#if defined(USE_PYTHON)
int nrn_nopython;
extern int use_python_interpreter;
std::string nrnpy_pyexe;
#endif

/*****************************************************************************/
// exported initialized data so shared libraries can have assert pure-text
#if HAVE_IV
int Oc::refcnt_ = 0;
Session* Oc::session_ = 0;
HandleStdin* Oc::handleStdin_ = 0;
std::ostream* OcIdraw::idraw_stream = 0;
#endif
/*****************************************************************************/
extern void ivoc_cleanup();
#if OCSMALL
static char* ocsmall_argv[] = {0, "difus.hoc"};
#endif
#if defined(WIN32) && HAVE_IV
extern HWND hCurrWnd;
#endif


extern void setneuronhome(const char*);
extern const char* neuron_home;
int hoc_xopen1(const char* filename, const char* rcs);
extern int units_on_flag_;
extern double hoc_default_dll_loaded_;
extern int hoc_print_first_instance;
int nrnpy_nositeflag;

#if !defined(MINGW)
extern void setneuronhome(const char*) {
    neuron_home = getenv("NEURONHOME");
}
#endif

#if DARWIN || defined(__linux__)
#include "nrnwrap_dlfcn.h"
#include <string>

/* It is definitely now the case on mac and I think sometimes the case on
linux that dlopen needs a full path to the file. A path to the binary
is not necessarily sufficent as one may launch python or nrniv on the
target machine and the lib folder cannot be derived from the location of
the python executable.This seems to be robust if nrn_version is inside a
shared library.
The return value ends with a '/' and if the prefix cannot be determined
the return value is "".
*/
const char* path_prefix_to_libnrniv() {
    static char* path_prefix_to_libnrniv_ = NULL;
    if (!path_prefix_to_libnrniv_) {
        Dl_info info;
        int rval = dladdr((void*) nrn_version, &info);
        std::string name;
        if (rval) {
            if (info.dli_fname) {
                name = info.dli_fname;
                if (info.dli_fname[0] == '/') {  // likely full path
                    size_t last_slash = name.rfind("/");
                    path_prefix_to_libnrniv_ = strndup(name.c_str(), last_slash + 1);
                    path_prefix_to_libnrniv_[last_slash + 1] = '\0';
                }
            }
        }
        if (!path_prefix_to_libnrniv_) {
            path_prefix_to_libnrniv_ = strdup("");
        }
    }
    return path_prefix_to_libnrniv_;
}
#endif  // DARWIN || defined(__linux__)

int ivocmain(int, const char**, const char**);
int ivocmain_session(int, const char**, const char**, int start_session);
int (*p_neosim_main)(int, const char**, const char**);
extern int nrn_global_argc;
extern const char** nrn_global_argv;
int always_false;
extern int nrn_is_python_extension;
extern void hoc_nrnmpi_init();
#if NRNMPI_DYNAMICLOAD
extern void nrnmpi_stubs();
extern std::string nrnmpi_load();
#endif

// some things are defined in libraries earlier than they are used so...
#include <nrnisaac.h>
static void force_load() {
    if (always_false) {
        nrnisaac_new();
    }
}

#ifdef MINGW
// see iv/src/OS/directory.cpp
#include <sys/stat.h>
static bool isdir(const char* p) {
    struct stat st;
    bool b = stat(p, &st) == 0 && S_ISDIR(st.st_mode);
    // printf("isdir %s returns %d\n", p, b);
    return b;
}
#endif

// in case we are running without IV then get some important args this way
static bool nrn_optarg_on(const char* opt, int* argc, char** argv);
static char* nrn_optarg(const char* opt, int* argc, char** argv);
static int nrn_optargint(const char* opt, int* argc, char** argv, int dflt);

static bool nrn_optarg_on(const char* opt, int* pargc, const char** argv) {
    char* a;
    int i;
    for (i = 0; i < *pargc; ++i) {
        if (strcmp(opt, argv[i]) == 0) {
            *pargc -= 1;
            for (; i < *pargc; ++i) {
                argv[i] = argv[i + 1];
            }
            //			printf("nrn_optarg_on %s  return true\n", opt);
            return true;
        }
    }
    return false;
}

static const char* nrn_optarg(const char* opt, int* pargc, const char** argv) {
    const char* a;
    int i;
    for (i = 0; i < *pargc - 1; ++i) {
        if (strcmp(opt, argv[i]) == 0) {
            a = argv[i + 1];
            *pargc -= 2;
            for (; i < *pargc; ++i) {
                argv[i] = argv[i + 2];
            }
            //			printf("nrn_optarg %s  return %s\n", opt, a);
            return a;
        }
    }
    return 0;
}

static int nrn_optargint(const char* opt, int* pargc, const char** argv, int dflt) {
    const char* a;
    int i;
    i = dflt;
    a = nrn_optarg(opt, pargc, argv);
    if (a) {
        sscanf(a, "%d", &i);
    }
    //	printf("nrn_optargint %s return %d\n", opt, i);
    return i;
}

#if USENRNJAVA
void nrn_InitializeJavaVM();
#endif

void hoc_nrnmpi_init() {
#if NRNMPI
    if (!nrnmpi_use) {
#if NRNMPI_DYNAMICLOAD
        nrnmpi_stubs();
        auto const pmes = nrnmpi_load();
        if (!pmes.empty()) {
            std::cout << pmes << std::endl;
        }
#endif

        char** foo = (char**) nrn_global_argv;
        nrnmpi_init(2, &nrn_global_argc, &foo);
        // if (nrnmpi_myid == 0) {printf("hoc_nrnmpi_init called nrnmpi_init\n");}
        // turn off gui for all ranks > 0
        if (nrnmpi_myid_world > 0) {
            hoc_usegui = 0;
            hoc_print_first_instance = 0;
        }
    }
#endif
    hoc_ret();
    hoc_pushx(0.0);
}

/**
 * Main entrypoint function into the HOC interpeter
 *
 * This function simply calls ivocmain_session with the \c start_session = 1.
 *
 * \note This is part of NEURON's public interface
 *
 * \note \c env argument should not be used as it might become invalid
 *
 * \param argc argument count as found in C/C++ \c main functions
 * \param argv argument vector as found in C/C++ \c main functions
 * \param env environment variable array as optionally found in main functions.
 * \return 0 on success, otherwise error code.
 */
int ivocmain(int argc, const char** argv, const char** env) {
    return ivocmain_session(argc, argv, env, 1);
}
/**
 * This used to be ivocmain, the main entrypoint to the HOC interpreter
 *
 * ivocmain_session parses command line argument, calls of initialization
 * functions and drops into an interactive HOC session.
 * This function is called for example by the "real main" in \c nrnmain.cpp ,
 * but can be also called from other external user applications that use
 * NEURON.
 * Additionally to the original implemenation a new parameter \c start_session
 * was introduced to control whether an interactive HOC session shoudl be started
 * or simply NEURON and all data structures be initialized and control returned
 * to the caller.
 *
 * \note \c env argument should not be used as it might become invalid
 *
 * \param argc argument count as found in C/C++ \c main functions
 * \param argv argument vector as found in C/C++ \c main functions
 * \param env environment variable array as optionally found in main functions.
 * \param start_session set to 1 for default behavior (drop into interactive HOC session
 * otherwise set to 0. If set to 1, but compiled with python support this function will
 * still directly return (since in that mode we don't need an interactive HOC session
 * either.
 * \return 0 on success, otherwise error code.
 */
int ivocmain_session(int argc, const char** argv, const char** env, int start_session) {
    nrn::Instrumentor::init_profile();

    // third arg should not be used as it might become invalid
    // after putenv or setenv. Instead, if necessary use
    // #include <unistd.h>
    // extern char** environ;
    int i;
    //	prargs("at beginning", argc, argv);
    force_load();
    nrn_global_argc = argc;
    // https://en.cppreference.com/w/cpp/language/main_function, note that argv is
    // of length argc + 1 and argv[argc] is null.
    nrn_global_argv = new const char*[argc + 1];
    for (i = 0; i < argc + 1; ++i) {
        nrn_global_argv[i] = argv[i];
    }
    nrn_assert(nrn_global_argv[nrn_global_argc] == nullptr);
    if (nrn_optarg_on("-help", &argc, argv) || nrn_optarg_on("-h", &argc, argv)) {
        printf(
            "nrniv [options] [fileargs]\n\
  options:\n\
    -dll filename    dynamically load the linked mod files.\n\
    -h               print this help message\n\
    -help            print this help message\n\
    -isatty          unbuffered stdout, print prompt when waiting for stdin\n\
    -mpi             launched by mpirun or mpiexec, in parallel environment\n\
    -mswin_scale float   scales gui on screen\n\
    -music           launched as a process of the  MUlti SImulator Coordinator\n\
    -NSTACK integer  size of stack (default 1000)\n\
    -NFRAME integer  depth of function call nesting (default 200)\n\
    -nobanner        do not print startup banner\n\
    -nogui           do not send any gui info to screen\n\
    -notatty         buffered stdout and no prompt\n\
    -python          Python is the interpreter\n\
    -pyexe path      Python to use if python (or python3 fallback) not right.\n\
    -nopython        Do not initialize Python\n\
    -Py_NoSiteFlag   Set Py_NoSiteFlag=1 before initializing Python\n\
    -realtime        For hard real-time simulation for dynamic clamp\n\
    --version        print version info\n\
    and all InterViews and X11 options\n\
  fileargs:          any number of following\n\
    -                input from stdin til ^D (end of file)\n\
    -c \"statement\"    execute next statement\n\
    filename         execute contents of filename\n\
");
        exit(0);
    }
    if (nrn_optarg_on("--version", &argc, argv)) {
        printf("%s\n", nrn_version(1));
        exit(0);
    }
    if (nrn_optarg_on("-nobanner", &argc, argv)) {
        nrn_nobanner_ = 1;
    }
    if (nrn_optarg_on("-Py_NoSiteFlag", &argc, argv)) {
        nrnpy_nositeflag = 1;
    }

    nrnmpi_numprocs = nrn_optargint("-bbs_nhost", &argc, argv, nrnmpi_numprocs);
    hoc_usegui = 1;
    if (nrn_optarg_on("-nogui", &argc, argv)) {
        hoc_usegui = 0;
        hoc_print_first_instance = 0;
    }
    if (nrnmpi_numprocs > 1) {
        hoc_usegui = 0;
        hoc_print_first_instance = 0;
    }
#if NRNMPI
    if (nrnmpi_use) {
        hoc_usegui = 0;
        hoc_print_first_instance = 0;
    }
#else

    // check if user is trying to use -mpi or -p4 when it was not
    // enabled at build time.  If so, issue a warning.

    int b;
    b = 0;
    for (i = 0; i < argc; ++i) {
        if (strncmp("-p4", (argv)[i], 3) == 0) {
            b = 1;
            break;
        }
        if (strcmp("-mpi", (argv)[i]) == 0) {
            b = 1;
            break;
        }
    }
    if (b) {
        printf(
            "Warning: detected user attempt to enable MPI, but MPI support was disabled at build "
            "time.\n");
    }

#endif

#if defined(IVX11_DYNAM)
    if (hoc_usegui && ivx11_dyload()) {
        hoc_usegui = 0;
        hoc_print_first_instance = 0;
    }
#endif

#if NRN_MUSIC
    nrn_optarg_on("-music", &argc, argv);
#else
    if (nrn_optarg_on("-music", &argc, argv)) {
        printf("Warning: attempt to enable MUSIC but MUSIC support was disabled at build time.\n");
    }
#endif

#if !HAVE_IV
    hoc_usegui = 0;
    hoc_print_first_instance = 0;
#endif
    int our_argc = argc;
    const char** our_argv = argv;
    int exit_status = 0;
    Session* session = NULL;
#if !defined(MINGW)
    // Gary Holt's first pass at this was:
    //
    // Set the NEURONHOME environment variable.  This should override any setting
    // in the environment, so someone doesn't accidently use data files from an
    // old version of neuron.
    //
    // But I have decided to use the environment variable if it exists
    neuron_home = getenv("NEURONHOME");
    if (!neuron_home) {
#if defined(HAVE_SETENV)
        setenv("NEURONHOME", NEURON_DATA_DIR, 1);
        neuron_home = NEURON_DATA_DIR;
#else
#error "I don't know how to set environment variables."
// Maybe in this case the user will have to set it by hand.
#endif
        // putenv and setenv may invalidate env but we no longer
        // use it so following should not be needed
    }

#else  // Not unix:
    neuron_home = getenv("NEURONHOME");
    if (!neuron_home) {
        setneuronhome((argc > 0) ? argv[0] : 0);
    }
    if (!neuron_home) {
#if defined(WIN32) && HAVE_IV
        MessageBox(0,
                   "No NEURONHOME environment variable.",
                   "NEURON Incomplete Installation",
                   MB_OK);
#else
        neuron_home = ".";
        fprintf(stderr,
                "Warning: no NEURONHOME environment variable-- setting\
 to %s\n",
                neuron_home);
#endif
    }
#endif  // !unix.

#if HAVE_IV
#if OCSMALL
    our_argc = 2;
    our_argv = new char*[2];
    our_argv[0] = "Neuron";
    our_argv[1] = ":lib:hoc:macload.hoc";
    session = new Session("NEURON", our_argc, our_argv, options, properties);
#else
#if defined(WIN32)
    IFGUI
    session = new Session("NEURON", our_argc, (char**) our_argv, options, properties);
    ENDGUI
#else
    IFGUI
    if (getenv("DISPLAY")) {
        session = new Session("NEURON", our_argc, (char**) our_argv, options, properties);
    } else {
        fprintf(stderr,
                "Warning: no DISPLAY environment variable.\
\n--No graphics will be displayed.\n");
        hoc_usegui = 0;
    }
    ENDGUI
#endif
    auto const nrn_props_size = strlen(neuron_home) + 20;
    char* nrn_props = new char[nrn_props_size];
    if (session) {
        std::snprintf(nrn_props, nrn_props_size, "%s/%s", neuron_home, "lib/nrn.defaults");
#ifdef WIN32
        FILE* f;
        if ((f = fopen(nrn_props, "r")) != (FILE*) 0) {
            fclose(f);
            session->style()->load_file(String(nrn_props), -5);
        } else {
#ifdef MINGW
            std::snprintf(nrn_props, nrn_props_size, "%s/%s", neuron_home, "lib/nrn.def");
#else
            std::snprintf(nrn_props, nrn_props_size, "%s\\%s", neuron_home, "lib\\nrn.def");
#endif
            if ((f = fopen(nrn_props, "r")) != (FILE*) 0) {
                fclose(f);
                session->style()->load_file(String(nrn_props), -5);
            } else {
                char buf[256];
                Sprintf(buf, "Can't load NEURON resources from %s[aults]", nrn_props);
                printf("%s\n", buf);
            }
        }
#else
        session->style()->load_file(String(nrn_props), -5);
#endif
        char* h = getenv("HOME");
        if (h) {
            std::snprintf(nrn_props, nrn_props_size, "%s/%s", h, ".nrn.defaults");
            session->style()->load_file(String(nrn_props), -5);
        }
    }
    delete[] nrn_props;

#endif /*OCSMALL*/

    if (session) {
        session->style()->find_attribute("NSTACK", hoc_nstack);
        session->style()->find_attribute("NFRAME", hoc_nframe);
        IFGUI
        if (session->style()->value_is_on("err_dialog")) {
            nrn_err_dialog_active_ = 1;
        }
        ENDGUI
    } else
#endif  // HAVE_IV
    {
        hoc_nstack = nrn_optargint("-NSTACK", &our_argc, our_argv, 0);
        hoc_nframe = nrn_optargint("-NFRAME", &our_argc, our_argv, 0);
    }

#if defined(USE_PYTHON)
    nrn_nopython = 0;
    if (!nrn_is_python_extension) {
#if HAVE_IV
        if (session) {
            if (session->style()->value_is_on("nopython")) {
                nrn_nopython = 1;
            }
            String str;
            if (session->style()->find_attribute("pyexe", str)) {
                nrnpy_pyexe = str.string();
            }
        } else
#endif
        {
            if (nrn_optarg_on("-nopython", &our_argc, our_argv)) {
                nrn_nopython = 1;
            }
            if (const char* buf = nrn_optarg("-pyexe", &our_argc, our_argv)) {
                nrnpy_pyexe = buf;
            }
        }
    }
#endif  // USE_PYTHON

#if defined(WIN32) && HAVE_IV
    IFGUI
    double scale = 1.;
    int pw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    if (pw < 1100) {
        scale = 1200. / double(pw);
    }
    session->style()->find_attribute("mswin_scale", scale);
    iv_display_scale(float(scale));
    ENDGUI
#endif

    // just eliminate from arg list
    nrn_optarg_on("-mpi", &our_argc, our_argv);

#if (defined(NRNMECH_DLL_STYLE) || defined(WIN32))
#if HAVE_IV
    String str;
    if (session) {
        if (session->style()->find_attribute("nrnmechdll", str)) {
            nrn_mech_dll = str.string();
        }
    } else
#endif
    {  // if running without IV.
        nrn_mech_dll = nrn_optarg("-dll", &our_argc, our_argv);
        // may be duplicated since nrnbbs adds all args to special
        // which is often a script that adds a -dll arg
        nrn_optarg("-dll", &our_argc, our_argv);
    }
#if NRNMPI
    if (nrnmpi_use && !nrn_mech_dll) {
        // for heterogeneous clusters, mpirun allows different programs
        // but not different arguments. So the -dll is insufficient.
        // Therefore we check to see if it makes sense to load
        // a dll from the usual location.
        // Actually this is done by default in src/nrnoc/init.cpp
    }
#endif

#endif  // NRNMECH_DLL_STYLE


#if HAVE_IV
    if (session) {
        long i;
        if (session->style()->find_attribute("isatty", i)) {
            nrn_istty_ = i;
        }
    } else
#endif
    {
        nrn_istty_ = nrn_optarg_on("-isatty", &our_argc, our_argv);
        if (nrn_istty_ == 0) {
            nrn_istty_ = nrn_optarg_on("-notatty", &our_argc, our_argv);
            if (nrn_istty_ == 1) {
                nrn_istty_ = -1;
            }
        }
    }

#if HAVE_IV
    if (session && session->style()->value_is_on("units_on_flag")) {
        units_on_flag_ = 1;
    }
    Oc oc(session, our_argv[0], env);
#else
    hoc_main1_init(our_argv[0], env);
#endif  // HAVE_IV

#if USENRNJAVA
    nrn_InitializeJavaVM();
#endif
#if OCSMALL
    if (argc == 1) {
        ocsmall_argv[0] = our_argv[0];
        exit_status = oc.run(2, ocsmall_argv);
    } else
#endif
#if defined(USE_PYTHON)
#if HAVE_IV
        if (session && session->style()->value_is_on("python")) {
        use_python_interpreter = 1;
    }
#endif
    if (nrn_optarg_on("-python", &our_argc, our_argv)) {
        use_python_interpreter = 1;
    }

    if (nrn_is_python_extension) {
        return 0;
    }
    if (neuron::python::methods.interpreter_start) {
        neuron::python::methods.interpreter_start(1);
    }
    if (use_python_interpreter && !neuron::python::methods.interpreter_start) {
        fprintf(stderr, "Python not available\n");
        exit(1);
    }
#endif
    if (start_session) {
#if HAVE_IV
        exit_status = oc.run(our_argc, our_argv);
#else
        exit_status = hoc_main1(our_argc, our_argv, env);
#endif
    } else {
        return 0;
    }
#if HAVE_IV
    if (session && session->style()->value_is_on("neosim")) {
        if (p_neosim_main) {
            (*p_neosim_main)(argc, argv, env);
        } else {
            printf(
                "neosim not available.\nModify nrn/src/nrniv/Imakefile and remove "
                "nrniv/$CPU/netcvode.o\n");
        }
    }
#endif
    PR_PROFILE
#if defined(USE_PYTHON)
    if (use_python_interpreter) {
        // process the .py files and an interactive interpreter
        if (neuron::python::methods.interpreter_start &&
            neuron::python::methods.interpreter_start(2) != 0) {
            // We encountered an error when processing the -c argument or Python
            // script given on the commandline.
            exit_status = 1;
        }
    }
    if (neuron::python::methods.interpreter_start) {
        neuron::python::methods.interpreter_start(0);
    }
#endif
    hoc_final_exit();
    ivoc_final_exit();
    nrn::Instrumentor::finalize_profile();

    return exit_status;
}

void ivoc_final_exit() {
#if NRNMPI
    nrnmpi_terminate();
#endif
}

extern void hoc_ret(), hoc_pushx(double);

void hoc_single_event_run() {
#if HAVE_IV
    IFGUI
    void single_event_run();

    single_event_run();
    ENDGUI
#endif
    hoc_ret();
    hoc_pushx(1.);
}

#if !HAVE_IV
int run_til_stdin() {
    return 1;
}
void hoc_notify_value() {}
#endif
