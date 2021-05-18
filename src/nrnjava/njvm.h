#ifndef njmv_h
#define njmv_h

#define jnisave JNIEnv* jesave = nrnjava_env; nrnjava_env = env;
#define jnirestore nrnjava_env = jesave;


extern JNIEnv* nrnjava_env;
extern JNIEnv* nrnjava_root_env;



#endif
