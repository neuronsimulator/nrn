#ifndef ASTUTILS_HPP
#define ASTUTILS_HPP

#include <string>

#include "lexer/modtoken.hpp"

namespace symtab {
    class SymbolTable;
}

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

    /* enum class for ast types */
    enum class Type;

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
        virtual Type getType() = 0;
        virtual std::string getTypeName() = 0;

        virtual std::string getName() {
            throw std::logic_error("getName() not implemented");
        }

        virtual AST* clone() {
            throw std::logic_error("clone() not implemented");
        }

        /* @todo: please revisit this. adding quickly for symtab */
        virtual ModToken* getToken() { /*std::cout << "\n ERROR: getToken not implemented!";*/
            return nullptr;
        }

        virtual std::shared_ptr<symtab::SymbolTable> getSymbolTable() {
            throw std::runtime_error("getSymbolTable() not implemented");
        }

        virtual ~AST() {
        }

        virtual bool isAST() {
            return true;
        }

        virtual bool isStatement() {
            return false;
        }

        virtual bool isExpression() {
            return false;
        }

        virtual bool isBlock() {
            return false;
        }

        virtual bool isIdentifier() {
            return false;
        }

        virtual bool isNumber() {
            return false;
        }

        virtual bool isString() {
            return false;
        }

        virtual bool isInteger() {
            return false;
        }

        virtual bool isFloat() {
            return false;
        }

        virtual bool isDouble() {
            return false;
        }

        virtual bool isBoolean() {
            return false;
        }

        virtual bool isName() {
            return false;
        }

        virtual bool isPrimeName() {
            return false;
        }

        virtual bool isVarName() {
            return false;
        }

        virtual bool isIndexedName() {
            return false;
        }

        virtual bool isArgument() {
            return false;
        }

        virtual bool isReactVarName() {
            return false;
        }

        virtual bool isReadIonVar() {
            return false;
        }

        virtual bool isWriteIonVar() {
            return false;
        }

        virtual bool isNonspeCurVar() {
            return false;
        }

        virtual bool isElectrodeCurVar() {
            return false;
        }

        virtual bool isSectionVar() {
            return false;
        }

        virtual bool isRangeVar() {
            return false;
        }

        virtual bool isGlobalVar() {
            return false;
        }

        virtual bool isPointerVar() {
            return false;
        }

        virtual bool isBbcorePointerVar() {
            return false;
        }

        virtual bool isExternVar() {
            return false;
        }

        virtual bool isThreadsafeVar() {
            return false;
        }

        virtual bool isParamBlock() {
            return false;
        }

        virtual bool isStepBlock() {
            return false;
        }

        virtual bool isIndependentBlock() {
            return false;
        }

        virtual bool isDependentBlock() {
            return false;
        }

        virtual bool isStateBlock() {
            return false;
        }

        virtual bool isPlotBlock() {
            return false;
        }

        virtual bool isInitialBlock() {
            return false;
        }

        virtual bool isConstructorBlock() {
            return false;
        }

        virtual bool isDestructorBlock() {
            return false;
        }

        virtual bool isStatementBlock() {
            return false;
        }

        virtual bool isDerivativeBlock() {
            return false;
        }

        virtual bool isLinearBlock() {
            return false;
        }

        virtual bool isNonLinearBlock() {
            return false;
        }

        virtual bool isDiscreteBlock() {
            return false;
        }

        virtual bool isPartialBlock() {
            return false;
        }

        virtual bool isFunctionTableBlock() {
            return false;
        }

        virtual bool isFunctionBlock() {
            return false;
        }

        virtual bool isProcedureBlock() {
            return false;
        }

        virtual bool isNetReceiveBlock() {
            return false;
        }

        virtual bool isSolveBlock() {
            return false;
        }

        virtual bool isBreakpointBlock() {
            return false;
        }

        virtual bool isTerminalBlock() {
            return false;
        }

        virtual bool isBeforeBlock() {
            return false;
        }

        virtual bool isAfterBlock() {
            return false;
        }

        virtual bool isBABlock() {
            return false;
        }

        virtual bool isForNetcon() {
            return false;
        }

        virtual bool isKineticBlock() {
            return false;
        }

        virtual bool isMatchBlock() {
            return false;
        }

        virtual bool isUnitBlock() {
            return false;
        }

        virtual bool isConstantBlock() {
            return false;
        }

        virtual bool isNeuronBlock() {
            return false;
        }

        virtual bool isUnit() {
            return false;
        }

        virtual bool isDoubleUnit() {
            return false;
        }

        virtual bool isLocalVar() {
            return false;
        }

        virtual bool isLimits() {
            return false;
        }

        virtual bool isNumberRange() {
            return false;
        }

        virtual bool isPlotVar() {
            return false;
        }

        virtual bool isBinaryOperator() {
            return false;
        }

        virtual bool isUnaryOperator() {
            return false;
        }

        virtual bool isReactionOperator() {
            return false;
        }

        virtual bool isBinaryExpression() {
            return false;
        }

        virtual bool isUnaryExpression() {
            return false;
        }

        virtual bool isNonLinEuation() {
            return false;
        }

        virtual bool isLinEquation() {
            return false;
        }

        virtual bool isFunctionCall() {
            return false;
        }

        virtual bool isFirstLastTypeIndex() {
            return false;
        }

        virtual bool isWatch() {
            return false;
        }

        virtual bool isQueueExpressionType() {
            return false;
        }

        virtual bool isMatch() {
            return false;
        }

        virtual bool isBABlockType() {
            return false;
        }

        virtual bool isUnitDef() {
            return false;
        }

        virtual bool isFactorDef() {
            return false;
        }

        virtual bool isValence() {
            return false;
        }

        virtual bool isUnitState() {
            return false;
        }

        virtual bool isLocalListStatement() {
            return false;
        }

        virtual bool isModel() {
            return false;
        }

        virtual bool isDefine() {
            return false;
        }

        virtual bool isInclude() {
            return false;
        }

        virtual bool isParamAssign() {
            return false;
        }

        virtual bool isStepped() {
            return false;
        }

        virtual bool isIndependentDef() {
            return false;
        }

        virtual bool isDependentDef() {
            return false;
        }

        virtual bool isPlotDeclaration() {
            return false;
        }

        virtual bool isConductanceHint() {
            return false;
        }

        virtual bool isExpressionStatement() {
            return false;
        }

        virtual bool isProtectStatement() {
            return false;
        }

        virtual bool isFromStatement() {
            return false;
        }

        virtual bool isForAllStatement() {
            return false;
        }

        virtual bool isWhileStatement() {
            return false;
        }

        virtual bool isIfStatement() {
            return false;
        }

        virtual bool isElseIfStatement() {
            return false;
        }

        virtual bool isElseStatement() {
            return false;
        }

        virtual bool isPartialEquation() {
            return false;
        }

        virtual bool isPartialBoundary() {
            return false;
        }

        virtual bool isWatchStatement() {
            return false;
        }

        virtual bool isMutexLock() {
            return false;
        }

        virtual bool isMutexUnlock() {
            return false;
        }

        virtual bool isReset() {
            return false;
        }

        virtual bool isSens() {
            return false;
        }

        virtual bool isConserve() {
            return false;
        }

        virtual bool isCompartment() {
            return false;
        }

        virtual bool isLDifuse() {
            return false;
        }

        virtual bool isReactionStatement() {
            return false;
        }

        virtual bool isLagStatement() {
            return false;
        }

        virtual bool isQueueStatement() {
            return false;
        }

        virtual bool isConstantStatement() {
            return false;
        }

        virtual bool isTableStatement() {
            return false;
        }

        virtual bool isSuffix() {
            return false;
        }

        virtual bool isUseion() {
            return false;
        }

        virtual bool isNonspecific() {
            return false;
        }

        virtual bool isElctrodeCurrent() {
            return false;
        }

        virtual bool isSection() {
            return false;
        }

        virtual bool isRange() {
            return false;
        }

        virtual bool isGlobal() {
            return false;
        }

        virtual bool isPointer() {
            return false;
        }

        virtual bool isBbcorePtr() {
            return false;
        }

        virtual bool isExternal() {
            return false;
        }

        virtual bool isThreadSafe() {
            return false;
        }

        virtual bool isVerbatim() {
            return false;
        }

        virtual bool isComment() {
            return false;
        }

        virtual bool isProgram() {
            return false;
        }
    };

}  // namespace ast

#endif
