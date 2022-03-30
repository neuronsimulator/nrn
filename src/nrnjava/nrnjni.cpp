/** JNI code for Neuron.java
C implementations of Java native methods
*/

#include <../../nrnconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OS/list.h>

#include "ivoc.h"
#include "nrnoc2iv.h"
#include "parse.hpp"
#include "ivocvect.h"

#include "neuron_Neuron.h"  // generated JNI Header file
#include "njvm.h"           // need nrnjava_env
#include "njreg.h"          // generated registration structure
                            // via mk_njreg.sh from neuron_Neuron.h
#include "neuron_Redirect.h"
#include "njredirreg.h"

char* nrn_dot2underbar(const char*);
jobject nj_encapsulate(Object*);
Object** nj_j2hObject(jobject, int);

declarePtrList(NJSymList, Symbol)
    // implemented in nrnjava.cpp
    // list of cTemplate of java registered classes in id order.
    // this parallels the classList in Neuron.Java
    extern NJSymList* nrn_jclass_symlist;
extern Symbol* nrn_jobj_sym;
extern Symbol* nrn_vec_sym;

// implemented in src/ivoc/pwman.cpp
void* nrnjava_pwm_listen(const char*, Object*);
void nrnjava_pwm_event(size_t, int, int, int, int, int);

extern Symlist* hoc_top_level_symlist;

extern "C" {

/** Create a hoc class from a java one
@param classindex : +ve id for class (0,1, ...)
*/
Symbol* java2nrn_class(const char* classname, int classindex, const char* methods);

extern double* hoc_varpointer;

extern Objectdata* hoc_top_level_data;
double hoc_integer(double);
Object* hoc_new_object(Symbol*, void*);
Object* hoc_newobj1(Symbol*, int);
#define call_ob_proc hoc_call_ob_proc
Object** hoc_objpop();
char** hoc_strpop();
void hoc_tobj_unref(Object**);
}

/*** Explicit registration required ***/
// the class loader apparently doesn't fill in the slots except
// when you dynamically load the native methods.

void nrnjni_registration(jclass neuronCls) {
    int cnt;
    for (cnt = 0; njreg_methods[cnt].name; ++cnt) {
    }
    nrnjava_env->RegisterNatives(neuronCls, njreg_methods, cnt);
}

void nrnjni_redirreg(jclass c) {
    int cnt;
    for (cnt = 0; njredirreg_methods[cnt].name; ++cnt) {
    }
    nrnjava_env->RegisterNatives(c, njredirreg_methods, cnt);
}

static void illegalArg(JNIEnv* env, const char* s) {
    //	env->ThrowNew(env->FindClass("java.lang.IllegalArgumentException"), s);
}

JNIEXPORT jstring JNICALL Java_neuron_Neuron_getHocStringArg(JNIEnv* env, jclass cls, jint idx) {
    char* str = "";
    if (!ifarg(idx)) {
        printf("error - missing string as arg %d\n", idx);
        illegalArg(env, "missing string");
        return nil;
    }
    if (hoc_is_str_arg(idx)) {
        str = gargstr(idx);
    } else {
        printf("error - expecting string as arg %d\n", idx);
        illegalArg(env, "expecting string");
        return nil;
    }
    jstring ret = env->NewStringUTF(str);
    return ret;
}

JNIEXPORT jdouble JNICALL Java_neuron_Neuron_getHocDoubleArg(JNIEnv* env,
                                                             jclass cls,
                                                             jint idx,
                                                             jint type) {
    double ret;
    if (!ifarg(idx)) {
        printf("error - missing double as arg %d\n", idx);
        illegalArg(env, "missing double");
        return (jdouble) -1.0E98;
    }
    if (hoc_is_double_arg(idx)) {
        ret = *(getarg(idx));
    } else {
        printf("error - expecting double as arg %d\n", idx);
        illegalArg(env, "expecting double");
        return (jdouble) -1.0E98;
    }
    if (type == 1) {
        ret = hoc_integer(ret);
    }
    return (jdouble) ret;
}

static jobject h2jObject(Object* o) {
    jobject jo;
    if (o == 0) {
        return 0;
    }
    hoc_obj_ref(o);
    // one of several possibilities here
    if (o->ctemplate->id < 0) {  // a Java object
        jo = (jobject) o->u.this_pointer;
    } else if (o->ctemplate->sym == nrn_jobj_sym) {
        // an unregistered java object
        jo = (jobject) o->u.this_pointer;
    } else {  // encapsulate in HocObject, HocVector, etc.
        jo = nj_encapsulate(o);
    }
    return jo;
}

JNIEXPORT jobject JNICALL Java_neuron_Neuron_getHocObjectArg(JNIEnv* env,
                                                             jclass cl,
                                                             jint i,
                                                             jthrowable e) {
    jobject ret;
    // printf("getHocObjArg %s  %ld\n", hoc_object_name(o), (long)o);
    if (!ifarg(i)) {
        printf("error - missing Object as arg %d\n", i);
        illegalArg(env, "missing Object");
        return e;
    }
    if (hoc_is_object_arg(i)) {
        Object* o = *hoc_objgetarg(i);
        ret = h2jObject(o);
    } else if (hoc_is_str_arg(i)) {  // encapsulate in String
        char* s = gargstr(i);
        ret = Java_neuron_Neuron_getHocStringArg(env, cl, i);
    } else {
        printf("error - expecting Object or strdef as arg %d\n", i);
        illegalArg(env, "expecting Object or strdef");
        ret = e;
    }
    return ret;
}

JNIEXPORT void JNICALL Java_neuron_Neuron_hocObjectUnref(JNIEnv*, jclass, jlong i) {
    Object* o = (Object*) i;
    // printf("hocObjectUnref %d %s\n", (long)i, hoc_object_name(o));
    hoc_obj_unref(o);
}

JNIEXPORT jlong JNICALL Java_neuron_Neuron_getVarPointer(JNIEnv*, jclass) {
    // there must have been a prior call to hoc_pointer_(&varname)
    // printf("getVarPointer %ld\n", (long)hoc_varpointer);
    return (jlong) hoc_varpointer;
}

JNIEXPORT jdouble JNICALL Java_neuron_Neuron_getVal(JNIEnv*, jclass, jlong p) {
    double* pd = (double*) p;
    return (jdouble) (*pd);
}

JNIEXPORT void JNICALL Java_neuron_Neuron_setVal(JNIEnv*, jclass, jlong p, jdouble d) {
    double* pd = (double*) p;
    *pd = d;
}

JNIEXPORT void JNICALL Java_neuron_Neuron_java2nrnClass(JNIEnv* env,
                                                        jclass cls,
                                                        jstring classname,
                                                        jint classindex,
                                                        jstring methods) {
    const char* cn = env->GetStringUTFChars(classname, 0);
    const char* m = env->GetStringUTFChars(methods, 0);
    char* mangled = nrn_dot2underbar(cn);
    env->ReleaseStringUTFChars(classname, cn);

    //	printf("class %s methods\n%s\n", mangled, m);
    Symbol* s = java2nrn_class(mangled, (int) classindex, m);
    nrn_jclass_symlist->append(s);
    delete[] mangled;
    env->ReleaseStringUTFChars(methods, m);
}

// Call a hoc method
JNIEXPORT jboolean JNICALL Java_neuron_Neuron_execute(JNIEnv* env, jclass, jstring statement) {
    //	assert(env == nrnjava_env);
    jnisave const char* s = env->GetStringUTFChars(statement, 0);
    jboolean r = Oc::valid_stmt(s, 0);
    env->ReleaseStringUTFChars(statement, s);
    jnirestore return r;
}


JNIEXPORT jint JNICALL Java_neuron_Neuron_getHocArgType(JNIEnv* env, jclass, jint i) {
    int type = hoc_argtype(i + 1);
    switch (type) {
    case NUMBER:
        return 1;
    case STRING:
        return 2;
    case OBJECTVAR:
    case OBJECTTMP:
        return 3;
    }
    return 0;
}

JNIEXPORT jstring JNICALL Java_neuron_Neuron_getHocArgSig(JNIEnv* env, jclass) {
    char sig[100];
    int i;
    for (i = 0; ifarg(i + 1); ++i) {
        int type = hoc_argtype(i + 1);
        switch (type) {
        case NUMBER:
            sig[i] = 'd';
            break;
        case STRING:
            sig[i] = 'S';
            break;
        case OBJECTVAR:
        case OBJECTTMP:
            sig[i] = 'o';
            break;
        }
    }
    sig[i] = '\0';
    //	printf("getHocArgSig |%s|\n", sig);
    return env->NewStringUTF(sig);
}

JNIEXPORT jlong JNICALL Java_neuron_Neuron_vectorNew(JNIEnv* env, jclass, jint size) {
    jnisave Vect* v = vector_new(size);
    Object* ho = hoc_new_object(nrn_vec_sym, v);
    hoc_obj_ref(ho);
    jlong jc = (jlong) ho;
    jnirestore return jc;
}

JNIEXPORT jint JNICALL Java_neuron_Neuron_vectorSize(JNIEnv*, jclass, jlong jc) {
    Object* ho = (Object*) jc;
    Vect* vec = (Vect*) ho->u.this_pointer;
    return vec->size();
}

static void outOfBounds(JNIEnv* env) {
    env->ThrowNew(env->FindClass("java.lang.IndexOutOfBoundsException"), "HocVector");
}

JNIEXPORT void JNICALL
Java_neuron_Neuron_vectorSet(JNIEnv* env, jclass, jlong jc, jint i, jdouble x) {
    Object* ho = (Object*) jc;
    Vect* vec = (Vect*) ho->u.this_pointer;
    if (i < 0 || i >= vec->size()) {
        printf("Neuron.vectorSet i=%d size=%d\n", i, vec->size());
        outOfBounds(env);
    }
    vec->elem(i) = x;
}

JNIEXPORT jdouble JNICALL Java_neuron_Neuron_vectorGet(JNIEnv* env, jclass, jlong jc, jint i) {
    Object* ho = (Object*) jc;
    Vect* vec = (Vect*) ho->u.this_pointer;
    if (i < 0 || i >= vec->size()) {
        printf("Neuron.vectorGet i=%d size=%d\n", i, vec->size());
        outOfBounds(env);
    }
    return vec->elem(i);
}

JNIEXPORT void JNICALL
Java_neuron_Neuron_vectorToHoc(JNIEnv* env, jclass, jlong jc, jdoubleArray ja, jint size) {
    Object* ho = (Object*) jc;
    Vect* vec = (Vect*) ho->u.this_pointer;
    vector_resize(vec, size);
    env->GetDoubleArrayRegion(ja, 0, size, &vec->elem(0));
}

JNIEXPORT jdoubleArray JNICALL Java_neuron_Neuron_vectorFromHoc(JNIEnv* env, jclass, jlong jc) {
    Object* ho = (Object*) jc;
    Vect* vec = (Vect*) ho->u.this_pointer;
    jint size = vec->size();
    jdoubleArray ja = env->NewDoubleArray(size);
    env->SetDoubleArrayRegion(ja, 0, size, &vec->elem(0));
    return ja;
}

JNIEXPORT jlong JNICALL Java_neuron_Neuron_cppPointer(JNIEnv*, jclass, jlong jc) {
    Object* ho = (Object*) jc;
    if (ho->ctemplate->sym->subtype == CPLUSOBJECT) {
        return (jlong) ho->u.this_pointer;
    } else {
        return 0;
    }
}

JNIEXPORT jstring JNICALL Java_neuron_Neuron_hocObjectName(JNIEnv* env, jclass, jlong jc) {
    Object* ho = (Object*) jc;
    char* s = hoc_object_name(ho);
    jstring js = env->NewStringUTF(s);
    return js;
}

JNIEXPORT jlong JNICALL Java_neuron_Neuron_getNewHObject(JNIEnv* env,
                                                         jclass,
                                                         jstring name,
                                                         jint narg) {
    jnisave const char* cn = env->GetStringUTFChars(name, 0);
    //	Symbol* s = hoc_table_lookup(cn, hoc_top_level_symlist);
    Symbol* s = hoc_lookup(cn);
    if (!s || s->type != TEMPLATE) {
        printf("getNewHObject could not lookup %s or it is not a template.\n", cn);
        return 0;
    }
    env->ReleaseStringUTFChars(name, cn);
    Object* o = hoc_newobj1(s, narg);
    hoc_obj_ref(o);
    jnirestore return (jlong) o;
}

JNIEXPORT jdouble JNICALL Java_neuron_Neuron_hDoubleMethod__Ljava_lang_String_2I(JNIEnv* env,
                                                                                 jclass,
                                                                                 jstring name,
                                                                                 jint narg) {
    jnisave const char* cn = env->GetStringUTFChars(name, 0);
    Symbol* sym = hoc_lookup(cn);
    env->ReleaseStringUTFChars(name, cn);
    jnirestore return hoc_call_func(sym, narg);
}

JNIEXPORT jdouble JNICALL
Java_neuron_Neuron_hDoubleMethod__JJI(JNIEnv* env, jclass, jlong jc, jlong mid, jint narg) {
    jnisave Object* ho = (Object*) jc;
    Symbol* s = (Symbol*) mid;
    // printf("hDoubleMethod %s.%s with %d args\n", hoc_object_name(ho), s->name, narg);
    call_ob_proc(ho, s, narg);
    jnirestore return hoc_xpop();
}

JNIEXPORT jstring JNICALL
Java_neuron_Neuron_hCharsMethod(JNIEnv* env, jclass, jlong jc, jlong mid, jint narg) {
    jnisave Object* ho = (Object*) jc;
    Symbol* sym = (Symbol*) mid;
    call_ob_proc(ho, sym, narg);
    char* s = *hoc_strpop();
    jstring js = env->NewStringUTF(s);
    jnirestore return js;
}

JNIEXPORT jobject JNICALL
Java_neuron_Neuron_hObjectMethod(JNIEnv* env, jclass, jlong jc, jlong mid, jint narg) {
    jnisave Object* ho = (Object*) jc;
    Symbol* s = (Symbol*) mid;
    call_ob_proc(ho, s, narg);
    Object** po = hoc_objpop();
    jobject jo = h2jObject(*po);
    hoc_tobj_unref(po);
    // o may need to get it's refcount decremented here.
    jnirestore return jo;
}

JNIEXPORT void JNICALL Java_neuron_Neuron_pushArgD(JNIEnv*, jclass, jdouble x) {
    // printf("pushArgD %g\n", x);
    hoc_pushx(x);
}

JNIEXPORT void JNICALL Java_neuron_Neuron_pushArgS(JNIEnv* env, jclass, jstring js) {
    // see java2nrn_smeth for a similar implementation
    // hopefully 20 is sufficient before a string is freed.
    // perhaps a more general implementation is needed.
    // It would be safe to free these when the corresponding
    // hCharsMethod call returns.
    static char** cs;
    static int i;
#define ncs 20
    const char* jc = env->GetStringUTFChars(js, 0);
    if (!cs) {
        cs = new char*[ncs];
        for (i = 0; i < ncs; ++i) {
            cs[i] = nil;
        }
        i = 0;
    }
    if (cs[i]) {
        delete[] cs[i];
    }
    i = (i + 1) % ncs;
    cs[i] = new char[strlen(jc + 1)];
    strcpy(cs[i], jc);
    hoc_pushstr(cs + i);
    // printf("pushArgS |%s|\n", cs[i]);
    env->ReleaseStringUTFChars(js, jc);
}

JNIEXPORT void JNICALL Java_neuron_Neuron_pushArgO(JNIEnv* env, jclass, jobject jo, jint type) {
    jnisave
        // printf("pushArgO jo=%p type=%d\n", jo, type);
        Object** po = nj_j2hObject(jo, type);
    if (!po) {
        printf("Do not recognize the object argument type %d\n", type);
    }
    hoc_pushobj(po);
    // printf("pushArgO %s refcount=%d\n", hoc_object_name(*po), (*po)->refcount);
    jnirestore
}

JNIEXPORT jlong JNICALL Java_neuron_Neuron_methodID(JNIEnv* env, jclass, jlong jc, jstring name) {
    const char* s = env->GetStringUTFChars(name, 0);
    Object* ho = (Object*) jc;
    Symbol* sym = hoc_table_lookup(s, ho->ctemplate->symtable);
    // printf("methodID %s %s\n", s, sym->name);
    env->ReleaseStringUTFChars(name, s);
    return (jlong) sym;
}

JNIEXPORT jlong JNICALL
Java_neuron_Neuron_pwmListen(JNIEnv* env, jclass, jstring title, jobject jo, jint type) {
    Object** po = nj_j2hObject(jo, type);
    Object* ho = po ? *po : nil;
    jnisave const char* s = env->GetStringUTFChars(title, 0);
    //	printf("pwmListen env=%ld %s\n", (long)env, s);
    jlong ic = (jlong) nrnjava_pwm_listen(s, ho);  // implementation in src/ivoc/pwman.cpp
    env->ReleaseStringUTFChars(title, s);
    jnirestore return ic;
}

JNIEXPORT void JNICALL Java_neuron_Neuron_pwmEvent(JNIEnv* env,
                                                   jclass,
                                                   jlong jwc,
                                                   jint type,
                                                   jint l,
                                                   jint t,
                                                   jint w,
                                                   jint h) {
    jnisave
        //	printf("pwmEvent env=%d type=%d l=%d t=%d w=%d h=%d\n",
        //(long)env, type, l, t, w, h);
        nrnjava_pwm_event(jwc, type, l, t, w, h);  // see src/ivoc/pwman.cpp
    jnirestore
}

JNIEXPORT jstring JNICALL Java_neuron_Neuron_sGet(JNIEnv* env, jclass, jstring js) {
    const char* s = env->GetStringUTFChars(js, 0);
    Symbol* sym = hoc_table_lookup(s, hoc_top_level_symlist);
    assert(sym && sym->type == STRING);
    char** sval = hoc_top_level_data[sym->u.oboff].ppstr;
    env->ReleaseStringUTFChars(js, s);
    return env->NewStringUTF(*sval);
}

JNIEXPORT jobject JNICALL Java_neuron_Neuron_oGet(JNIEnv* env, jclass, jstring js) {
    const char* s = env->GetStringUTFChars(js, 0);
    Symbol* sym = hoc_table_lookup(s, hoc_top_level_symlist);
    assert(sym && sym->type == OBJECTVAR);
    Object** po = hoc_top_level_data[sym->u.oboff].pobj;
    env->ReleaseStringUTFChars(js, s);
    jobject jo = h2jObject(*po);
    hoc_obj_unref(*po);
    return jo;
}

JNIEXPORT jstring JNICALL Java_neuron_Neuron_hGetStringField(JNIEnv* env,
                                                             jclass,
                                                             jlong v,
                                                             jstring js) {
    const char* s = env->GetStringUTFChars(js, 0);
    Object* o = (Object*) v;
    Symbol* sym = hoc_table_lookup(s, o->ctemplate->symtable);
    assert(sym && sym->type == STRING);
    char** sval = hoc_top_level_data[sym->u.oboff].ppstr;
    env->ReleaseStringUTFChars(js, s);
    return env->NewStringUTF(*sval);
}

JNIEXPORT jobject JNICALL Java_neuron_Neuron_hGetObjectField(JNIEnv* env,
                                                             jclass,
                                                             jlong v,
                                                             jstring js) {
    const char* s = env->GetStringUTFChars(js, 0);
    Object* o = (Object*) v;
    Symbol* sym = hoc_table_lookup(s, o->ctemplate->symtable);
    assert(sym && sym->type == OBJECTVAR);
    Object** po = o->u.dataspace[sym->u.oboff].pobj;
    env->ReleaseStringUTFChars(js, s);
    jobject jo = h2jObject(*po);
    hoc_obj_unref(*po);
    return jo;
}

JNIEXPORT void JNICALL
Java_neuron_Neuron_hSetObjectField__JLjava_lang_String_2Ljava_lang_Object_2I(JNIEnv* env,
                                                                             jclass,
                                                                             jlong v,
                                                                             jstring js,
                                                                             jobject joval,
                                                                             jint type) {
    jnisave const char* s = env->GetStringUTFChars(js, 0);
    Object* o = (Object*) v;
    Symbol* sym = hoc_table_lookup(s, o->ctemplate->symtable);
    assert(sym && sym->type == OBJECTVAR);
    Object** po = o->u.dataspace[sym->u.oboff].pobj;
    Object** poval = nj_j2hObject(joval, type);
    Object* old = *po;
    *po = *poval;
    (*po)->refcount++;
    hoc_obj_unref(old);
    env->ReleaseStringUTFChars(js, s);
    jnirestore
}


JNIEXPORT void JNICALL
Java_neuron_Neuron_hSetObjectField__Ljava_lang_String_2Ljava_lang_Object_2I(JNIEnv* env,
                                                                            jclass,
                                                                            jstring js,
                                                                            jobject joval,
                                                                            jint type) {
    jnisave const char* s = env->GetStringUTFChars(js, 0);
    Symbol* sym = hoc_table_lookup(s, hoc_top_level_symlist);
    assert(sym && sym->type == OBJECTVAR);
    Object** po = hoc_top_level_data[sym->u.oboff].pobj;
    Object** poval = nj_j2hObject(joval, type);
    Object* old = *po;
    *po = *poval;
    (*po)->refcount++;
    hoc_obj_unref(old);
    env->ReleaseStringUTFChars(js, s);
    jnirestore
}


JNIEXPORT void JNICALL Java_neuron_Redirect_consoleWrite(JNIEnv* env, jclass, jint fd, jint b) {
    jnisave
#ifdef _MSWIN
        if (b != '\r') {
        printf("%c", b);
    }
#else
#ifdef MAC
        if (b == '\r') {
        b = '\n';
    }
    printf("%c", b);
#else
        printf("%c", b);
#endif
#endif
    //	if (fd == 1) {
    //		fputc(b, stdout);
    //	}else{
    //		fputc(b, stderr);
    //	}
    jnirestore
}
