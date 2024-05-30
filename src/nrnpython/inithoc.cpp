#include "../../nrnconf.h"
#include "nrnmpiuse.h"
#include <stdio.h>
#include <stdint.h>
#include "nrnmpi.h"
#if defined(__MINGW32__)
#define _hypot hypot
#endif
#include "nrnpy_utils.h"
#include <stdlib.h>
#include <ctype.h>

#include <iostream>
#include <string>

extern int nrn_is_python_extension;
extern int nrn_nobanner_;
extern int ivocmain(int, const char**, const char**);
extern int nrn_main_launch;


// int nrn_global_argc;
extern char** nrn_global_argv;
extern void (*p_nrnpython_finalize)();
extern PyObject* nrnpy_hoc();

#if NRNMPI_DYNAMICLOAD
extern void nrnmpi_stubs();
extern std::string nrnmpi_load();
#endif

#if NRN_ENABLE_THREADS
#include <thread>
static std::thread::id main_thread_;
#endif

/**
 * Manage argc,argv for calling ivocmain
 * add_arg(...) will only add if name is not already in the arg list
 * returns 1 if added, 0 if not
 */
static size_t arg_size;
static int argc;
static char** argv;
static int add_arg(const char* name, const char* value) {
    if (size_t(argc + 2 + bool(value)) >= arg_size) {
        arg_size += 10;
        argv = (char**) realloc(argv, arg_size * sizeof(char*));
    }
    // Don't add if already in argv
    for (int i = 1; i < argc; ++i) {
        if (strcmp(name, argv[i]) == 0) {
            return 0;
        }
    }
    argv[argc++] = strdup(name);
    if (value) {
        argv[argc++] = strdup(value);
    }
    // To match C/C++ semantics for the main function then argv[argc] should
    // always be null. https://en.cppreference.com/w/cpp/language/main_function,
    // for example, says argv is a "Pointer to the first element of an array of
    // argc + 1 pointers, of which the last one is null". Section 11.2.1 of the
    // MPI 4.0 standard says "MPI_INIT accepts the argc and argv that are provided
    // by the arguments to main or NULL", so it seems correct to follow the rules
    // for a C/C++ main function. More pragmatically, OpenMPI assumes argv[argc]
    // is nullptr and crashes if it is not.
    argv[argc] = NULL;
    return 1;
}

/**
 * Return 1 if string, 0 otherwise.
 */
static int is_string(PyObject* po) {
    if (PyUnicode_Check(po) || PyBytes_Check(po)) {
        return 1;
    }
    return 0;
}

/**
 * Add all name:value from __main__.neuron_options dict if exists
 * to the argc,argv for calling ivocmain
 * Note: if value is None then only name is added to argv.
 * The special "-print-options" name is not added to argv but
 * causes a return value of 1. Otherwise the return value is 0.
 */
static int add_neuron_options() {
    PyObject* modules = PyImport_GetModuleDict();
    PyObject* module = PyDict_GetItemString(modules, "__main__");
    int rval = 0;
    if (!module) {
        PyErr_Clear();
        PySys_WriteStdout("No __main__  module\n");
        return rval;
    }
    PyObject* neuron_options = PyObject_GetAttrString(module, "neuron_options");
    if (!neuron_options) {
        PyErr_Clear();
        return rval;
    }
    if (!PyDict_Check(neuron_options)) {
        PySys_WriteStdout("__main__.neuron_options is not a dict\n");
        return rval;
    }
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(neuron_options, &pos, &key, &value)) {
        if (!is_string(key) || (!is_string(value) && value != Py_None)) {
            PySys_WriteStdout("A neuron_options key:value is not a string:string or string:None\n");
            continue;
        }
        Py2NRNString skey(key);
        Py2NRNString sval(value);
        if (strcmp(skey.c_str(), "-print-options") == 0) {
            rval = 1;
            continue;
        }
        add_arg(skey.c_str(), sval.c_str());
    }
    return rval;
}

/**
 * Space separated options. Must handle escaped space, '...' and "...".
 * Return 1 if contains a -print-options (not added to options)
 */

static int add_space_separated_options(const char* str) {
    int rval = 0;
    if (!str) {
        return rval;
    }
    char* s = strdup(str);
    // int state = 0; // 1 means in "...", 2 means in '...'
    for (char* cp = s; *cp; cp++) {
        while (isspace(*cp)) {  // skip spaces
            ++cp;
            if (*cp == '\0') {
                free(s);
                return rval;
            }
        }
        // start processing a token
        char* cpbegin = cp;
        char* cp1 = cpbegin;                    // in token pointer, escapes cause to lag behind
        while (!isspace(*cp) && *cp != '\0') {  // to next space delimiter
            *cp1++ = *cp++;
            if (cp1[-1] == '\\' && (isspace(*cp) || *cp == '"' || *cp == '\'')) {
                // escaped space, ", or '
                cp1[-1] = *cp++;
            } else if (cp1[-1] == '"') {  // consume to next (unescaped) "
                cp1--;                    // backup over the "
                while (*cp != '"' && *cp != '\0') {
                    *cp1++ = *cp++;
                    if (cp1[-1] == '\\' && *cp == '"') {  // escaped " inside "..."
                        cp1[-1] = *cp++;
                    }
                }
                if (*cp == '"') {
                    cp++;  // skip over the closing "
                }
            } else if (cp1[-1] == '\'') {  // consume to next (unescaped) '
                cp1--;                     // backup over the '
                while (*cp != '\'' && *cp != '\0') {
                    *cp1++ = *cp++;
                    if (cp1[-1] == '\\' && *cp == '\'') {  // escaped ' inside '...'
                        cp1[-1] = *cp++;
                    }
                }
                if (*cp == '\'') {
                    cp++;  // skip over the closing '
                }
            }
        }
        if (*cp == '\0') {  // at end of s. 'for' will return after it increments.
            --cp;
        }
        if (cp1 > cpbegin) {
            *cp1 = '\0';
        }
        if (strcmp(cpbegin, "-print-options") == 0) {
            rval = 1;
        } else {
            add_arg(cpbegin, NULL);
        }
    }
    free(s);
    return rval;
}

/**
 * Return 1 if the option exists in argv[]
 */
static int have_opt(const char* arg) {
    if (!arg) {
        return 0;
    }
    for (int i = 0; i < argc; ++i) {
        if (strcmp(arg, argv[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void nrnpython_finalize() {
#if NRN_ENABLE_THREADS
    if (main_thread_ == std::this_thread::get_id()) {
#else
    {
#endif
        Py_Finalize();
    }
#if linux
    if (system("stty sane > /dev/null 2>&1")) {
    }  // 'if' to avoid ignoring return value warning
#endif
}

static char* env[] = {0};

extern "C" PyObject* PyInit_hoc() {
#if NRN_ENABLE_THREADS
    main_thread_ = std::this_thread::get_id();
#endif

    if (nrn_global_argv) {  // ivocmain was already called so already loaded
        return nrnpy_hoc();
    }

    add_arg("NEURON", NULL);
    int print_options = add_neuron_options();
    print_options += add_space_separated_options(getenv("NEURON_MODULE_OPTIONS"));

#ifdef NRNMPI

    int flag = 0;                 // true if MPI_Initialized is called
    int mpi_mes = 0;              // for printing mpi message only once
    int libnrnmpi_is_loaded = 1;  // becomes 0 if NEURON_INIT_MPI == 0 with dynamic mpi
    std::string pmes;             // error message
    char* env_mpi = getenv("NEURON_INIT_MPI");

#if NRNMPI_DYNAMICLOAD
    nrnmpi_stubs();
    /**
     * In case of dynamic mpi build we load MPI unless NEURON_INIT_MPI is explicitly set to 0.
     * and there is no '-mpi' arg.
     * We call nrnmpi_load to load MPI library which returns:
     *  - nullptr if loading is successfull
     *  - error message in case of loading error
     */
    if (env_mpi != NULL && strcmp(env_mpi, "0") == 0 && !have_opt("-mpi")) {
        libnrnmpi_is_loaded = 0;
    }
    if (libnrnmpi_is_loaded) {
        pmes = nrnmpi_load();
        if (!pmes.empty() && env_mpi == NULL) {
            // common case on MAC distribution is no NEURON_INIT_MPI and
            // no MPI installed (so nrnmpi_load fails)
            libnrnmpi_is_loaded = 0;
        }
        if (!pmes.empty() && libnrnmpi_is_loaded) {
            std::cout << "NEURON_INIT_MPI nonzero in env (or -mpi arg) but NEURON cannot "
                         "initialize MPI because:\n"
                      << pmes << std::endl;
            exit(1);
        }
    }
#else
    have_opt(NULL);  // avoid 'defined but not used' warning
#endif

    /**
     * In case of non-dynamic mpi build mpi library is already linked. We add -mpi
     * argument in following scenario:
     * - if user has not explicitly set NEURON_INIT_MPI to 0 and mpi is already initialized
     * - if user has explicitly set NEURON_INIT_MPI to 1 to load mpi initialization
     */
    if (libnrnmpi_is_loaded) {
        nrnmpi_wrap_mpi_init(&flag);
        if (flag) {
            mpi_mes = 1;
            add_arg("-mpi", NULL);
        } else if (env_mpi != NULL && strcmp(env_mpi, "1") == 0) {
            mpi_mes = 2;
            add_arg("-mpi", NULL);
        } else {
            mpi_mes = 3;
        }
    } else {
        // no mpi
        mpi_mes = 3;
    }

    // merely avoids unused variable warning
    if (!pmes.empty() && mpi_mes == 2) {
        exit(1);
    }

#endif  // NRNMPI
    std::string buf{neuron::config::system_processor};
    buf += "/.libs/libnrnmech.so";
    // printf("buf = |%s|\n", buf);
    FILE* f;
    if ((f = fopen(buf.c_str(), "r")) != 0) {
        fclose(f);
        add_arg("-dll", buf.c_str());
    }
    nrn_is_python_extension = 1;
    nrn_nobanner_ = 1;
    const char* pyver = Py_GetVersion();
    nrn_is_python_extension = (pyver[0] - '0') * 10 + (pyver[2] - '0');
    if (isdigit(pyver[3])) {  // minor >= 10 e.g. 3.10  is 310
        nrn_is_python_extension = nrn_is_python_extension * 10 + pyver[3] - '0';
    }
    p_nrnpython_finalize = nrnpython_finalize;
#if NRNMPI
    if (libnrnmpi_is_loaded) {
        nrnmpi_init(1, &argc, &argv);  // may change argc and argv
    }
#if 0 && !defined(NRNMPI_DYNAMICLOAD)
	if (nrnmpi_myid == 0) {
		switch(mpi_mes) {
			case 0:
				break;
			case 1:
  printf("MPI_Initialized==true, MPI functionality enabled by Python.\n");
				break;
			case 2:
  printf("MPI functionality enabled by NEURON.\n");
				break;
			case 3:
  printf("MPI_Initialized==false, MPI functionality not enabled.\n");
				break;
		}
	}
#endif  // 0 && !defined(NRNMPI_DYNAMICLOAD)
#endif  // NRNMPI

    char* env_nframe = getenv("NEURON_NFRAME");
    if (env_nframe != NULL) {
        char* endptr;
        const int nframe_env_value = strtol(env_nframe, &endptr, 10);
        if (*endptr == '\0') {
            if (nframe_env_value > 0) {
                add_arg("-NFRAME", env_nframe);
            } else {
                PySys_WriteStdout("NEURON_NFRAME env value must be positive\n");
            }
        } else {
            PySys_WriteStdout("NEURON_NFRAME env value is invalid!\n");
        }
    }

    if (print_options) {
        PySys_WriteStdout("ivocmain options:");
        for (int i = 1; i < argc; ++i) {
            PySys_WriteStdout(" '%s'", argv[i]);
        }
        PySys_WriteStdout("\n");
    }

    nrn_main_launch = 2;
    ivocmain(argc, (const char**) argv, (const char**) env);
    return nrnpy_hoc();
}

#if !defined(MINGW)
extern "C" void modl_reg() {}
#endif  // !defined(MINGW)
