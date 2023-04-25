#pragma once
#include <ostream>
union Inst;
struct Object;
union Objectdata;
struct Symlist;

struct ObjectContext {
    [[nodiscard]] ObjectContext(Object*);
    ~ObjectContext();

  private:
    Object* a1;
    Objectdata* a2;
    int a4;
    Symlist* a5;
};

struct OcJump {
    static bool execute(Inst* p);
    /**
     * @brief Execute code with state machine cleanup.
     * @param code Null-terminated code, potentially multiple expressions.
     * @param os If non-null, print an error message to this stream when returning false.
     * @return true if the code was successfully executed, false otherwise.
     * @throw on errors not directly related to the code
     * @todo Rename something like check_execute?
     *
     * This does not modify the Object context.
     */
    static bool execute(const char* code, std::ostream* os = nullptr);

    /**
     * @brief Execute code with state machine cleanup.
     * @param code Null-terminated code, potentially multiple expressions.
     * @param ob Object in whose context to execute the code. nullptr implies top-level context.
     * @param os If non-null, print an error message to this stream when returning false.
     * @return true if the code was successfully executed, false otherwise.
     * @throw on errors not directly related to the code
     * @todo Rename something like check_execute?
     */
    static bool execute(const char* code, Object* ob, std::ostream* os = nullptr);

    /**
     * @brief Execute code with state machine cleanup.
     */
    static void execute_unchecked(const char* code);

    /**
     * @brief Execute code with state machine cleanup.
     */
    static void execute_unchecked(const char* code, Object* ob);

    static void* fpycall(void* (*) (void*, void*), void*, void*);
};
