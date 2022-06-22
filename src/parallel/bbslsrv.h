#ifndef bbslsrv_h
#define bbslsrv_h

#include <InterViews/resource.h>

class MessageList;
class WorkList;
class ReadyList;
class ResultList;

class MessageItem {
  public:
    MessageItem();
    virtual ~MessageItem();
    MessageItem* next_;
    int type_;
    size_t size_;  // for pickle type
    union {
        int i;
        double d;
        double* pd;
        char* s;
    } u;
};

class MessageValue: public Resource {
  public:
    MessageValue();
    virtual ~MessageValue();
    void init_unpack();
    // following return 0 if success, -1 if failure
    int upkint(int*);
    int upkdouble(double*);
    int upkvec(int, double*);
    int upkstr(char*);
    int upkpickle(char*, size_t*);

    int pkint(int);
    int pkdouble(double);
    int pkvec(int, double*);
    int pkstr(const char*);
    int pkpickle(const char*, size_t);

  private:
    MessageItem* link();

  private:
    MessageItem* first_;
    MessageItem* last_;
    MessageItem* unpack_;
};

class BBSLocalServer {
  public:
    BBSLocalServer();
    virtual ~BBSLocalServer();

    void post(const char* key, MessageValue*);
    bool look(const char* key, MessageValue**);
    bool look_take(const char* key, MessageValue**);

    void post_todo(int parentid, MessageValue*);
    void post_result(int id, MessageValue*);
    int look_take_todo(MessageValue**);
    int look_take_result(int pid, MessageValue**);

  private:
    MessageList* messages_;
    WorkList* work_;
    ReadyList* todo_;
    ResultList* results_;
    int next_id_;
};

#endif
