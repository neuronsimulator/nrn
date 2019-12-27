#ifndef nrnjava_h
#define nrnjava_h

#if defined(__cplusplus)
#extern "C" {
#endif

extern void* (*p_java2nrn_cons)(Object*);
extern void  (*p_java2nrn_destruct)(void* opaque_java_object);
extern double (*p_java2nrn_dmeth)(Object* ho, Symbol* method);
extern char** (*p_java2nrn_smeth)(Object* ho, Symbol* method);
extern Object** (*p_java2nrn_ometh)(Object* ho, Symbol* method);
extern const char* (*p_java2nrn_classname)(Object* ho);
extern Symbol* java2nrn_class(const char* classname, int classindex, const char* methods);
extern int (*p_hoc_load_java)(void);

#if defined(__cplusplus)
}
#endif

#endif
