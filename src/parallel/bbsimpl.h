#ifndef bbsimpl_h
#define bbsimpl_h

class BBSImpl {
public:
	BBSImpl();
	virtual ~BBSImpl();

	virtual bool look(const char*) = 0;

	virtual void take(const char*) = 0; /* blocks til something to take */
	virtual bool look_take(const char*) = 0; /* returns false if nothing to take */
	// after taking use these
	virtual int upkint() = 0;
	virtual double upkdouble() = 0;
	virtual void upkvec(int, double*) = 0;
	virtual char* upkstr() = 0; // delete [] char* when finished
	virtual char* upkpickle(size_t*) = 0; // delete [] char* when finished

	// before posting use these
	virtual void pkbegin() = 0;
	virtual void pkint(int) = 0;
	virtual void pkdouble(double) = 0;
	virtual void pkvec(int, double*) = 0;
	virtual void pkstr(const char*) = 0;
	virtual void pkpickle(const char*, size_t) = 0;
	virtual void post(const char*) = 0;

	virtual void post_todo(int parentid) = 0;
	virtual void post_result(int id) = 0;
	virtual int look_take_result(int pid) = 0; // returns id, or 0 if nothing
	virtual int master_take_result(int pid); // returns id
	virtual int look_take_todo() = 0; // returns id, or 0 if nothing
	virtual int take_todo() = 0; // returns id
	virtual void save_args(int userid) = 0;
	virtual void return_args(int userid);

	virtual void execute(int id); // assumes a "todo" message in receive buffer
	virtual int submit(int userid);
	virtual bool working(int &id, double& x, int& userid);
	virtual void context();

	virtual void start();
	virtual void done();

	virtual void worker(); // forever execute
	virtual bool is_master();
	virtual double time();
	
	virtual void perror(const char*);
public:
	int runworker_called_;
	int working_id_, n_;
	double wait_time_;
	double integ_time_;
	double send_time_;
	char* pickle_ret_;
	size_t pickle_ret_size_;
	static bool is_master_;
	static bool started_, done_;
	static bool use_pvm_;
	static int mytid_;
	static int debug_;
	static bool master_works_;
protected:
	char* execute_helper(size_t*, int id); // involves hoc specific details in ocbbs.c
	void subworld_worker_execute(); //shadows execute_helper. ie. each of
		// the nrnmpi_myid_bbs workers (and master) need to execute
		// the same thing on each of the subworld processes
		// associated with nrnmpi_myid==0. A subworld does not
		// intracommunicate via the bulletin board but only via
		// mpi on the subworld communicator.
};

#endif
