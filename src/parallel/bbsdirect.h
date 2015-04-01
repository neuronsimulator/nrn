#ifndef bbsdirect_h
#define bbsdirect_h

#include <nrnmpiuse.h>

#include "bbsimpl.h"
class KeepArgs;
#if NRNMPI
struct bbsmpibuf;
#endif

// uses the pvm packing and unpacking functions but calls the
// server directly instead of through pvmd send, recv.
// i.e. this bbs can only be on the master

class BBSDirect : public BBSImpl {
public:
	BBSDirect();
	virtual ~BBSDirect();

	virtual bool look(const char*);

	virtual void take(const char*); /* blocks til something to take */
	virtual bool look_take(const char*); /* returns false if nothing to take */
	// after taking use these
	virtual int upkint();
	virtual double upkdouble();
	virtual void upkvec(int, double*);
	virtual char* upkstr(); // delete [] char* when finished
	virtual char* upkpickle(size_t*); // delete [] char* when finished

	// before posting use these
	virtual void pkbegin();
	virtual void pkint(int);
	virtual void pkdouble(double);
	virtual void pkvec(int, double*);
	virtual void pkstr(const char*);
	virtual void pkpickle(const char*, size_t);
	virtual void post(const char*);

	virtual void post_todo(int parentid);
	virtual void post_result(int id);
	virtual int look_take_result(int pid); // returns id, or 0 if nothing
	virtual int master_take_result(int pid); // returns id
	virtual int look_take_todo(); // returns id, or 0 if nothing
	virtual int take_todo(); // returns id
	virtual void save_args(int);
	virtual void return_args(int);

	virtual void context();
	
	virtual void start();
	virtual void done();

	virtual void perror(const char*);
	static void check_pvm();
private:
	KeepArgs* keepargs_;
#if NRNMPI
	bbsmpibuf* sendbuf_, *recvbuf_;
#endif
};

#endif
