#pragma once

#include <string>

namespace ast {

    /* enumaration of all binary operators in the language */
    typedef enum {
        BOP_ADDITION,
        BOP_SUBTRACTION,
        BOP_MULTIPLICATION,
        BOP_DIVISION,
        BOP_POWER,
        BOP_AND,
        BOP_OR,
        BOP_GREATER,
        BOP_LESS,
        BOP_GREATER_EQUAL,
        BOP_LESS_EQUAL,
        BOP_ASSIGN,
        BOP_NOT_EQUAL,
        BOP_EXACT_EQUAL
    } BinaryOp;

    static const std::string BinaryOpNames[] = {"+", "-", "*",  "/",  "^", "&&", "||",
                                                ">", "<", ">=", "<=", "=", "!=", "=="};

    /* enumaration of all unary operators in the language */
    typedef enum { UOP_NOT, UOP_NEGATION } UnaryOp;
    static const std::string UnaryOpNames[] = {"!", "-"};

    /* enumaration of types used in partial equation */
    typedef enum { PEQ_FIRST, PEQ_LAST } FirstLastType;
    static const std::string FirstLastTypeNames[] = {"FIRST", "LAST"};

    /* enumaration of queue types */
    typedef enum { PUT_QUEUE, GET_QUEUE } QueueType;
    static const std::string QueueTypeNames[] = {"PUTQ", "GETQ"};

    /* enumaration of type used for BEFORE or AFTER block */
    typedef enum { BATYPE_BREAKPOINT, BATYPE_SOLVE, BATYPE_INITIAL, BATYPE_STEP } BAType;
    static const std::string BATypeNames[] = {"BREAKPOINT", "SOLVE", "INITIAL", "STEP"};

    /* enumaration of type used for UNIT_ON or UNIT_OFF state*/
    typedef enum { UNIT_ON, UNIT_OFF } UnitStateType;
    static const std::string UnitStateTypeNames[] = {"UNITSON", "UNITSOFF"};

    /* enumaration of type used for Reaction statement */
    typedef enum { LTMINUSGT, LTLT, MINUSGT } ReactionOp;
    static const std::string ReactionOpNames[] = {"<->", "<<", "->"};

    /* base class for all visitors implementation */
    class Visitor;

    /* define abstract base class for all AST nodes
     * this also serves to define the visitable objects.
     */
    class AST {
      public:
        /* all AST nodes have a member which stores their
         * basetype (int, bool, none, object). Further type
         * information will come from the symbol table.
         * BaseType basetype;
         */

        /* all AST nodes provide visit children and accept methods */
        virtual void visitChildren(Visitor* v) = 0;
        virtual void accept(Visitor* v) = 0;
        virtual std::string getType() = 0;
        /* @todo: please revisit this. adding quickly for symtab */
        virtual std::string getName() {
            return "";
        }
        virtual ModToken* getToken() { /*std::cout << "\n ERROR: getToken not implemented!";*/
            return NULL;
        }
        // virtual AST* clone() { std::cout << "\n ERROR: clone() not implemented! \n"; abort(); }
    };

}  // namespace ast
