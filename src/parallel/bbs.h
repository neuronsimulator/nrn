#ifndef bbs_h
#define bbs_h

#include "bbsimpl.h"

class IvocVect;

class BBS {
public:
	BBS();
	BBS(int nhost);
	virtual ~BBS();

	bool look(const char*);

	void take(const char*); /* blocks til something to take */
	bool look_take(const char*); /* returns false if nothing to take */
	// after taking use these
	int upkint();
	double upkdouble();
	void upkvec(int n, double* px); // n input px space must exist
	char* upkstr(); // delete [] char* when finished
	char* upkpickle(size_t* size); // delete [] char* when finished

	// before posting use these
	void pkbegin();
	void pkint(int);
	void pkdouble(double);
	void pkvec(int n, double* px); // doesn't pack n
	void pkstr(const char*);
	void pkpickle(const char*, size_t size);
	void post(const char*);

	int submit(int userid);
	bool working(int &id, double& x, int& userid);
	void master_works(int flag);
	void context();

	bool is_master();
	void worker(); // forever execute
	void done(); // prints timing

	void perror(const char*);
	double time();
	double wait_time();
	double integ_time();
	double send_time();
	void add_wait_time(double); // add interval since arg

	int nhost();
	int myid();

	// netpar interface
	void set_gid2node(int, int);
	int gid_exists(int);
	double threshold();
	void cell();
	void outputcell(int);
	void spike_record(int, IvocVect*, IvocVect*);
	void netpar_solve(double);
	Object** gid2obj(int);
	Object** gid2cell(int);
	Object** gid_connect(int);
	double netpar_mindelay(double maxdelay);
	void netpar_spanning_statistics(int*, int*, int*, int*);
	IvocVect* netpar_max_histogram(IvocVect*);
	Object** pyret();
protected:
	void init(int);
protected:
	BBSImpl* impl_;
};

#endif
