/** Neuron/Java Interface code
 *
 * @author Fred Howell
 * @date March 2001
 *
 * Modified by Michael Hines
 * The Java to Neuron JNIEXPORT functions for Neuron.java
 * have been collected in nrnjni.cpp. This file now mostly supports the
 * Neuron to Java direction
 */
 
#include <../../nrnconf.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <jni.h>
#include <time.h>
#include <OS/list.h>
#include "ivoc.h"
#include "classreg.h"

#include "nrnoc2iv.h"
#include "parse.h"

#include "njvm.h"
 
extern "C" {
extern Symlist* hoc_top_level_symlist;

// Hooks in nrn-5.0.0/src/oc/hoc_oop.c for filling in cTemplate structure
extern void* (*p_java2nrn_cons)(Object*); /* returns pointer to java object */
extern void  (*p_java2nrn_destruct)(void* opaque_java_object);
extern double (*p_java2nrn_dmeth)(Object* ho, Symbol* method);
extern char** (*p_java2nrn_smeth)(Object* ho, Symbol* method);
extern Object** (*p_java2nrn_ometh)(Object* ho, Symbol* method);
extern const char* (*p_java2nrn_classname)(Object* ho);
extern bool (*p_java2nrn_identity)(Object* o1, Object* o2);
}
// and others
extern void (*nrnjava_pwm_setwin)(void*, int, int, int);

// actual functions in this file that fill in the above hooks
static void* java2nrn_cons(Object* o);
static void java2nrn_destruct(void* opaque_java_object);
static double java2nrn_dmeth(Object* ho, Symbol* method);
static char** java2nrn_smeth(Object* ho, Symbol* method);
static Object** java2nrn_ometh(Object* ho, Symbol* method);
static const char* java2nrn_classname(Object* ho);
static bool java2nrn_identity(Object* o1, Object* o2);
static void pwm_setwin(void*, int, int, int);
static char** js2charpool(jstring js);

// see nrnjni.cpp
extern void nrnjni_registration(jclass nrnCls);
extern void nrnjni_redirreg(jclass nrnCls);

// Java Method & class IDs
static jclass nrnclsldr;
static jclass neuronCls;
static jmethodID nrnclsldID;
static jmethodID constructNoArgID;
static jmethodID constructWithArgID;
static jmethodID makeHocClassID;
static jmethodID invokeDoubleMethodID;
static jmethodID invokeCharsMethodID;
static jmethodID invokeObjectMethodID;
static jmethodID encapsulateID;
static jmethodID getObjectTypeID;
static jmethodID setwinID;
static jmethodID identityID;
static jmethodID jclassnameID;
static jfieldID  hocObjectCastID; 

declarePtrList(NJSymList, Symbol)
implementPtrList(NJSymList, Symbol)
// list of cTemplate of java registered classes in id order.
// this parallels the classList in Neuron.Java
NJSymList* nrn_jclass_symlist;

declarePtrList(NJStrList, String)
implementPtrList(NJStrList, String)
// list of the full java classnames for use in session save
static NJStrList* njclassnames;

Symbol* nrn_jobj_sym; // for a JavaObject
Symbol* nrn_vec_sym; // for deciding if Vector

static void* joconstruct(Object*) {
	hoc_execerror("JavaObject for internal use only.", "Do not construct");
	return nil;
}
static void jodestruct(void* v) {
	if (v) {
		nrnjava_env->DeleteGlobalRef((jobject)v);
	}
}

static double joequals(void* v) {
	jobject jo1 = (jobject)v;
	Object* o2 = *hoc_objgetarg(1);
	jobject jo2;
	if (o2 == nil) {return 0.;}
	if (o2->ctemplate->constructor == joconstruct // a JavaObject
	 || o2->ctemplate->sym->type == JAVAOBJECT) { // registerd java Object
		jobject jo2 = (jobject)o2->u.this_pointer;
		return (nrnjava_env->CallStaticIntMethod(neuronCls, identityID,
			jo1,jo2 )  != 0 ) ? 1. : 0.;

	}
	return 0.;
}

static const char** joname(void* v) {
	jstring js = (jstring)nrnjava_env->CallStaticObjectMethod(
		neuronCls, jclassnameID, (jobject)v);
	return (const char**)js2charpool(js);	
}

static Member_func jo_members[] = {
	"equals", joequals,
	0,0
};
static Member_ret_str_func jo_retstr_members[] = {
	"name", joname,
	0,0
};

// called from njvm.cpp when the first NrnJava hoc object is consstructed
void nrnjava_init () {	
//	printf("nrnjava_init\n"); 
	
	nrn_jclass_symlist = new NJSymList(20);
	class2oc("JavaObject", joconstruct, jodestruct, jo_members,
		nil, nil, jo_retstr_members);
	nrn_jobj_sym = hoc_lookup("JavaObject");
	nrn_vec_sym = hoc_lookup("Vector");

#if defined(_MSWIN) || (defined(MAC) && !defined(DARWIN))
	neuronCls = nrnjava_env->FindClass("neuron/Redirect");
	if (neuronCls == 0) {
		printf("ERROR in nrnjava_init : neuron/Redirect class not loaded\n");
printf("Look in the Redirect.out file of the current working directory for the reason for failure\n");
		return;
	}
	nrnjni_redirreg(neuronCls);
#endif

	nrnclsldr = nrnjava_env->FindClass("neuron/NrnClassLoader");
	if (nrnclsldr == 0) {
		printf("ERROR in nrnjava_init : neuron/NrnClassLoader not found\n");
	}
	nrnclsldID = nrnjava_env->GetStaticMethodID(nrnclsldr,
		"load", "(Ljava/lang/String;)Ljava/lang/Class;" );

	jstring js;
	js = nrnjava_env->NewStringUTF("neuron.Neuron");
	neuronCls  = (jclass)nrnjava_env->CallStaticObjectMethod(nrnclsldr, nrnclsldID,js);
	if (neuronCls == 0) {
		printf("ERROR in nrnjava_init : neuron.Neuron class not found\n");
		return;
	}
	js = nrnjava_env->NewStringUTF("neuron.HocObject");
	jclass hoCls = (jclass)nrnjava_env->CallStaticObjectMethod(nrnclsldr, nrnclsldID,js);
	if (hoCls == 0) {
		printf("ERROR in nrnjava_init : neuron.HocObject class not found\n");
		return;
	}

	p_java2nrn_cons = java2nrn_cons;
	p_java2nrn_destruct = java2nrn_destruct;
	p_java2nrn_dmeth = java2nrn_dmeth;
	p_java2nrn_smeth = java2nrn_smeth;
	p_java2nrn_ometh = java2nrn_ometh;
	p_java2nrn_classname = java2nrn_classname;
	p_java2nrn_identity = java2nrn_identity;

	nrnjava_pwm_setwin = pwm_setwin;

	nrnjni_registration(neuronCls);
	
	/*** Lookup method ids of Java Neuron class ***/
	constructNoArgID = nrnjava_env->GetStaticMethodID( neuronCls, 
		"constructNoArg", 
		"(I)Ljava/lang/Object;" );
	constructWithArgID = nrnjava_env->GetStaticMethodID( neuronCls, 
		"constructWithArg", 
		"(I)Ljava/lang/Object;" );
	makeHocClassID = nrnjava_env->GetStaticMethodID( neuronCls,
		"makeHocClass", 
		"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I" );
	invokeDoubleMethodID = nrnjava_env->GetStaticMethodID( neuronCls,
		"invokeDoubleMethod", 
		"(Ljava/lang/Object;III)D" );
	invokeCharsMethodID = nrnjava_env->GetStaticMethodID( neuronCls,
		"invokeCharsMethod", 
		"(Ljava/lang/Object;III)Ljava/lang/String;" );
	invokeObjectMethodID = nrnjava_env->GetStaticMethodID( neuronCls,
		"invokeObjectMethod", 
		"(Ljava/lang/Object;III)Ljava/lang/Object;" );
	encapsulateID = nrnjava_env->GetStaticMethodID( neuronCls,
		"encapsulateHocObject", 
		"(JI)Ljava/lang/Object;" );
	getObjectTypeID = nrnjava_env->GetStaticMethodID( neuronCls,
		"getObjectType", 
		"(Ljava/lang/Object;)I" );
	setwinID = nrnjava_env->GetStaticMethodID( neuronCls,
		"setwin",
		"(JIII)I");
	identityID = nrnjava_env->GetStaticMethodID( neuronCls,
		"identity",
		"(Ljava/lang/Object;Ljava/lang/Object;)I" );
	jclassnameID = nrnjava_env->GetStaticMethodID( neuronCls,
		"javaClassName",
		"(Ljava/lang/Object;)Ljava/lang/String;" );
	hocObjectCastID = nrnjava_env->GetFieldID( hoCls,
		"hocObjectCast", "J");

//	printf("neuronCls=%ld mids %ld %ld %ld\n", 
//	neuronCls, constructNoArgID,
//	makeHocClassID, invokeDoubleMethodID);

}

// don't forget to delete [] after use
char* nrn_underbar2dot(const char* s) {
	char *mangledstr = new char[strlen(s) + 1];
	strcpy(mangledstr, s);
	for (char* cp = mangledstr; *cp; ++cp) {
		if (*cp == '_') {
			*cp = '.';
		}
	}
	return mangledstr;
}

//don't forget to delete []
char* nrn_dot2underbar(const char* s) {
	char *mangledstr = new char[strlen(s) + 1];
	strcpy(mangledstr, s);
	for (char* cp = mangledstr; *cp; ++cp) {
		if (*cp == '.') {
			*cp = '_';
		}
	}
	return mangledstr;
}

/** equality in the sense of ==
*/
static bool java2nrn_identity(Object* o1, Object* o2) {
	if (o1 == o2) return true;
	if (o1 == 0 || o2 == 0) return false;
	return (nrnjava_env->CallStaticIntMethod(neuronCls, identityID,
		(jobject)o1->u.this_pointer, (jobject)o2->u.this_pointer
		)  != 0 ) ? true : false;
}

/** Construct a new instance of a java class  
 * @param o Empty hoc object which will contain this object
 */
static void* java2nrn_cons(Object* o) { 
//  printf("java2nrn_cons %s\n",o->ctemplate->sym->name );

	jint cid = -(o->ctemplate->id) - 1;

	jobject newo;
	if (!ifarg(1)) {
		newo = nrnjava_env->CallStaticObjectMethod( neuronCls,
			constructNoArgID, cid );
	}else{
		newo = nrnjava_env->CallStaticObjectMethod( neuronCls,
			constructWithArgID, cid);
	}
	jobject ret = 0;
	if (newo) { // otherwise it is pure static like java.lang.Math
//  printf("java2nrn_cons got jobject %ld\n", newo );
		ret = nrnjava_env->NewGlobalRef( newo );
//  printf("java2nrn_cons got global jobject %ld\n", ret );
	}
	return (void*)ret;
}

/** Destroy java reference
 */
static void java2nrn_destruct(void* opaque_java_object) { 
	//  printf("java2nrn_destruct\n");
	if (opaque_java_object) {
		nrnjava_env->DeleteGlobalRef( (jobject) opaque_java_object );
	}
}

/** Make a hoc equivalent to Java class (jname) calling it hname in hoc.
	Done from NrnJava.load("name")
	after the class is loaded with FindClass.
 */
int convertJavaClassToHoc( JNIEnv *env, const char* jname, const char* hname, const char* path ) {
	//only if classname not already in use.
	if (!njclassnames) {
		njclassnames = new NJStrList();
	}
	char* hn = nrn_dot2underbar(hname);
//	printf("loading %s --- calling it %s\n", jname, hn);
	Symbol* s = hoc_table_lookup(hn, hoc_top_level_symlist);
	if (s) {
		delete [] hn;
		if (s->type == TEMPLATE && s->u.ctemplate->id < 0) {
			return 2;
		}
		return 0;
	}
	jstring js = env->NewStringUTF( jname );
if (env->ExceptionOccurred()) { env->ExceptionDescribe();}
	jstring hs = env->NewStringUTF( hn );
	jstring jp = env->NewStringUTF(path);

	int i = env->CallStaticIntMethod(neuronCls, makeHocClassID, js, hs, jp);
	if (i == 1) {
		njclassnames->append(new CopyString(jname));
	}
	errno = 0; // have seen this set to 2 by linux
	delete [] hn;
	return i;
}


static const char* java2nrn_classname(Object* ho) {
	jint cid = -(ho->ctemplate->id) - 1;
	return njclassnames->item(cid)->string();
}

static void overloaded(Object* ho, Symbol* method) {
	int n = strlen(method->name);
	printf("%s.%s Overloaded. Use one of:\n", hoc_object_name(ho), method->name);
	for (Symbol* s = ho->ctemplate->symtable->first; s; s = s->next) {
		if (s != method && strncmp(s->name, method->name, n) == 0
			&& isdigit(s->name[n])
		) {
			printf("	%s\n", s->name);
		}
	}
	hoc_execerror(method->name, "Overloaded.  Disambiguate using a more specific method.");
}

/** invoke java method returning double
 */
static double java2nrn_dmeth(Object* ho, Symbol* method) {
//  printf("java2nrn_dmeth invoking %s.%s\n", hoc_object_name(ho), method->name);
	if (method->s_varn) {overloaded(ho, method);}
	double d = (double) nrnjava_env->CallStaticDoubleMethod(
		neuronCls, invokeDoubleMethodID,
		(jobject)ho->u.this_pointer,
		-(jint)ho->ctemplate->id - 1,
		(jint)method->u.u_auto, (jint)method->s_varn);
	errno = 0;
	if (d == -1e98) {
		hoc_execerror("Java Exception for", method->name);
	}
	return d;
}

static char** js2charpool(jstring js) {
#define imax 5
	static char** cs;
	static int i;
	// allow up to 5 calls before freeing
	// the problem is that several of these can be put on the
	// hoc stack before an early one is copied into strdef
	// and until then the early one must stay in existence
	if (!cs) {
		cs = new char*[imax];
		for (i=0; i < imax; ++i) {
			cs[i] = nil;
		}
		i = 0;	
	}
	const char* jc = nrnjava_env->GetStringUTFChars(js, 0);
	if (cs[i]) {
		delete [] cs[i];
		cs[i] = nil;
	}
	i = (i+1)%imax;
	cs[i] = new char[strlen(jc) + 1];
	strcpy(cs[i], jc);
	nrnjava_env->ReleaseStringUTFChars(js, jc);
	return cs + i;
}

/** invoke java method returning String
 */
static char** java2nrn_smeth(Object* ho, Symbol* method) {
//  printf("java2nrn_smeth invoking %s.%s\n", hoc_object_name(ho), method->name);
	if (method->s_varn) {overloaded(ho, method);}
	jstring js = (jstring)nrnjava_env->CallStaticObjectMethod(
		neuronCls, invokeCharsMethodID,
		(jobject)ho->u.this_pointer,
		-(jint)ho->ctemplate->id - 1,
		(jint)method->u.u_auto, (jint)method->s_varn);
	errno = 0;
	if (js == nil) {
		hoc_execerror("Java Exception for", method->name);
	}
	return js2charpool(js);
}

Object** nj_j2hObject(jobject jo, int type) {
	Object** po = nil;
	if (jo == 0) { //null
		po = hoc_temp_objptr(0);
		return po;
	}
	if (type >= 0) {
		void* v = nrnjava_env->NewGlobalRef(jo);
		Symbol* tsym = nrn_jclass_symlist->item(type);
		po = hoc_temp_objvar(tsym, v);
		return po;
	}else{
		void* v;
		Object* o;
		switch (type) {
		case -1: //Unregistered java object
			// put it in a JavaObject
			v = nrnjava_env->NewGlobalRef(jo);
			po = hoc_temp_objvar(nrn_jobj_sym, v);
			return po;
		case -2: //encapsulated hoc object
		case -3: //encapsulated Vector object
			o = (Object*)nrnjava_env->GetLongField(jo, hocObjectCastID);
			po = hoc_temp_objptr(o);
			return po;
		default:
			break;
		}
	}
	return 0;
}

/** invoke java method returning a Java Object
 */
static Object** java2nrn_ometh(Object* ho, Symbol* method) {
//  printf("java2nrn_ometh invoking %s.%s\n", hoc_object_name(ho), method->name);
	// I don't know how to return two things at once so we call two
	// methods. The first returns a java object and the second tells
	// the type.
	//type 0+ is a registered java object and is the template id.
	//type -1 is an unregistered java object -- enclose in JavaObject
	//type -2 is a HocObject and the jobject has a field called
	// long hocObjectCast which we can cast to the correct object.
	// other types are for Java objects that extend HocObject
	// but all have a field called "long hocObjectCast"
	Object** po = nil;
	if (method->s_varn) {overloaded(ho, method);}

	jobject jo = nrnjava_env->CallStaticObjectMethod(
		neuronCls, invokeObjectMethodID,
		(jobject)ho->u.this_pointer,
		-(jint)ho->ctemplate->id - 1,
		(jint)method->u.u_auto, (jint)method->s_varn);
	if (nrnjava_env->ExceptionOccurred()) {
		nrnjava_env->ExceptionDescribe();
		hoc_execerror("Java Exception for", method->name);
	}
	jint type = 0;
	if (jo) {
		type = nrnjava_env->CallStaticIntMethod(
			neuronCls, getObjectTypeID, jo);
	}
//printf("java2nrn_ometh type = %d\n", type);
	po = nj_j2hObject(jo, type);
	if (!po) {
		printf("%s.%s : do not recognize the return object type %d\n", hoc_object_name(ho), method->name, type);
	}
	errno = 0;
	return po;
}

/** a hoc object must be encapsulated in a java object for use in
java. The generic case for hoc objects opaque to java is HocObject
and it's java id for this purpose is 0.
Other kinds of hoc object which java should be able to do something
with, eg. Vector all have a corresponding java class that extends
HocObject and an id known here and in Neuron.java.
*/
jobject nj_encapsulate(Object* ho) {
	jobject jo;
	int type = 0;
	if ( ho->ctemplate->sym == nrn_vec_sym) {
		type = 1;
	}
//printf("nj_encapsulate %s %ld\n", hoc_object_name(ho), (long)ho);
	jo = nrnjava_env->CallStaticObjectMethod(neuronCls,
		encapsulateID, (jlong)ho, type);
	// refcount incremented already by h2jObject(Object*)
	return jo;
}

static void pwm_setwin(void* win, int type, int left, int top) {
//	printf("pwm_setwin %p %ld %d %d %d\n", win, type, left, top);
	if (nrnjava_root_env == nrnjava_env) {
		nrnjava_env->CallStaticIntMethod(neuronCls,
			setwinID, (jlong)win, type, left, top);
	}else{
		printf("nrnjava_env = %ld  nrnjava_root_env = %ld\n", (long)nrnjava_env, (long)nrnjava_root_env);
	}
}
