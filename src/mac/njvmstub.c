#include <jni.h>
#pragma export on

jint nrn2_GetDefaultJavaVMInitArgs(void* args);
jint nrn2_CreateJavaVM(JavaVM **pvm, JNIEnv **penv, void *args);

#pragma export off

jint nrn2_GetDefaultJavaVMInitArgs(void* args) {
	return JNI_GetDefaultJavaVMInitArgs(args);
}

jint nrn2_CreateJavaVM(JavaVM **pvm, JNIEnv **penv, void *args){
	return JNI_CreateJavaVM(pvm, penv, args);
}

