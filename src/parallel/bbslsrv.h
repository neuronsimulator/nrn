#pragma once

#include <map>
#include <queue>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <InterViews/resource.h>

using MessageItem = std::variant<int, double, std::vector<double>, std::string>;

class MessageValue final: public Resource {
  public:
    MessageValue() = default;
    // following return 0 if success, -1 if failure
    int upkint(int*);
    int upkdouble(double*);
    int upkvec(int, double*);
    int upkstr(char*);
    int upkpickle(char*, size_t*);

    void pkint(int);
    void pkdouble(double);
    void pkvec(int, double*);
    void pkstr(const char*);
    void pkpickle(const char*, size_t);

  private:
    std::queue<MessageItem> unpack_{};
};

class WorkItem final {
  public:
    WorkItem(int id, MessageValue*);
    ~WorkItem();
    WorkItem* parent_{};
    int id_{};
    bool todo_less_than(const WorkItem*) const;
  private:
    MessageValue* val_{};
};

struct ltstr {
    bool operator()(const char* s1, const char* s2) const {
        return strcmp(s1, s2) < 0;
    }
};

struct ltWorkItem {
    bool operator()(const WorkItem* w1, const WorkItem* w2) const {
        return w1->todo_less_than(w2);
    }
};

using MessageList = std::multimap<const char*, const MessageValue*, ltstr>;
using WorkList = std::map<int, const WorkItem*>;
using ReadyList = std::set<WorkItem*, ltWorkItem>;
using ResultList = std::multimap<int, const WorkItem*>;

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
