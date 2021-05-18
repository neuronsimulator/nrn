#ifndef bbslsrv2_h
#define bbslsrv2_h

#include <InterViews/resource.h>

void bbs_handle();

class BBSDirectServer {
public:
	BBSDirectServer();
	virtual ~BBSDirectServer();

	void post(const char* key);
	bool look(const char* key);
	bool look_take(const char* key);
	bool take_pending(const char* key, int* cid);
	void put_pending(const char* key, int cid);
	static BBSDirectServer* server_;
	static void handle(); // all remote requests
	static void handle1();	
	void start();
	void done();

	void post_todo(int parentid, int cid);
	void context(int ncid, int* cids);
	bool send_context(int cid); // sends if not sent already
	void post_result(int id);
	int look_take_todo();
	int look_take_result(int parentid);	
private:
	void add_looking_todo(int cid);
private:
	int next_id_;
	int context_buf_;
	int remaining_context_cnt_;
};

#endif
