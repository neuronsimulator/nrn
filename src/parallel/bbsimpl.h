#ifndef bbsimpl_h
#define bbsimpl_h

class BBSImpl {
public:
	BBSImpl();
	virtual ~BBSImpl();

	virtual boolean look(const char*) = 0;

	virtual void take(const char*) = 0; /* blocks til something to take */
	virtual boolean look_take(const char*) = 0; /* returns false if nothing to take */
	// after taking use these
	virtual int upkint() = 0;
	virtual double upkdouble() = 0;
	virtual void upkvec(int, double*) = 0;
	virtual char* upkstr() = 0; // delete [] char* when finished

	// before posting use these
	virtual void pkbegin() = 0;
	virtual void pkint(int) = 0;
	virtual void pkdouble(double) = 0;
	virtual void pkvec(int, double*) = 0;
	virtual void pkstr(const char*) = 0;
	virtual void post(const char*) = 0;

	virtual void post_todo(int parentid) = 0;
	virtual void post_result(int id) = 0;
	virtual int look_take_result(int pid) = 0; // returns id, or 0 if nothing
	virtual int look_take_todo() = 0; // returns id, or 0 if nothing
	virtual int take_todo() = 0; // returns id
	virtual void save_args(int userid) = 0;
	virtual void return_args(int userid);

	virtual void execute(int id); // assumes a "todo" message in receive buffer
	virtual int submit(int userid);
	virtual boolean working(int &id, double& x, int& userid);
	virtual void context();

	virtual void start();
	virtual void done();

	virtual void worker(); // forever execute
	virtual boolean is_master();
	virtual double time();
	
	virtual void perror(const char*);
public:
	int working_id_, n_;
	double wait_time_;
	double integ_time_;
	double send_time_;
	char* pickle_ret_;
	static boolean is_master_;
	static boolean started_, done_;
	static boolean use_pvm_;
	static int mytid_;
	static int debug_;
protected:
	char* execute_helper(); // involves hoc specific details in ocbbs.c
};

#endif
