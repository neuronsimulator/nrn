#include <../../nrnconf.h>
#include <../nrnpython/nrnpython_config.h>

extern "C" {
long hoc_nframe, hoc_nstack;
}

#if !HAVE_IV
#define Session void
extern "C" {
	int hoc_main1(int, const char**, const char**);
	void hoc_main1_init(const char*, const char**);
}
#endif

#include <stdio.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#if !defined (__APPLE__)
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

#include <ivstream.h>
#include <assert.h>
#include "ivoc.h"
#include "idraw.h"
#include <InterViews/style.h>
#endif
#include <OS/string.h>
#include "string.h"
#include "oc2iv.h"
#include "nrnmpi.h"
#include "nrnrt.h"

#if defined(carbon)
#undef MAC
#endif

#if MAC || defined(WIN32)
#include "njconf.h"
#else
#include "../nrnjava/njconf.h"
#endif

#if 1
extern "C" { void pr_profile(); }
#define PR_PROFILE pr_profile();
#else
#define PR_PROFILE /**/
#endif
/*****************************************************************************/

#if HAVE_IV
static PropertyData properties[] = {
{"*gui", "sgimotif"},
{"*PopupWindow*overlay", "true"},
{"*PopupWindow*saveUnder", "on"},
{"*TransientWindow*saveUnder", "on"},
{"*background", "#ffffff"},
{"*brush_width", "0"},
{"*double_buffered", "on"},
{"*flat", "#aaaaaa"},
#if defined(WIN32)|| defined(CYGWIN)
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
{"*NFRAME", "0"}, // see src/oc/code.c for the default value
{"*NSTACK", "0"}, // see src/oc/code.c for the default value
{"*Py_NoSiteFlag", "0"}, 
{"*python", "off"},
{"*nopython", "off"},
{"*banner", "on"},
	 { NULL }
};

static OptionDesc options[] = {
{"-dismissbutton", "*dismiss_button", OptionValueImplicit, "Close"},
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
{"-Py_NoSiteFlag", "*Py_NoSiteFlag", OptionValueImplicit, "1"},
{"-nobanner", "*banner", OptionValueImplicit, "off"},
#if defined(WIN32)
{"-mswin_scale", "*mswin_scale", OptionValueNext},
#endif
	 { NULL }
};
#endif // HAVE_IV

extern "C" {
	extern int hoc_obj_run(const char*, Object*);
	extern int nrn_istty_;
	extern char* nrn_version(int);
	extern int nrn_nobanner_;
	extern void hoc_final_exit();
	void ivoc_final_exit();
#if (defined(NRNMECH_DLL_STYLE) || defined(WIN32))
	extern const char* nrn_mech_dll;
#endif
#if defined(USE_PYTHON)
	int nrn_nopython;
	extern int use_python_interpreter;
	extern void (*p_nrnpython_start)(int);
#endif
}

/*****************************************************************************/
//exported initialized data so shared libraries can have assert pure-text
#if HAVE_IV
int Oc::refcnt_ = 0;
Session* Oc::session_ = 0;
HandleStdin* Oc::handleStdin_ = 0;
ostream* OcIdraw::idraw_stream = 0;
#endif
/*****************************************************************************/
extern void ivoc_cleanup();
#if OCSMALL
static char* ocsmall_argv[] = {0, "difus.hoc"};
#endif
#if defined(WIN32) && HAVE_IV
extern "C" {
extern HWND hCurrWnd;
}
#endif


extern "C" {
	extern void setneuronhome(const char*);
	extern const char* neuron_home;
	int hoc_xopen1(const char* filename, const char* rcs);
	extern int units_on_flag_;
	extern double hoc_default_dll_loaded_;
	extern int hoc_print_first_instance;
	int nrnpy_nositeflag;
}

#if !defined(WIN32) && !MAC && !defined(CYGWIN)
void setneuronhome(const char*) {
	neuron_home = getenv("NEURONHOME");
}
#endif

#if 0
void penv() {
	int i;
	for (i=0; environ[i]; ++i) {
		printf("%p %s\n", environ[i], environ[i]);
	}
}
#endif

#if MAC
#include <string.h>
#include <sioux.h>
extern bool mac_load_dll(const char*);
extern "C" {
void mac_open_doc(const char* s) {
	// only chdir and load dll on the first opendoc
	static bool done = false;
	char cs[256];
	strncpy(cs, s, 256);
	char* cp  = strrchr(cs, ':');
	if (cp && !done) {
		*cp = '\0';
		 if (chdir(cs) == 0) {
		 	done = true;
			printf("current directory is \"%s\"\n", cs);
			if (mac_load_dll("nrnmac.dll")) {
				hoc_default_dll_loaded_ = 1.;
			}
		}
	}		
	hoc_xopen1(s, 0);
}
void mac_open_app(){
	hoc_xopen1(":lib:hoc:macload.hoc", 0);
}
}
#endif

#ifdef MAC
#pragma export on
#endif

extern "C" {
	int ivocmain(int, const char**, const char**);
	int (*p_neosim_main)(int, const char**, const char**);
	extern int nrn_global_argc;
	extern const char** nrn_global_argv;
	int always_false;
	int nrn_is_python_extension;
}

// some things are defined in libraries earlier than they are used so...
#include <nrnisaac.h>
static void force_load() {
	if (always_false) {
		nrnisaac_new();
	}
}

#if defined(CYGWIN)
// see iv/src/OS/directory.cpp
#include <sys/stat.h>
static bool isdir(const char* p) {
	struct stat st;
	bool b =  stat((char*)p, &st) == 0 && S_ISDIR(st.st_mode);
	//printf("isdir %s returns %d\n", p, b);
	return b;
}
#endif

#ifdef MAC
#pragma export off
#endif

// in case we are running without IV then get some important args this way
static bool nrn_optarg_on(const char* opt, int* argc, char** argv);
static char* nrn_optarg(const char* opt, int* argc, char** argv);
static int nrn_optargint(const char* opt, int* argc, char** argv, int dflt);

static bool nrn_optarg_on(const char* opt, int* pargc, const char** argv) {
	char* a;
	int i;
	for (i=0; i < *pargc; ++i) {
		if (strcmp(opt, argv[i]) == 0) {
			*pargc -= 1;
			for (; i < *pargc; ++i) {
				argv[i] = argv[i+1];
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
	for (i=0; i < *pargc - 1; ++i) {
		if (strcmp(opt, argv[i]) == 0) {
			a = argv[i+1];
			*pargc -= 2;
			for (; i < *pargc; ++i) {
				argv[i] = argv[i+2];
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

#if 0 //for debugging
void prargs(const char* s, int argc, const char** argv) {
	int i;
	printf("%s argc=%d\n", s, argc);
	for (i=0; i < argc; ++i) {
		printf(" %d |%s|\n", i, argv[i]);
	}
}
#endif

// see nrnmain.cpp for the real main()

int ivocmain (int argc, const char** argv, const char** env) {
// third arg should not be used as it might become invalid
// after putenv or setenv. Instead, if necessary use
// #include <unistd.h>
// extern char** environ;
	int i;
//	prargs("at beginning", argc, argv);
	force_load();
	nrn_global_argc = argc;
	nrn_global_argv = new const char*[argc];
	for (i = 0; i < argc; ++i) {
		nrn_global_argv[i] = argv[i];
	}
	if (nrn_optarg_on("-help", &argc, argv)
	    || nrn_optarg_on("-h", &argc, argv)) {
		printf("nrniv [options] [fileargs]\n\
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
	for (i=0; i < argc; ++i) {
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
	  printf("Warning: detected user attempt to enable MPI, but MPI support was disabled at build time.\n");
	}

#endif 		

#if NRN_MUSIC
	nrn_optarg_on("-music", &argc, argv);
#else
	if (nrn_optarg_on("-music", &argc, argv)) {
	  printf("Warning: attempt to enable MUSIC but MUSIC support was disabled at build time.\n");
	}
#endif

#if NRN_REALTIME
	if (nrn_optarg_on("-realtime", &argc, argv)) {
		nrn_realtime_ = 1;
		nrn_setscheduler();
	}
	if (nrn_optarg_on("-schedfifo", &argc, argv)) {
		if (nrn_realtime_ != 1) {
			nrn_setscheduler();
		}
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
#if !defined(WIN32)&&!defined(MAC) && !defined(CYGWIN)
// Gary Holt's first pass at this was:
//
// Set the NEURONHOME environment variable.  This should override any setting
// in the environment, so someone doesn't accidently use data files from an
// old version of neuron.
//
// But I have decided to use the environment variable if it exists
	neuron_home = getenv("NEURONHOME");
	if (!neuron_home) {
#if defined(HAVE_PUTENV)
		// the only reason the following is static is to prevent valgrind
		// from complaining it is a memory leak.
		static char* buffer = new char[strlen(NEURON_DATA_DIR) + 12];
		sprintf(buffer, "NEURONHOME=%s", NEURON_DATA_DIR);
		putenv(buffer);
		neuron_home = NEURON_DATA_DIR;
#elif defined(HAVE_SETENV)
		setenv("NEURONHOME", NEURON_DATA_DIR, 1);
		neuron_home = NEURON_DATA_DIR;
#else
#error "I don't know how to set environment variables."
// Maybe in this case the user will have to set it by hand.
#endif
		// putenv and setenv may invalidate env but we no longer
		// use it so following should not be needed
#if 0
#if HAVE_UNISTD_H && !defined(__APPLE__)
	env = environ;
#endif
#if defined (__APPLE__)
	env = (*_NSGetEnviron());
#endif
#endif
	}

#else // Not unix:
	neuron_home = getenv("NEURONHOME");
	if (!neuron_home) {
		setneuronhome((argc > 0)?argv[0]:0);
	}
	if (!neuron_home) {
#if defined(WIN32) && HAVE_IV
		MessageBox(0, "No NEURONHOME environment variable.", "NEURON Incomplete Installation", MB_OK);
#else
		neuron_home = ".";
		fprintf(stderr, "Warning: no NEURONHOME environment variable-- setting\
 to %s\n", neuron_home);
#endif
	}
#endif // !unix.
    
#if HAVE_IV
#if OCSMALL
	our_argc = 2;
	our_argv = new char*[2];
	our_argv[0] = "Neuron";
	our_argv[1] = ":lib:hoc:macload.hoc";
	session = new Session("NEURON", our_argc, our_argv, options, properties);
#else
#if MAC
	our_argc = 1;
	our_argv = new char*[1];
	our_argv[0] = "Neuron";
	session = new Session("NEURON", our_argc, our_argv, options, properties);
	SIOUXSettings.asktosaveonclose = false;
#else
#if defined(WIN32) || carbon
IFGUI
	session = new Session("NEURON", our_argc, (char**)our_argv, options, properties);
ENDGUI
#else
IFGUI
	if (getenv("DISPLAY")) {
		session = new Session("NEURON", our_argc, (char**)our_argv, options, properties);
	}else{
		fprintf(stderr, "Warning: no DISPLAY environment variable.\
\n--No graphics will be displayed.\n");
		hoc_usegui = 0;
	}
ENDGUI
#endif
#endif
	char nrn_props[256];
	if (session) {
		sprintf(nrn_props, "%s/%s", neuron_home, "lib/nrn.defaults");
#ifdef WIN32
		FILE* f;
		if ((f = fopen(nrn_props, "r")) != (FILE*)0) {
			fclose(f);
			session->style()->load_file(String(nrn_props), -5);
		}else{
#if defined(CYGWIN)
			sprintf(nrn_props, "%s/%s", neuron_home, "lib/nrn.def");
#else
			sprintf(nrn_props, "%s\\%s", neuron_home, "lib\\nrn.def");
#endif
			if ((f = fopen(nrn_props, "r")) != (FILE*)0) {
				fclose(f);
				session->style()->load_file(String(nrn_props), -5);
			}else{
				char buf[256];
				sprintf(buf, "Can't load NEURON resources from %s[aults]",
					nrn_props);
				printf("%s\n", buf);
			}
		}
#else
		 session->style()->load_file(String(nrn_props), -5);
#endif
#if ! MAC
		char* h = getenv("HOME");
		if (h) {
		    	sprintf(nrn_props, "%s/%s", h, ".nrn.defaults");
		    	session->style()->load_file(String(nrn_props), -5);
		}
#endif
	}

#endif /*OCSMALL*/

	if (session) {
		session->style()->find_attribute("NSTACK", hoc_nstack);
		session->style()->find_attribute("NFRAME", hoc_nframe);
	}else
#endif //HAVE_IV
	{
		hoc_nstack = nrn_optargint("-NSTACK", &our_argc, our_argv, 0);
		hoc_nframe = nrn_optargint("-NFRAME", &our_argc, our_argv, 0);
	}

#if defined(USE_PYTHON)
#if HAVE_IV
	nrn_nopython = 0;
	if (session && session->style()->value_is_on("nopython")) {
		nrn_nopython = 1;
	}
#endif
//	if (nrn_optarg_on("-nopython", &our_argc, our_argv)) {
//		nrn_nopython = 1;
//	}

#endif //USE_PYTHON

#if defined(WIN32) && HAVE_IV
IFGUI
	double scale = 1.;
	int pw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	if (pw < 1100) {
		scale = 1200./double(pw);
	}
	session->style()->find_attribute("mswin_scale", scale); 
	iv_display_scale(float(scale));
ENDGUI
#endif

	// just eliminate from arg list
	nrn_optarg_on("-mpi", &our_argc, our_argv);

#if (defined(NRNMECH_DLL_STYLE) || defined(WIN32))
	String str;
#if HAVE_IV
	if (session) {
		if (session->style()->find_attribute("nrnmechdll", str)) {
			nrn_mech_dll = str.string();
		}
	}else
#endif
	{ // if running without IV.
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
		// Actually this is done by default in src/nrnoc/init.c
	}
#endif

#endif //NRNMECH_DLL_STYLE


#if HAVE_IV
	if (session) {
		long i;
		if (session->style()->find_attribute("isatty", i)) {
			nrn_istty_ = i;
		}		
	}else
#endif
	{
		nrn_istty_ = nrn_optarg_on("-isatty", &our_argc, our_argv);
		if (nrn_istty_ == 0) {
			nrn_istty_ = nrn_optarg_on("-notatty", &our_argc, our_argv);
			if (nrn_istty_ == 1) { nrn_istty_ = -1; }
		}
	}

#if HAVE_IV
	if (session && session->style()->value_is_on("units_on_flag")) {
		units_on_flag_ = 1;
	};
	Oc oc(session, our_argv[0], env);
#if defined(WIN32) && !defined(CYGWIN)
	if (session->style()->find_attribute("showwinio", str)
      && !session->style()->value_is_on("showwinio")
	) {
		ShowWindow(hCurrWnd, SW_HIDE);
		hoc_obj_run("pwman_place(100,100)\n", 0);
	}
#endif
#else
	hoc_main1_init(our_argv[0], env);
#endif //HAVE_IV

#if USENRNJAVA
	nrn_InitializeJavaVM();	
#endif
#if OCSMALL
	if (argc == 1) {
		ocsmall_argv[0] = our_argv[0];
		oc.run(2, ocsmall_argv);
	}else
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

	if (nrn_is_python_extension) { return 0; }
#if defined(CYGWIN) && defined(HAVE_SETENV)
	if (!isdir("/usr/lib/python2.5")) {
		char* buf = new char[strlen(neuron_home) + 20];
		sprintf(buf, "%s/lib/%s", neuron_home, "python2.5");
		if (isdir(buf)) {
			setenv("PYTHONHOME", neuron_home, 0);
		}
		delete [] buf;

	}
#endif
	//printf("p_nrnpython_start = %p\n", p_nrnpython_start);
	if (p_nrnpython_start) { (*p_nrnpython_start)(1); }
	if (use_python_interpreter && !p_nrnpython_start) {
		fprintf(stderr, "Python not available\n");
		exit(1);
	}
#endif
#if NRN_REALTIME
	nrn_maintask_init();
#endif
#if HAVE_IV
	oc.run(our_argc, our_argv);
#else
	hoc_main1(our_argc, our_argv, env);
#endif
#if HAVE_IV
	if (session && session->style()->value_is_on("neosim")) {
		if (p_neosim_main) {
			(*p_neosim_main)(argc, argv, env);
		}else{
			printf("neosim not available.\nModify nrn/src/nrniv/Imakefile and remove nrniv/$CPU/netcvode.o\n");
		}
	}
#endif
PR_PROFILE
#if defined(USE_PYTHON)
	if (use_python_interpreter) {
		// process the .py files and an interactive interpreter
		if (p_nrnpython_start) {(*p_nrnpython_start)(2);}
	}
	if (p_nrnpython_start) { (*p_nrnpython_start)(0); }
#endif
	hoc_final_exit();
	ivoc_final_exit();
	return exit_status;
}

void ivoc_final_exit() {
#if NRNMPI
	nrnmpi_terminate();
#endif
#if NRN_REALTIME
	nrn_maintask_delete();
#endif
}

extern "C" {

extern void hoc_ret(), hoc_pushx(double);
extern double *getarg(int i);
extern int ifarg(int);

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
int run_til_stdin() {return 1;}
void hoc_notify_value(){}
#endif
}
