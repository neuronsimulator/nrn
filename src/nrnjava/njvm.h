#ifndef njmv_h
#define njmv_h

#define jnisave JNIEnv* jesave = nrnjava_env; nrnjava_env = env;
#define jnirestore nrnjava_env = jesave;

#if defined(__cplusplus)
extern "C" {
#endif

extern JNIEnv* nrnjava_env;
extern JNIEnv* nrnjava_root_env;

#if defined(__cplusplus)
}
#endif


#endif
