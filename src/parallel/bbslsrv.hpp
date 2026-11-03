#pragma once

#include <string>
#include <variant>
#include <vector>

#include <InterViews/resource.h>

class MessageList;
class WorkList;
class ReadyList;
class ResultList;

using MessageItem = std::variant<int, double, std::vector<double>, std::vector<char>, std::string>;

class MessageValue: public Resource {
  public:
    void init_unpack();
    // following return 0 if success, -1 if failure
    int upkint(int*);
    int upkdouble(double*);
    int upkvec(int, double*);
    int upkstr(char*);
    int upkpickle(std::vector<char>&);

    int pkint(int);
    int pkdouble(double);
    int pkvec(int, double*);
    int pkstr(const char*);
    int pkpickle(const std::vector<char>&);

  private:
    std::vector<MessageItem> args_{};
    std::size_t index_{};
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
