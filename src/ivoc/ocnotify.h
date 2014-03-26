#ifndef ocnotify_h
#define ocnotify_h

#include <InterViews/observe.h>

#if defined(__cplusplus)
extern "C" {
#endif

void nrn_notify_freed(void (*pf)(void*, int));
void nrn_notify_when_void_freed(void* p, Observer* ob);
void nrn_notify_when_double_freed(double* p, Observer* ob);
void nrn_notify_pointer_disconnect(Observer* ob);
void notify_pointer_freed(void* pt);
void notify_freed(void* p);
void notify_freed_val_array(double* p, size_t);

#if defined(__cplusplus)
}
#endif

#endif
