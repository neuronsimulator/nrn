#ifndef bbsrcli_h
#define bbsrcli_h

#include <nrnmpiuse.h>
#include "bbsimpl.h"
class KeepArgs;
struct bbsmpibuf;

class BBSClient : public BBSImpl{ // implemented as PVM Client
public:
	BBSClient();
	virtual ~BBSClient();

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
	virtual int look_take_todo(); // returns id, or 0 if nothing
	virtual int take_todo(); // returns id
	virtual void save_args(int);
	virtual void return_args(int);

	virtual void start();
	virtual void done();

	virtual void perror(const char*);
private:
	int get(const char* key, int type); // return type
	int get(int key, int type); // return type
	int get(int type); // return type
private:
	static int sid_;
	KeepArgs* keepargs_;
#if NRNMPI
	void upkbegin();
	char* getkey();
	int getid();
	bbsmpibuf* sendbuf_, *recvbuf_, *request_;
#endif
	
};

#endif
