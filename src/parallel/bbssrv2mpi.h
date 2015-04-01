#ifndef bbslsrv2_h
#define bbslsrv2_h

#include <nrnmpiuse.h>
#include <InterViews/resource.h>

class MpiMessageList;
class MpiPendingList;
class MpiWorkList;
class MpiReadyList;
class MpiLookingToDoList;
class MpiResultList;
struct bbsmpibuf;

extern "C" {
	void bbs_handle();
}

class BBSDirectServer {
public:
	BBSDirectServer();
	virtual ~BBSDirectServer();

	void post(const char* key, bbsmpibuf*);
	bool look(const char* key, bbsmpibuf**);
	bool look_take(const char* key, bbsmpibuf**);
	bool take_pending(const char* key, int* cid);
	void put_pending(const char* key, int cid);
	static BBSDirectServer* server_;
	static void handle(); // all remote requests
	static void handle_block();
	static void handle1(int size, int tag, int source);
	void start();
	void done();

	void post_todo(int parentid, int cid, bbsmpibuf*);
	void context(bbsmpibuf*);
	bool send_context(int cid); // sends if not sent already
	void post_result(int id, bbsmpibuf*);
	int look_take_todo(bbsmpibuf**);
	int look_take_result(int parentid, bbsmpibuf**);
	void context_wait();
private:
	void add_looking_todo(int cid);
private:
	MpiMessageList* messages_;
	MpiPendingList* pending_;
	MpiWorkList* work_;
	MpiLookingToDoList* looking_todo_;
	MpiReadyList* todo_;
	MpiResultList* results_;
	MpiLookingToDoList* send_context_;
	int next_id_;
	bbsmpibuf* context_buf_;
	int remaining_context_cnt_;
};

#endif
