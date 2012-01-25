/*
 March 2001 modified by Michael Hines so that NEURON starts
	the Java VM
*/

// nj_load() supports the load_java hoc command

// nrn_InitializeJavaVM makes the Java virtual machine ready for use
// this is done from ivocmain.cpp shortly after NEURON is launched.
// Initializing just before the first load_java command did not work.
// Mac and MSWIN dynamically load the jvm. 

#include <../../nrnconf.h>
#ifdef WIN32
#include <windows.h>
#endif

#include "njconf.h" // which jvm version to use
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <InterViews/resource.h>
#include "nrnoc2iv.h"
#include "njvm.h"
#if HAVE_LOCALE_H
// Java may set the locale so sprint(buf, "%g", 1.5) gives "1,5"
// if so we will need to set it back.
#include <locale.h>
#endif
#if defined(JVM_DLOPEN)
#include <dlfcn.h>
jint nrn_CreateJavaVM(JavaVM **pvm, void **penv, void *args);
#endif

// Java virtual machine version
// The Mac only has Java 1 via the MRJ. No reason not to always use
// Java2 for mswin. Unix can use either (see njconf.h.in)
// However, only Java2 is fully supported and allows additions to
// the classpath from hoc.
#if carbon
#undef MAC
#endif

#ifndef USEJVM
#if MAC
#define USEJVM 1
#else
#define USEJVM 2
#endif
#endif

extern "C" {
// Java has threads and NEURON does not. It is important that when
// Java calls a function in NEURON and then NEURON calls back a function
// in Java that the proper thread environment is used. This is done
// with the njvm.h macros jnisave and jnirestore that put the env into nrnjava_env
// How are errors handled?
JNIEnv* nrnjava_env;
JNIEnv* nrnjava_root_env;
JavaVM* nrnjava_vm; // CreateJavaVM fills this in but we do not use it.
extern char* neuron_home;
extern int(*p_hoc_load_java)();
static int nj_load();
extern void* (*p_java2nrn_cons)(Object*);

#ifdef WIN32
#undef _WIN32
#define _WIN32
#endif
#ifdef _WIN32
char* hoc_back2forward(char*);
jint nrn_CreateJavaVM(JavaVM **pvm, void **penv, void *args);
#endif
#if MAC
static jint nrn_GetDefaultJavaVMInitArgs(void*);
static jint nrn_CreateJavaVM(JavaVM **pvm, JNIEnv **penv, void *args);
#endif
}
int convertJavaClassToHoc(JNIEnv*, const char*, const char*, const char*);
void nrnjava_init();

#ifdef _WIN32
#define PATH_SEPARATOR ';'
#else /* UNIX */
#define PATH_SEPARATOR ':'
#endif

#define NULL_CHECK(arg) assert((arg))
#if USEJVM == 2
/*
 * List of VM options to be specified when the VM is created.
 */
static JavaVMOption *options;
static int numOptions, maxOptions;
#endif

extern "C" { // needed by microsoft visual c++
// support load_java command in hoc.
// e.g. load_java("java.lang.String", "JString")
// p_hoc_load_java points to this
static int nj_load() {
	// first time through, initialize
	if (!p_java2nrn_cons) {
		if (!nrnjava_root_env) {
			hoc_execerror("The JavaVM is not available.", 0);
		}
		nrnjava_init();
		if (!p_java2nrn_cons) {
			hoc_execerror(" Java portion of NEURON was not initialized.", 0);
		}
	}
	char* jname; // fully qualified name. e.g. java.lang.String
	char* hname; // hoc name, e.g. JString
	char* path = ""; // classpath addition to find jname. See NrnClassLoader.add

	jname = gargstr(1);
	// hocname same as jname if not specified as second arg
	if (ifarg(2)) {
		hname = gargstr(2);
	}else{
		hname = jname;
	}
	if (ifarg(3)) {
		path = gargstr(3);
	}
	// see neuron/Neuron.java makeHocClass
	return convertJavaClassToHoc(nrnjava_env, jname, hname, path);
}
}

#if USEJVM == 2
// copied from /usr/j2se/src.jar : src/launcher/java.c
/*
 * Adds a new VM option with the given given name and value.
 */
static void
AddOption(char *str, void *info)
{
	int i;
    /*
     * Expand options array if needed to accomodate at least one more
     * VM option.
     */
    if (numOptions >= maxOptions) {
	if (options == 0) {
	    maxOptions = 4;
	    options = new JavaVMOption[maxOptions];
	} else {
	    JavaVMOption *tmp;
	    tmp = new JavaVMOption[maxOptions*2];
	    for (i=0; i < numOptions; ++i) {
		tmp[i].optionString = options[i].optionString;
		tmp[i].extraInfo = options[i].extraInfo;
	    }
	    maxOptions *= 2;
	    delete [] options;
	    options = tmp;
	}
    }
    options[numOptions].optionString = str;
    options[numOptions++].extraInfo = info;
}
#endif

// copied from /usr/j2se/src.jar : src/launcher/java.c
/*
 * Prints the version information from the java.version and other properties.
 */

static void
PrintJavaVersion(JNIEnv *env)
{
    jclass ver;
    jmethodID print;

    NULL_CHECK(ver = (env)->FindClass("sun/misc/Version"));
    NULL_CHECK(print = (env)->GetStaticMethodID(ver, "print", "()V"));

    (env)->CallStaticVoidMethod(ver, print);
}

static void myabort() {
	printf("my abort\n");
	exit(1);
}

static void myexit() {
	printf("my exit\n");
	exit(1);
}

jint myvfprintf(FILE* fp, const char* format, va_list args);

jint myvfprintf(FILE* fp, const char* format, va_list args) {
	char buf[1024];
	vsprintf(buf, format, args);
	printf("%s", buf);
	return 1;
}

#if USEJVM == 1
// allowing Mac classic dlopen and Unix static
static void initialize_jvm1();
static void initialize_jvm1() {
	char* classpath;
	JDK1_1InitArgs args;
	args.version = 0x00010001;
	//args.debugging = 1;
	//args.vfprintf = myvfprintf;
#if MAC
	if (nrn_GetDefaultJavaVMInitArgs(&args) < 0) {
		return;
	}
	classpath = new char[strlen(args.classpath) + 2*strlen(neuron_home) + 100];
	// following string for mac, then convert ; to :
	sprintf(classpath, "%s;/%s/classes/neuron.jar",
	 args.classpath, neuron_home);
	for (char* cp = classpath; *cp; ++cp) {
		if (*cp == ';') {
			*cp = ':';
		}else if (*cp == ':') {
			*cp = '/';
		}
	}
#else
	JNI_GetDefaultJavaMVInitArgs(&args);
	const char* ucpenv = getenv("CLASSPATH");
// Find classes first in the working directory where neuron was launched.
// Then the users CLASSPATH
// environment variable (if any). And lastly, the, $NEURONHOME/classes
	if (ucpenv == nil) {
		ucpenv = "."; // can't hurt to have it twice
	}
	int len = strlen(args.classpath) + strlen(ucpenv) + 2*strlen(neuron_home) + 100;
	classpath = new char[len];
		
	sprintf(classpath,
		"%s%c.%c%s%c%s/classes/neuron.jar",
		args.classpath, PATH_SEPARATOR, PATH_SEPARATOR,
		PATH_SEPARATOR, ucpenv, PATH_SEPARATOR,  neuron_home,
	);

//printf("%s\n", classpath);

	sprintf(classpath, "%s:.:%s/classes/neuron.jar", args.classpath, neuron_home);
#endif
	args.classpath = classpath;
	//for (int i = 0;  args.properties[i]; ++i) {
		//printf("properties |%s|\n", args.properties[i]);
	//}
	//args.debugging = 1;
	//args.vfprintf = myvfprintf;
	printf("classpath |%s|\n", args.classpath);
#if MAC
	jint res = nrn_CreateJavaVM(&nrnjava_vm, &nrnjava_root_env, (void*)&args);
#else
	jint res = JNI_CreateJavaVM(&nrnjava_vm, (void**)&nrnjava_root_env, &args);
#endif
	nrnjava_env = nrnjava_root_env;
	if (res < 0) {
		fprintf(stderr, "JNI_CreateJavaVM returned %d\n", res);
	}else{
		p_hoc_load_java = nj_load;
		fprintf(stderr, "Created Java VM\n");
	}
}
#endif

#if USEJVM == 2
// allowing mswin and unix dlopen and unix static
static void initialize_jvm2();
static void initialize_jvm2() {
	JavaVMInitArgs args;
// Because we want to dynamically append to the classpath from hoc
// we do all class loading through the neuron/NrnClassLoader in order
// to defeat the security policy.
	char* classpath;
	int len = strlen(neuron_home) + 100;
	classpath = new char[len];
	sprintf(classpath, "-Djava.class.path=%s/classes/nrnclsld.jar",	neuron_home);

#if defined(_WIN32)
	hoc_back2forward(classpath);
#endif

//printf("%s\n", classpath);

	args.version = JNI_VERSION_1_2;
//printf("version = %lx\n", args.version);
	
	AddOption(classpath, nil);
//	AddOption("-verbose", nil);
//	AddOption("abort", myabort);
//	AddOption("exit", myexit);
	args.nOptions = numOptions;
	args.options = options;
	args.ignoreUnrecognized = JNI_FALSE;

#if defined(_WIN32) || defined(JVM_DLOPEN)
	jint res = nrn_CreateJavaVM(&nrnjava_vm, (void**)&nrnjava_root_env, &args);
	if (res == -10) { return; }
#else
	jint res = JNI_CreateJavaVM(&nrnjava_vm, (void**)&nrnjava_root_env, &args);
#endif
	nrnjava_env = nrnjava_root_env;
	delete [] classpath;
	delete [] options;
	if (res < 0) {
		switch (res) {
		case JNI_EVERSION:
			fprintf(stderr, "JNI Version error. VM is not JNI_VERSION_1_2\n");
			break;
		case JNI_ENOMEM:
			fprintf(stderr, "Not enough memory\n");
			break;
		case JNI_EINVAL:
			fprintf(stderr, "invalid arguments\n");
			break;
		default:
			fprintf(stderr, "JNI_CreateJavaVM returned %d\n", res);
			break;
		}
		fprintf(stderr, "Info: optional feature Java VM is not present.\n");
	}else{
		p_hoc_load_java = nj_load;
		if (nrn_istty_) {
			fprintf(stderr, "Created Java VM\n");
			PrintJavaVersion(nrnjava_env);
		}
	}
}
#endif

void nrn_InitializeJavaVM() {
	if (nrnjava_root_env) { // hmm. NEURON must have been loaded by java
		nrnjava_env = nrnjava_root_env;
		p_hoc_load_java = nj_load;
	} else {
#if USEJVM == 2
		initialize_jvm2();
#else
		initialize_jvm1();
#endif
	}

#if HAVE_LOCALE_H
	// in case Java set the locale such that the radix is a ',', set it
	// back to a '.'
	char radixtest[50];
	sprintf(radixtest, "%g", 1.5);
//printf("radixtest=|%s|\n", radixtest);
	if (strchr(radixtest, ',')) {
		setlocale(LC_NUMERIC, "C");
//		sprintf(radixtest, "%g", 1.5);
//printf("after setlocale(LC_NUMERIC, \"C\"), radixtest=|%s|\n", radixtest);
	}
#endif
}

#if defined(JVM_DLOPEN)

#include <InterViews/session.h>
#include <OS/string.h>
#include <InterViews/style.h>
typedef jint(*PCJVM)(JavaVM**, void**, void*);

jint nrn_CreateJavaVM(JavaVM **pvm, void **penv, void *args) {
	jint res;

	*pvm = 0;
	*penv = 0;
	Session* ses = Session::instance();
	String str("libjvm.so");
	char* name = "jvmdll";
	if (ses && !ses->style()->find_attribute(name, str)){
//		fprintf(stderr, "\"%s\" not defined in nrn.defaults\n", name);
		return -10;
	}
	void* handle = (void *) dlopen(str.string(), RTLD_NOW | RTLD_GLOBAL);
	if (!handle) {
		fprintf(stderr, "dlopen(\"%s\") failed: %s\n", str.string(), dlerror());
		return -1;
	}
#if defined(DARWIN)
	PCJVM addr = (PCJVM)dlsym(handle, "JNI_CreateJavaVM_Impl");
#else
	PCJVM addr = (PCJVM)dlsym(handle, "JNI_CreateJavaVM");
#endif
	if (!addr) {
		fprintf(stderr, "%s\n", dlerror());
		return -1;
	}
	res = (*addr)(pvm, penv, args);
	return res;
}
#endif

#ifdef _WIN32
#if _MSC_VER
#undef bool
#endif
#if defined(__MWERKS__) && __MWERKS__ >= 7
#undef bool
#endif
#include <InterViews/session.h>
#include <OS/string.h>
#include <InterViews/style.h>
static int jerr_;
static void *
dlopen (const char *name, int)
{
  void *ret;

    {
      /* handle for the named library */
      String str;
	jerr_ = 0;
      if (!Session::instance()->style()->find_attribute(name, str)){
      	//fprintf(stderr, "\"%s\" not defined in nrn.def\n", name);
	jerr_ = -10;
        ret = NULL;
      }else{
          ret = (void *) LoadLibrary (str.string());
          if (ret == NULL) {
		DWORD dw = GetLastError();
            fprintf(stderr, "LoadLibrary(\"%s\") failed with error %d\n", str.string(), dw);
		jerr_ = -1;
	  }
      }
    }

  return ret;
}

static void *
dlsym (void *handle, const char *name)
{
  void *ret = (void *) GetProcAddress ((HMODULE) handle, name);
  if (!ret) {
  	fprintf(stderr, "Could not GetProcAddress for \"%s\"\n", name);
  }
  return ret;
}

#if defined(_MSC_VER)
typedef jint(CALLBACK *PCJVM)(JavaVM**, void**, void*);
#else
typedef jint(*PCJVM)(JavaVM**, void**, void*);
#endif

jint nrn_CreateJavaVM(JavaVM **pvm, void **penv, void *args) {
	jint res;

	*pvm = 0;
	*penv = 0;
	void* handle = dlopen("jvmdll", 0);
	if (!handle) { return jerr_; }
	PCJVM addr = (PCJVM)dlsym(handle, "JNI_CreateJavaVM");
	if (!addr) { return -1; }
	res = (*addr)(pvm, penv, args);
	return res;
}
#endif

#if MAC
extern "C" {
bool is_mac_dll(FSSpec*);
extern OSErr __path2fss(const char* name, FSSpec*);
}
typedef jint(*PCJVM)(JavaVM**, JNIEnv**, void*);
typedef jint(*PIJVM)(void*);
typedef CFragConnectionID(*PF)();
static PCJVM caddr;

static jint nrn_GetDefaultJavaVMInitArgs(void* args) {
	PIJVM iaddr = 0;
	long i, cnt;
	CFragConnectionID id;
	CFragSymbolClass sc;
	Ptr sa;
	Str255 sname;
	OSErr myErr;
	FSSpec fs;
	char name[256];
	
	sprintf(name, "%s:nrnjvmdll", neuron_home);
	
	if ((__path2fss(name, &fs) == fnfErr) || !is_mac_dll(&fs)) {
		fprintf(stderr, "%s is not the nrnjvmdll\n", name);
		return -1;
	}
	
	myErr = GetDiskFragment(&fs, 0, kCFragGoesToEOF,
		0, kLoadCFrag, &id, &sa, sname);
	//myErr = GetSharedLibrary("\pMRJLib", kPowerPCCFragArch, kLoadCFrag,
	//	&id, &sa, sname);
	if (myErr) {
		sname[sname[0]+1]='\0';
		fprintf(stderr, "could not load the Java VM : %s\n", sname+1);
		return -1;
	}
	sa = 0;
	myErr = CountSymbols(id, &cnt);
	//printf("%d symbols exported\n", cnt);
	for (i=0; i < cnt; ++i) {
		myErr = GetIndSymbol(id, i, sname, &sa, &sc);
		sname[sname[0]+1] = '\0';
		//printf("%d %s\n", i, sname+1);
		if (strcmp((char*)(sname+1), "nrn2_GetDefaultJavaVMInitArgs") == 0) {
			iaddr = (PIJVM)sa;
		}
		if (strcmp((char*)(sname+1), "nrn2_CreateJavaVM") == 0) {
			caddr = (PCJVM)sa;
		}
	}
	if (iaddr) {
		jint res = (*iaddr)(args);
		if (res < 0) {
			fprintf(stderr, "call to JNI_GetDefaultJavaVMInitArgs returned %d\n", res);
		}
		return res;
	}
	fprintf(stderr, "no address for JNI_GetDefaultJavaVMInitArgs\n");
	return -1;
}

static jint nrn_CreateJavaVM(JavaVM **pvm, JNIEnv **penv, void *args) {
	jint res;
	*pvm = 0;
	*penv = 0;
	if (!caddr) {
		fprintf(stderr, "no address for JNI_CreateJavaVM\n");
		return -1;
	}
	res = (*caddr)(pvm, penv, args);
	return res;
}
#endif
