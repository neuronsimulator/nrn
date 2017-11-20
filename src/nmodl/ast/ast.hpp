#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ast/ast_utils.hpp"
#include "lexer/modtoken.hpp"
#include "utils/common_utils.hpp"

/* all classes representing Abstract Syntax Tree (AST) nodes */
namespace ast {

    /* forward declarations of AST classes */
    class Statement;
    class Expression;
    class Block;
    class Identifier;
    class Number;
    class String;
    class Integer;
    class Float;
    class Double;
    class Boolean;
    class Name;
    class PrimeName;
    class VarName;
    class IndexedName;
    class Argument;
    class ReactVarName;
    class ReadIonVar;
    class WriteIonVar;
    class NonspeCurVar;
    class ElectrodeCurVar;
    class SectionVar;
    class RangeVar;
    class GlobalVar;
    class PointerVar;
    class BbcorePointerVar;
    class ExternVar;
    class ThreadsafeVar;
    class ParamBlock;
    class StepBlock;
    class IndependentBlock;
    class DependentBlock;
    class StateBlock;
    class PlotBlock;
    class InitialBlock;
    class ConstructorBlock;
    class DestructorBlock;
    class StatementBlock;
    class DerivativeBlock;
    class LinearBlock;
    class NonLinearBlock;
    class DiscreteBlock;
    class PartialBlock;
    class FunctionTableBlock;
    class FunctionBlock;
    class ProcedureBlock;
    class NetReceiveBlock;
    class SolveBlock;
    class BreakpointBlock;
    class TerminalBlock;
    class BeforeBlock;
    class AfterBlock;
    class BABlock;
    class ForNetcon;
    class KineticBlock;
    class MatchBlock;
    class UnitBlock;
    class ConstantBlock;
    class NeuronBlock;
    class Unit;
    class DoubleUnit;
    class LocalVariable;
    class Limits;
    class NumberRange;
    class PlotVariable;
    class BinaryOperator;
    class UnaryOperator;
    class ReactionOperator;
    class BinaryExpression;
    class UnaryExpression;
    class NonLinEuation;
    class LinEquation;
    class FunctionCall;
    class FirstLastTypeIndex;
    class Watch;
    class QueueExpressionType;
    class Match;
    class BABlockType;
    class UnitDef;
    class FactorDef;
    class Valence;
    class UnitState;
    class LocalListStatement;
    class Model;
    class Define;
    class Include;
    class ParamAssign;
    class Stepped;
    class IndependentDef;
    class DependentDef;
    class PlotDeclaration;
    class ConductanceHint;
    class ExpressionStatement;
    class ProtectStatement;
    class FromStatement;
    class ForAllStatement;
    class WhileStatement;
    class IfStatement;
    class ElseIfStatement;
    class ElseStatement;
    class PartialEquation;
    class PartialBoundary;
    class WatchStatement;
    class MutexLock;
    class MutexUnlock;
    class Reset;
    class Sens;
    class Conserve;
    class Compartment;
    class LDifuse;
    class ReactionStatement;
    class LagStatement;
    class QueueStatement;
    class ConstantStatement;
    class TableStatement;
    class NrnSuffix;
    class NrnUseion;
    class NrnNonspecific;
    class NrnElctrodeCurrent;
    class NrnSection;
    class NrnRange;
    class NrnGlobal;
    class NrnPointer;
    class NrnBbcorePtr;
    class NrnExternal;
    class NrnThreadSafe;
    class Verbatim;
    class Comment;
    class Program;

    /* std::vector for convenience */
    using StatementVector = std::vector<std::shared_ptr<Statement>>;
    using ExpressionVector = std::vector<std::shared_ptr<Expression>>;
    using BlockVector = std::vector<std::shared_ptr<Block>>;
    using IdentifierVector = std::vector<std::shared_ptr<Identifier>>;
    using NumberVector = std::vector<std::shared_ptr<Number>>;
    using StringVector = std::vector<std::shared_ptr<String>>;
    using IntegerVector = std::vector<std::shared_ptr<Integer>>;
    using FloatVector = std::vector<std::shared_ptr<Float>>;
    using DoubleVector = std::vector<std::shared_ptr<Double>>;
    using BooleanVector = std::vector<std::shared_ptr<Boolean>>;
    using NameVector = std::vector<std::shared_ptr<Name>>;
    using PrimeNameVector = std::vector<std::shared_ptr<PrimeName>>;
    using VarNameVector = std::vector<std::shared_ptr<VarName>>;
    using IndexedNameVector = std::vector<std::shared_ptr<IndexedName>>;
    using ArgumentVector = std::vector<std::shared_ptr<Argument>>;
    using ReactVarNameVector = std::vector<std::shared_ptr<ReactVarName>>;
    using ReadIonVarVector = std::vector<std::shared_ptr<ReadIonVar>>;
    using WriteIonVarVector = std::vector<std::shared_ptr<WriteIonVar>>;
    using NonspeCurVarVector = std::vector<std::shared_ptr<NonspeCurVar>>;
    using ElectrodeCurVarVector = std::vector<std::shared_ptr<ElectrodeCurVar>>;
    using SectionVarVector = std::vector<std::shared_ptr<SectionVar>>;
    using RangeVarVector = std::vector<std::shared_ptr<RangeVar>>;
    using GlobalVarVector = std::vector<std::shared_ptr<GlobalVar>>;
    using PointerVarVector = std::vector<std::shared_ptr<PointerVar>>;
    using BbcorePointerVarVector = std::vector<std::shared_ptr<BbcorePointerVar>>;
    using ExternVarVector = std::vector<std::shared_ptr<ExternVar>>;
    using ThreadsafeVarVector = std::vector<std::shared_ptr<ThreadsafeVar>>;
    using ParamBlockVector = std::vector<std::shared_ptr<ParamBlock>>;
    using StepBlockVector = std::vector<std::shared_ptr<StepBlock>>;
    using IndependentBlockVector = std::vector<std::shared_ptr<IndependentBlock>>;
    using DependentBlockVector = std::vector<std::shared_ptr<DependentBlock>>;
    using StateBlockVector = std::vector<std::shared_ptr<StateBlock>>;
    using PlotBlockVector = std::vector<std::shared_ptr<PlotBlock>>;
    using InitialBlockVector = std::vector<std::shared_ptr<InitialBlock>>;
    using ConstructorBlockVector = std::vector<std::shared_ptr<ConstructorBlock>>;
    using DestructorBlockVector = std::vector<std::shared_ptr<DestructorBlock>>;
    using StatementBlockVector = std::vector<std::shared_ptr<StatementBlock>>;
    using DerivativeBlockVector = std::vector<std::shared_ptr<DerivativeBlock>>;
    using LinearBlockVector = std::vector<std::shared_ptr<LinearBlock>>;
    using NonLinearBlockVector = std::vector<std::shared_ptr<NonLinearBlock>>;
    using DiscreteBlockVector = std::vector<std::shared_ptr<DiscreteBlock>>;
    using PartialBlockVector = std::vector<std::shared_ptr<PartialBlock>>;
    using FunctionTableBlockVector = std::vector<std::shared_ptr<FunctionTableBlock>>;
    using FunctionBlockVector = std::vector<std::shared_ptr<FunctionBlock>>;
    using ProcedureBlockVector = std::vector<std::shared_ptr<ProcedureBlock>>;
    using NetReceiveBlockVector = std::vector<std::shared_ptr<NetReceiveBlock>>;
    using SolveBlockVector = std::vector<std::shared_ptr<SolveBlock>>;
    using BreakpointBlockVector = std::vector<std::shared_ptr<BreakpointBlock>>;
    using TerminalBlockVector = std::vector<std::shared_ptr<TerminalBlock>>;
    using BeforeBlockVector = std::vector<std::shared_ptr<BeforeBlock>>;
    using AfterBlockVector = std::vector<std::shared_ptr<AfterBlock>>;
    using BABlockVector = std::vector<std::shared_ptr<BABlock>>;
    using ForNetconVector = std::vector<std::shared_ptr<ForNetcon>>;
    using KineticBlockVector = std::vector<std::shared_ptr<KineticBlock>>;
    using MatchBlockVector = std::vector<std::shared_ptr<MatchBlock>>;
    using UnitBlockVector = std::vector<std::shared_ptr<UnitBlock>>;
    using ConstantBlockVector = std::vector<std::shared_ptr<ConstantBlock>>;
    using NeuronBlockVector = std::vector<std::shared_ptr<NeuronBlock>>;
    using UnitVector = std::vector<std::shared_ptr<Unit>>;
    using DoubleUnitVector = std::vector<std::shared_ptr<DoubleUnit>>;
    using LocalVariableVector = std::vector<std::shared_ptr<LocalVariable>>;
    using LimitsVector = std::vector<std::shared_ptr<Limits>>;
    using NumberRangeVector = std::vector<std::shared_ptr<NumberRange>>;
    using PlotVariableVector = std::vector<std::shared_ptr<PlotVariable>>;
    using BinaryOperatorVector = std::vector<std::shared_ptr<BinaryOperator>>;
    using UnaryOperatorVector = std::vector<std::shared_ptr<UnaryOperator>>;
    using ReactionOperatorVector = std::vector<std::shared_ptr<ReactionOperator>>;
    using BinaryExpressionVector = std::vector<std::shared_ptr<BinaryExpression>>;
    using UnaryExpressionVector = std::vector<std::shared_ptr<UnaryExpression>>;
    using NonLinEuationVector = std::vector<std::shared_ptr<NonLinEuation>>;
    using LinEquationVector = std::vector<std::shared_ptr<LinEquation>>;
    using FunctionCallVector = std::vector<std::shared_ptr<FunctionCall>>;
    using FirstLastTypeIndexVector = std::vector<std::shared_ptr<FirstLastTypeIndex>>;
    using WatchVector = std::vector<std::shared_ptr<Watch>>;
    using QueueExpressionTypeVector = std::vector<std::shared_ptr<QueueExpressionType>>;
    using MatchVector = std::vector<std::shared_ptr<Match>>;
    using BABlockTypeVector = std::vector<std::shared_ptr<BABlockType>>;
    using UnitDefVector = std::vector<std::shared_ptr<UnitDef>>;
    using FactorDefVector = std::vector<std::shared_ptr<FactorDef>>;
    using ValenceVector = std::vector<std::shared_ptr<Valence>>;
    using UnitStateVector = std::vector<std::shared_ptr<UnitState>>;
    using LocalListStatementVector = std::vector<std::shared_ptr<LocalListStatement>>;
    using ModelVector = std::vector<std::shared_ptr<Model>>;
    using DefineVector = std::vector<std::shared_ptr<Define>>;
    using IncludeVector = std::vector<std::shared_ptr<Include>>;
    using ParamAssignVector = std::vector<std::shared_ptr<ParamAssign>>;
    using SteppedVector = std::vector<std::shared_ptr<Stepped>>;
    using IndependentDefVector = std::vector<std::shared_ptr<IndependentDef>>;
    using DependentDefVector = std::vector<std::shared_ptr<DependentDef>>;
    using PlotDeclarationVector = std::vector<std::shared_ptr<PlotDeclaration>>;
    using ConductanceHintVector = std::vector<std::shared_ptr<ConductanceHint>>;
    using ExpressionStatementVector = std::vector<std::shared_ptr<ExpressionStatement>>;
    using ProtectStatementVector = std::vector<std::shared_ptr<ProtectStatement>>;
    using FromStatementVector = std::vector<std::shared_ptr<FromStatement>>;
    using ForAllStatementVector = std::vector<std::shared_ptr<ForAllStatement>>;
    using WhileStatementVector = std::vector<std::shared_ptr<WhileStatement>>;
    using IfStatementVector = std::vector<std::shared_ptr<IfStatement>>;
    using ElseIfStatementVector = std::vector<std::shared_ptr<ElseIfStatement>>;
    using ElseStatementVector = std::vector<std::shared_ptr<ElseStatement>>;
    using PartialEquationVector = std::vector<std::shared_ptr<PartialEquation>>;
    using PartialBoundaryVector = std::vector<std::shared_ptr<PartialBoundary>>;
    using WatchStatementVector = std::vector<std::shared_ptr<WatchStatement>>;
    using MutexLockVector = std::vector<std::shared_ptr<MutexLock>>;
    using MutexUnlockVector = std::vector<std::shared_ptr<MutexUnlock>>;
    using ResetVector = std::vector<std::shared_ptr<Reset>>;
    using SensVector = std::vector<std::shared_ptr<Sens>>;
    using ConserveVector = std::vector<std::shared_ptr<Conserve>>;
    using CompartmentVector = std::vector<std::shared_ptr<Compartment>>;
    using LDifuseVector = std::vector<std::shared_ptr<LDifuse>>;
    using ReactionStatementVector = std::vector<std::shared_ptr<ReactionStatement>>;
    using LagStatementVector = std::vector<std::shared_ptr<LagStatement>>;
    using QueueStatementVector = std::vector<std::shared_ptr<QueueStatement>>;
    using ConstantStatementVector = std::vector<std::shared_ptr<ConstantStatement>>;
    using TableStatementVector = std::vector<std::shared_ptr<TableStatement>>;
    using NrnSuffixVector = std::vector<std::shared_ptr<NrnSuffix>>;
    using NrnUseionVector = std::vector<std::shared_ptr<NrnUseion>>;
    using NrnNonspecificVector = std::vector<std::shared_ptr<NrnNonspecific>>;
    using NrnElctrodeCurrentVector = std::vector<std::shared_ptr<NrnElctrodeCurrent>>;
    using NrnSectionVector = std::vector<std::shared_ptr<NrnSection>>;
    using NrnRangeVector = std::vector<std::shared_ptr<NrnRange>>;
    using NrnGlobalVector = std::vector<std::shared_ptr<NrnGlobal>>;
    using NrnPointerVector = std::vector<std::shared_ptr<NrnPointer>>;
    using NrnBbcorePtrVector = std::vector<std::shared_ptr<NrnBbcorePtr>>;
    using NrnExternalVector = std::vector<std::shared_ptr<NrnExternal>>;
    using NrnThreadSafeVector = std::vector<std::shared_ptr<NrnThreadSafe>>;
    using VerbatimVector = std::vector<std::shared_ptr<Verbatim>>;
    using CommentVector = std::vector<std::shared_ptr<Comment>>;
    using ProgramVector = std::vector<std::shared_ptr<Program>>;
    using NumberVector = std::vector<std::shared_ptr<Number>>;
    using IdentifierVector = std::vector<std::shared_ptr<Identifier>>;
    using BlockVector = std::vector<std::shared_ptr<Block>>;
    using ExpressionVector = std::vector<std::shared_ptr<Expression>>;
    using StatementVector = std::vector<std::shared_ptr<Statement>>;
    using statement_ptr = Statement*;
    using expression_ptr = Expression*;
    using block_ptr = Block*;
    using identifier_ptr = Identifier*;
    using number_ptr = Number*;
    using string_ptr = String*;
    using integer_ptr = Integer*;
    using name_ptr = Name*;
    using float_ptr = Float*;
    using double_ptr = Double*;
    using boolean_ptr = Boolean*;
    using primename_ptr = PrimeName*;
    using varname_ptr = VarName*;
    using indexedname_ptr = IndexedName*;
    using argument_ptr = Argument*;
    using unit_ptr = Unit*;
    using reactvarname_ptr = ReactVarName*;
    using readionvar_ptr = ReadIonVar*;
    using writeionvar_ptr = WriteIonVar*;
    using nonspecurvar_ptr = NonspeCurVar*;
    using electrodecurvar_ptr = ElectrodeCurVar*;
    using sectionvar_ptr = SectionVar*;
    using rangevar_ptr = RangeVar*;
    using globalvar_ptr = GlobalVar*;
    using pointervar_ptr = PointerVar*;
    using bbcorepointervar_ptr = BbcorePointerVar*;
    using externvar_ptr = ExternVar*;
    using threadsafevar_ptr = ThreadsafeVar*;
    using paramblock_ptr = ParamBlock*;
    using paramassign_list = ParamAssignVector;
    using stepblock_ptr = StepBlock*;
    using stepped_list = SteppedVector;
    using independentblock_ptr = IndependentBlock*;
    using independentdef_list = IndependentDefVector;
    using dependentblock_ptr = DependentBlock*;
    using dependentdef_list = DependentDefVector;
    using stateblock_ptr = StateBlock*;
    using plotblock_ptr = PlotBlock*;
    using plotdeclaration_ptr = PlotDeclaration*;
    using initialblock_ptr = InitialBlock*;
    using statementblock_ptr = StatementBlock*;
    using constructorblock_ptr = ConstructorBlock*;
    using destructorblock_ptr = DestructorBlock*;
    using statement_list = StatementVector;
    using derivativeblock_ptr = DerivativeBlock*;
    using linearblock_ptr = LinearBlock*;
    using name_list = NameVector;
    using nonlinearblock_ptr = NonLinearBlock*;
    using discreteblock_ptr = DiscreteBlock*;
    using partialblock_ptr = PartialBlock*;
    using functiontableblock_ptr = FunctionTableBlock*;
    using argument_list = ArgumentVector;
    using functionblock_ptr = FunctionBlock*;
    using procedureblock_ptr = ProcedureBlock*;
    using netreceiveblock_ptr = NetReceiveBlock*;
    using solveblock_ptr = SolveBlock*;
    using breakpointblock_ptr = BreakpointBlock*;
    using terminalblock_ptr = TerminalBlock*;
    using beforeblock_ptr = BeforeBlock*;
    using bablock_ptr = BABlock*;
    using afterblock_ptr = AfterBlock*;
    using bablocktype_ptr = BABlockType*;
    using fornetcon_ptr = ForNetcon*;
    using kineticblock_ptr = KineticBlock*;
    using matchblock_ptr = MatchBlock*;
    using match_list = MatchVector;
    using unitblock_ptr = UnitBlock*;
    using expression_list = ExpressionVector;
    using constantblock_ptr = ConstantBlock*;
    using constantstatement_list = ConstantStatementVector;
    using neuronblock_ptr = NeuronBlock*;
    using doubleunit_ptr = DoubleUnit*;
    using localvariable_ptr = LocalVariable*;
    using limits_ptr = Limits*;
    using numberrange_ptr = NumberRange*;
    using plotvariable_ptr = PlotVariable*;
    using binaryoperator_ptr = BinaryOperator;
    using unaryoperator_ptr = UnaryOperator;
    using reactionoperator_ptr = ReactionOperator;
    using binaryexpression_ptr = BinaryExpression*;
    using unaryexpression_ptr = UnaryExpression*;
    using nonlineuation_ptr = NonLinEuation*;
    using linequation_ptr = LinEquation*;
    using functioncall_ptr = FunctionCall*;
    using firstlasttypeindex_ptr = FirstLastTypeIndex*;
    using watch_ptr = Watch*;
    using queueexpressiontype_ptr = QueueExpressionType*;
    using match_ptr = Match*;
    using unitdef_ptr = UnitDef*;
    using factordef_ptr = FactorDef*;
    using valence_ptr = Valence*;
    using unitstate_ptr = UnitState*;
    using localliststatement_ptr = LocalListStatement*;
    using localvariable_list = LocalVariableVector;
    using model_ptr = Model*;
    using define_ptr = Define*;
    using include_ptr = Include*;
    using paramassign_ptr = ParamAssign*;
    using stepped_ptr = Stepped*;
    using number_list = NumberVector;
    using independentdef_ptr = IndependentDef*;
    using dependentdef_ptr = DependentDef*;
    using plotvariable_list = PlotVariableVector;
    using conductancehint_ptr = ConductanceHint*;
    using expressionstatement_ptr = ExpressionStatement*;
    using protectstatement_ptr = ProtectStatement*;
    using fromstatement_ptr = FromStatement*;
    using forallstatement_ptr = ForAllStatement*;
    using whilestatement_ptr = WhileStatement*;
    using ifstatement_ptr = IfStatement*;
    using elseifstatement_list = ElseIfStatementVector;
    using elsestatement_ptr = ElseStatement*;
    using elseifstatement_ptr = ElseIfStatement*;
    using partialequation_ptr = PartialEquation*;
    using partialboundary_ptr = PartialBoundary*;
    using watchstatement_ptr = WatchStatement*;
    using watch_list = WatchVector;
    using mutexlock_ptr = MutexLock*;
    using mutexunlock_ptr = MutexUnlock*;
    using reset_ptr = Reset*;
    using sens_ptr = Sens*;
    using varname_list = VarNameVector;
    using conserve_ptr = Conserve*;
    using compartment_ptr = Compartment*;
    using ldifuse_ptr = LDifuse*;
    using reactionstatement_ptr = ReactionStatement*;
    using lagstatement_ptr = LagStatement*;
    using queuestatement_ptr = QueueStatement*;
    using constantstatement_ptr = ConstantStatement*;
    using tablestatement_ptr = TableStatement*;
    using nrnsuffix_ptr = NrnSuffix*;
    using nrnuseion_ptr = NrnUseion*;
    using readionvar_list = ReadIonVarVector;
    using writeionvar_list = WriteIonVarVector;
    using nrnnonspecific_ptr = NrnNonspecific*;
    using nonspecurvar_list = NonspeCurVarVector;
    using nrnelctrodecurrent_ptr = NrnElctrodeCurrent*;
    using electrodecurvar_list = ElectrodeCurVarVector;
    using nrnsection_ptr = NrnSection*;
    using sectionvar_list = SectionVarVector;
    using nrnrange_ptr = NrnRange*;
    using rangevar_list = RangeVarVector;
    using nrnglobal_ptr = NrnGlobal*;
    using globalvar_list = GlobalVarVector;
    using nrnpointer_ptr = NrnPointer*;
    using pointervar_list = PointerVarVector;
    using nrnbbcoreptr_ptr = NrnBbcorePtr*;
    using bbcorepointervar_list = BbcorePointerVarVector;
    using nrnexternal_ptr = NrnExternal*;
    using externvar_list = ExternVarVector;
    using nrnthreadsafe_ptr = NrnThreadSafe*;
    using threadsafevar_list = ThreadsafeVarVector;
    using verbatim_ptr = Verbatim*;
    using comment_ptr = Comment*;
    using program_ptr = Program*;
    using block_list = BlockVector;

    #include <visitors/visitor.hpp>

    /* Define all AST nodes */

    class Statement : public AST {
        public:
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitStatement(this); }
            virtual ~Statement() {}
            virtual std::string getType() { return "Statement"; }
            virtual Statement* clone() { return new Statement(*this); }
    };

    class Expression : public AST {
        public:
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitExpression(this); }
            virtual ~Expression() {}
            virtual std::string getType() { return "Expression"; }
            virtual Expression* clone() { return new Expression(*this); }
    };

    class Block : public Expression {
        public:
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitBlock(this); }
            virtual ~Block() {}
            virtual std::string getType() { return "Block"; }
            virtual Block* clone() { return new Block(*this); }
    };

    class Identifier : public Expression {
        public:
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitIdentifier(this); }
            virtual ~Identifier() {}
            virtual std::string getType() { return "Identifier"; }
            virtual Identifier* clone() { return new Identifier(*this); }
            virtual void setName(std::string name) { std::cout << "ERROR : setName() not implemented! "; abort(); }
    };

    class Number : public Expression {
        public:
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNumber(this); }
            virtual ~Number() {}
            virtual std::string getType() { return "Number"; }
            virtual Number* clone() { return new Number(*this); }
            virtual void negate() { std::cout << "ERROR : negate() not implemented! "; abort(); } 
    };

    class String : public Expression {
        public:
            /* member variables */
            std::string value;
            std::shared_ptr<ModToken> token;
            /* constructors */
            String(std::string value);
            String(const String& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitString(this); }
            virtual ~String() {}
            virtual std::string getType() { return "String"; }
            virtual String* clone() { return new String(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            std::string eval() { return value; }
            virtual void set(std::string _value) { value = _value; }
    };

    class Integer : public Number {
        public:
            /* member variables */
            int value;
            std::shared_ptr<Name> macroname;
            std::shared_ptr<ModToken> token;
            /* constructors */
            Integer(int value, Name* macroname);
            Integer(const Integer& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitInteger(this); }
            virtual ~Integer() {}
            virtual std::string getType() { return "Integer"; }
            virtual Integer* clone() { return new Integer(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void negate() { value = -value; }
            int eval() { return value; }
            virtual void set(int _value) { value = _value; }
    };

    class Float : public Number {
        public:
            /* member variables */
            float value;
            /* constructors */
            Float(float value);
            Float(const Float& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitFloat(this); }
            virtual ~Float() {}
            virtual std::string getType() { return "Float"; }
            virtual Float* clone() { return new Float(*this); }
            void negate() { value = -value; }
            float eval() { return value; }
            virtual void set(float _value) { value = _value; }
    };

    class Double : public Number {
        public:
            /* member variables */
            double value;
            std::shared_ptr<ModToken> token;
            /* constructors */
            Double(double value);
            Double(const Double& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitDouble(this); }
            virtual ~Double() {}
            virtual std::string getType() { return "Double"; }
            virtual Double* clone() { return new Double(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void negate() { value = -value; }
            double eval() { return value; }
            virtual void set(double _value) { value = _value; }
    };

    class Boolean : public Number {
        public:
            /* member variables */
            int value;
            /* constructors */
            Boolean(int value);
            Boolean(const Boolean& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitBoolean(this); }
            virtual ~Boolean() {}
            virtual std::string getType() { return "Boolean"; }
            virtual Boolean* clone() { return new Boolean(*this); }
            void negate() { value = !value; }
            bool eval() { return value; }
            virtual void set(bool _value) { value = _value; }
    };

    class Name : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<String> value;
            std::shared_ptr<ModToken> token;
            /* constructors */
            Name(String* value);
            Name(const Name& obj);

            virtual std::string getName() { return value->eval(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitName(this); }
            virtual ~Name() {}
            virtual std::string getType() { return "Name"; }
            virtual Name* clone() { return new Name(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            virtual void setName(std::string name) { value->set(name); }
    };

    class PrimeName : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<String> value;
            std::shared_ptr<Integer> order;
            std::shared_ptr<ModToken> token;
            /* constructors */
            PrimeName(String* value, Integer* order);
            PrimeName(const PrimeName& obj);

            virtual std::string getName() { return value->eval(); }
            virtual int getOrder() { return order->eval(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitPrimeName(this); }
            virtual ~PrimeName() {}
            virtual std::string getType() { return "PrimeName"; }
            virtual PrimeName* clone() { return new PrimeName(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
    };

    class VarName : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Integer> at_index;
            /* constructors */
            VarName(Identifier* name, Integer* at_index);
            VarName(const VarName& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitVarName(this); }
            virtual ~VarName() {}
            virtual std::string getType() { return "VarName"; }
            virtual VarName* clone() { return new VarName(*this); }
    };

    class IndexedName : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Expression> index;
            /* constructors */
            IndexedName(Identifier* name, Expression* index);
            IndexedName(const IndexedName& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitIndexedName(this); }
            virtual ~IndexedName() {}
            virtual std::string getType() { return "IndexedName"; }
            virtual IndexedName* clone() { return new IndexedName(*this); }
    };

    class Argument : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Unit> unit;
            /* constructors */
            Argument(Identifier* name, Unit* unit);
            Argument(const Argument& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitArgument(this); }
            virtual ~Argument() {}
            virtual std::string getType() { return "Argument"; }
            virtual Argument* clone() { return new Argument(*this); }
    };

    class ReactVarName : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Integer> value;
            std::shared_ptr<VarName> name;
            /* constructors */
            ReactVarName(Integer* value, VarName* name);
            ReactVarName(const ReactVarName& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitReactVarName(this); }
            virtual ~ReactVarName() {}
            virtual std::string getType() { return "ReactVarName"; }
            virtual ReactVarName* clone() { return new ReactVarName(*this); }
    };

    class ReadIonVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            ReadIonVar(Name* name);
            ReadIonVar(const ReadIonVar& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitReadIonVar(this); }
            virtual ~ReadIonVar() {}
            virtual std::string getType() { return "ReadIonVar"; }
            virtual ReadIonVar* clone() { return new ReadIonVar(*this); }
    };

    class WriteIonVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            WriteIonVar(Name* name);
            WriteIonVar(const WriteIonVar& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitWriteIonVar(this); }
            virtual ~WriteIonVar() {}
            virtual std::string getType() { return "WriteIonVar"; }
            virtual WriteIonVar* clone() { return new WriteIonVar(*this); }
    };

    class NonspeCurVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            NonspeCurVar(Name* name);
            NonspeCurVar(const NonspeCurVar& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNonspeCurVar(this); }
            virtual ~NonspeCurVar() {}
            virtual std::string getType() { return "NonspeCurVar"; }
            virtual NonspeCurVar* clone() { return new NonspeCurVar(*this); }
    };

    class ElectrodeCurVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            ElectrodeCurVar(Name* name);
            ElectrodeCurVar(const ElectrodeCurVar& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitElectrodeCurVar(this); }
            virtual ~ElectrodeCurVar() {}
            virtual std::string getType() { return "ElectrodeCurVar"; }
            virtual ElectrodeCurVar* clone() { return new ElectrodeCurVar(*this); }
    };

    class SectionVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            SectionVar(Name* name);
            SectionVar(const SectionVar& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitSectionVar(this); }
            virtual ~SectionVar() {}
            virtual std::string getType() { return "SectionVar"; }
            virtual SectionVar* clone() { return new SectionVar(*this); }
    };

    class RangeVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            RangeVar(Name* name);
            RangeVar(const RangeVar& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitRangeVar(this); }
            virtual ~RangeVar() {}
            virtual std::string getType() { return "RangeVar"; }
            virtual RangeVar* clone() { return new RangeVar(*this); }
    };

    class GlobalVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            GlobalVar(Name* name);
            GlobalVar(const GlobalVar& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitGlobalVar(this); }
            virtual ~GlobalVar() {}
            virtual std::string getType() { return "GlobalVar"; }
            virtual GlobalVar* clone() { return new GlobalVar(*this); }
    };

    class PointerVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            PointerVar(Name* name);
            PointerVar(const PointerVar& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitPointerVar(this); }
            virtual ~PointerVar() {}
            virtual std::string getType() { return "PointerVar"; }
            virtual PointerVar* clone() { return new PointerVar(*this); }
    };

    class BbcorePointerVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            BbcorePointerVar(Name* name);
            BbcorePointerVar(const BbcorePointerVar& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitBbcorePointerVar(this); }
            virtual ~BbcorePointerVar() {}
            virtual std::string getType() { return "BbcorePointerVar"; }
            virtual BbcorePointerVar* clone() { return new BbcorePointerVar(*this); }
    };

    class ExternVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            ExternVar(Name* name);
            ExternVar(const ExternVar& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitExternVar(this); }
            virtual ~ExternVar() {}
            virtual std::string getType() { return "ExternVar"; }
            virtual ExternVar* clone() { return new ExternVar(*this); }
    };

    class ThreadsafeVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            ThreadsafeVar(Name* name);
            ThreadsafeVar(const ThreadsafeVar& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitThreadsafeVar(this); }
            virtual ~ThreadsafeVar() {}
            virtual std::string getType() { return "ThreadsafeVar"; }
            virtual ThreadsafeVar* clone() { return new ThreadsafeVar(*this); }
    };

    class ParamBlock : public Block {
        public:
            /* member variables */
            ParamAssignVector statements;
            void* symtab = nullptr;

            /* constructors */
            ParamBlock(ParamAssignVector statements);
            ParamBlock(const ParamBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitParamBlock(this); }
            virtual ~ParamBlock() {}
            virtual std::string getType() { return "ParamBlock"; }
            virtual ParamBlock* clone() { return new ParamBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class StepBlock : public Block {
        public:
            /* member variables */
            SteppedVector statements;
            void* symtab = nullptr;

            /* constructors */
            StepBlock(SteppedVector statements);
            StepBlock(const StepBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitStepBlock(this); }
            virtual ~StepBlock() {}
            virtual std::string getType() { return "StepBlock"; }
            virtual StepBlock* clone() { return new StepBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class IndependentBlock : public Block {
        public:
            /* member variables */
            IndependentDefVector definitions;
            void* symtab = nullptr;

            /* constructors */
            IndependentBlock(IndependentDefVector definitions);
            IndependentBlock(const IndependentBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitIndependentBlock(this); }
            virtual ~IndependentBlock() {}
            virtual std::string getType() { return "IndependentBlock"; }
            virtual IndependentBlock* clone() { return new IndependentBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class DependentBlock : public Block {
        public:
            /* member variables */
            DependentDefVector definitions;
            void* symtab = nullptr;

            /* constructors */
            DependentBlock(DependentDefVector definitions);
            DependentBlock(const DependentBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitDependentBlock(this); }
            virtual ~DependentBlock() {}
            virtual std::string getType() { return "DependentBlock"; }
            virtual DependentBlock* clone() { return new DependentBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class StateBlock : public Block {
        public:
            /* member variables */
            DependentDefVector definitions;
            void* symtab = nullptr;

            /* constructors */
            StateBlock(DependentDefVector definitions);
            StateBlock(const StateBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitStateBlock(this); }
            virtual ~StateBlock() {}
            virtual std::string getType() { return "StateBlock"; }
            virtual StateBlock* clone() { return new StateBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class PlotBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<PlotDeclaration> plot;
            void* symtab = nullptr;

            /* constructors */
            PlotBlock(PlotDeclaration* plot);
            PlotBlock(const PlotBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitPlotBlock(this); }
            virtual ~PlotBlock() {}
            virtual std::string getType() { return "PlotBlock"; }
            virtual PlotBlock* clone() { return new PlotBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class InitialBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            InitialBlock(StatementBlock* statementblock);
            InitialBlock(const InitialBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitInitialBlock(this); }
            virtual ~InitialBlock() {}
            virtual std::string getType() { return "InitialBlock"; }
            virtual InitialBlock* clone() { return new InitialBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class ConstructorBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            ConstructorBlock(StatementBlock* statementblock);
            ConstructorBlock(const ConstructorBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitConstructorBlock(this); }
            virtual ~ConstructorBlock() {}
            virtual std::string getType() { return "ConstructorBlock"; }
            virtual ConstructorBlock* clone() { return new ConstructorBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class DestructorBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            DestructorBlock(StatementBlock* statementblock);
            DestructorBlock(const DestructorBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitDestructorBlock(this); }
            virtual ~DestructorBlock() {}
            virtual std::string getType() { return "DestructorBlock"; }
            virtual DestructorBlock* clone() { return new DestructorBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class StatementBlock : public Block {
        public:
            /* member variables */
            StatementVector statements;
            std::shared_ptr<ModToken> token;
            void* symtab = nullptr;

            /* constructors */
            StatementBlock(StatementVector statements);
            StatementBlock(const StatementBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitStatementBlock(this); }
            virtual ~StatementBlock() {}
            virtual std::string getType() { return "StatementBlock"; }
            virtual StatementBlock* clone() { return new StatementBlock(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class DerivativeBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<StatementBlock> statementblock;
            std::shared_ptr<ModToken> token;
            void* symtab = nullptr;

            /* constructors */
            DerivativeBlock(Name* name, StatementBlock* statementblock);
            DerivativeBlock(const DerivativeBlock& obj);

            virtual std::string getName() { return name->getName(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitDerivativeBlock(this); }
            virtual ~DerivativeBlock() {}
            virtual std::string getType() { return "DerivativeBlock"; }
            virtual DerivativeBlock* clone() { return new DerivativeBlock(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class LinearBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            NameVector solvefor;
            std::shared_ptr<StatementBlock> statementblock;
            std::shared_ptr<ModToken> token;
            void* symtab = nullptr;

            /* constructors */
            LinearBlock(Name* name, NameVector solvefor, StatementBlock* statementblock);
            LinearBlock(const LinearBlock& obj);

            virtual std::string getName() { return name->getName(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitLinearBlock(this); }
            virtual ~LinearBlock() {}
            virtual std::string getType() { return "LinearBlock"; }
            virtual LinearBlock* clone() { return new LinearBlock(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class NonLinearBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            NameVector solvefor;
            std::shared_ptr<StatementBlock> statementblock;
            std::shared_ptr<ModToken> token;
            void* symtab = nullptr;

            /* constructors */
            NonLinearBlock(Name* name, NameVector solvefor, StatementBlock* statementblock);
            NonLinearBlock(const NonLinearBlock& obj);

            virtual std::string getName() { return name->getName(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNonLinearBlock(this); }
            virtual ~NonLinearBlock() {}
            virtual std::string getType() { return "NonLinearBlock"; }
            virtual NonLinearBlock* clone() { return new NonLinearBlock(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class DiscreteBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<StatementBlock> statementblock;
            std::shared_ptr<ModToken> token;
            void* symtab = nullptr;

            /* constructors */
            DiscreteBlock(Name* name, StatementBlock* statementblock);
            DiscreteBlock(const DiscreteBlock& obj);

            virtual std::string getName() { return name->getName(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitDiscreteBlock(this); }
            virtual ~DiscreteBlock() {}
            virtual std::string getType() { return "DiscreteBlock"; }
            virtual DiscreteBlock* clone() { return new DiscreteBlock(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class PartialBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<StatementBlock> statementblock;
            std::shared_ptr<ModToken> token;
            void* symtab = nullptr;

            /* constructors */
            PartialBlock(Name* name, StatementBlock* statementblock);
            PartialBlock(const PartialBlock& obj);

            virtual std::string getName() { return name->getName(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitPartialBlock(this); }
            virtual ~PartialBlock() {}
            virtual std::string getType() { return "PartialBlock"; }
            virtual PartialBlock* clone() { return new PartialBlock(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class FunctionTableBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            ArgumentVector arguments;
            std::shared_ptr<Unit> unit;
            std::shared_ptr<ModToken> token;
            void* symtab = nullptr;

            /* constructors */
            FunctionTableBlock(Name* name, ArgumentVector arguments, Unit* unit);
            FunctionTableBlock(const FunctionTableBlock& obj);

            virtual std::string getName() { return name->getName(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitFunctionTableBlock(this); }
            virtual ~FunctionTableBlock() {}
            virtual std::string getType() { return "FunctionTableBlock"; }
            virtual FunctionTableBlock* clone() { return new FunctionTableBlock(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class FunctionBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            ArgumentVector arguments;
            std::shared_ptr<Unit> unit;
            std::shared_ptr<StatementBlock> statementblock;
            std::shared_ptr<ModToken> token;
            void* symtab = nullptr;

            /* constructors */
            FunctionBlock(Name* name, ArgumentVector arguments, Unit* unit, StatementBlock* statementblock);
            FunctionBlock(const FunctionBlock& obj);

            virtual std::string getName() { return name->getName(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitFunctionBlock(this); }
            virtual ~FunctionBlock() {}
            virtual std::string getType() { return "FunctionBlock"; }
            virtual FunctionBlock* clone() { return new FunctionBlock(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class ProcedureBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            ArgumentVector arguments;
            std::shared_ptr<Unit> unit;
            std::shared_ptr<StatementBlock> statementblock;
            std::shared_ptr<ModToken> token;
            void* symtab = nullptr;

            /* constructors */
            ProcedureBlock(Name* name, ArgumentVector arguments, Unit* unit, StatementBlock* statementblock);
            ProcedureBlock(const ProcedureBlock& obj);

            virtual std::string getName() { return name->getName(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitProcedureBlock(this); }
            virtual ~ProcedureBlock() {}
            virtual std::string getType() { return "ProcedureBlock"; }
            virtual ProcedureBlock* clone() { return new ProcedureBlock(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class NetReceiveBlock : public Block {
        public:
            /* member variables */
            ArgumentVector arguments;
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            NetReceiveBlock(ArgumentVector arguments, StatementBlock* statementblock);
            NetReceiveBlock(const NetReceiveBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNetReceiveBlock(this); }
            virtual ~NetReceiveBlock() {}
            virtual std::string getType() { return "NetReceiveBlock"; }
            virtual NetReceiveBlock* clone() { return new NetReceiveBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class SolveBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<Name> method;
            std::shared_ptr<StatementBlock> ifsolerr;
            void* symtab = nullptr;

            /* constructors */
            SolveBlock(Name* name, Name* method, StatementBlock* ifsolerr);
            SolveBlock(const SolveBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitSolveBlock(this); }
            virtual ~SolveBlock() {}
            virtual std::string getType() { return "SolveBlock"; }
            virtual SolveBlock* clone() { return new SolveBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class BreakpointBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            BreakpointBlock(StatementBlock* statementblock);
            BreakpointBlock(const BreakpointBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitBreakpointBlock(this); }
            virtual ~BreakpointBlock() {}
            virtual std::string getType() { return "BreakpointBlock"; }
            virtual BreakpointBlock* clone() { return new BreakpointBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class TerminalBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            TerminalBlock(StatementBlock* statementblock);
            TerminalBlock(const TerminalBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitTerminalBlock(this); }
            virtual ~TerminalBlock() {}
            virtual std::string getType() { return "TerminalBlock"; }
            virtual TerminalBlock* clone() { return new TerminalBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class BeforeBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<BABlock> block;
            void* symtab = nullptr;

            /* constructors */
            BeforeBlock(BABlock* block);
            BeforeBlock(const BeforeBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitBeforeBlock(this); }
            virtual ~BeforeBlock() {}
            virtual std::string getType() { return "BeforeBlock"; }
            virtual BeforeBlock* clone() { return new BeforeBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class AfterBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<BABlock> block;
            void* symtab = nullptr;

            /* constructors */
            AfterBlock(BABlock* block);
            AfterBlock(const AfterBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitAfterBlock(this); }
            virtual ~AfterBlock() {}
            virtual std::string getType() { return "AfterBlock"; }
            virtual AfterBlock* clone() { return new AfterBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class BABlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<BABlockType> type;
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            BABlock(BABlockType* type, StatementBlock* statementblock);
            BABlock(const BABlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitBABlock(this); }
            virtual ~BABlock() {}
            virtual std::string getType() { return "BABlock"; }
            virtual BABlock* clone() { return new BABlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class ForNetcon : public Block {
        public:
            /* member variables */
            ArgumentVector arguments;
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            ForNetcon(ArgumentVector arguments, StatementBlock* statementblock);
            ForNetcon(const ForNetcon& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitForNetcon(this); }
            virtual ~ForNetcon() {}
            virtual std::string getType() { return "ForNetcon"; }
            virtual ForNetcon* clone() { return new ForNetcon(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class KineticBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            NameVector solvefor;
            std::shared_ptr<StatementBlock> statementblock;
            std::shared_ptr<ModToken> token;
            void* symtab = nullptr;

            /* constructors */
            KineticBlock(Name* name, NameVector solvefor, StatementBlock* statementblock);
            KineticBlock(const KineticBlock& obj);

            virtual std::string getName() { return name->getName(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitKineticBlock(this); }
            virtual ~KineticBlock() {}
            virtual std::string getType() { return "KineticBlock"; }
            virtual KineticBlock* clone() { return new KineticBlock(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class MatchBlock : public Block {
        public:
            /* member variables */
            MatchVector matchs;
            void* symtab = nullptr;

            /* constructors */
            MatchBlock(MatchVector matchs);
            MatchBlock(const MatchBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitMatchBlock(this); }
            virtual ~MatchBlock() {}
            virtual std::string getType() { return "MatchBlock"; }
            virtual MatchBlock* clone() { return new MatchBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class UnitBlock : public Block {
        public:
            /* member variables */
            ExpressionVector definitions;
            void* symtab = nullptr;

            /* constructors */
            UnitBlock(ExpressionVector definitions);
            UnitBlock(const UnitBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitUnitBlock(this); }
            virtual ~UnitBlock() {}
            virtual std::string getType() { return "UnitBlock"; }
            virtual UnitBlock* clone() { return new UnitBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class ConstantBlock : public Block {
        public:
            /* member variables */
            ConstantStatementVector statements;
            void* symtab = nullptr;

            /* constructors */
            ConstantBlock(ConstantStatementVector statements);
            ConstantBlock(const ConstantBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitConstantBlock(this); }
            virtual ~ConstantBlock() {}
            virtual std::string getType() { return "ConstantBlock"; }
            virtual ConstantBlock* clone() { return new ConstantBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class NeuronBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            NeuronBlock(StatementBlock* statementblock);
            NeuronBlock(const NeuronBlock& obj);

            std::string getName() { return getType(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNeuronBlock(this); }
            virtual ~NeuronBlock() {}
            virtual std::string getType() { return "NeuronBlock"; }
            virtual NeuronBlock* clone() { return new NeuronBlock(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };

    class Unit : public Expression {
        public:
            /* member variables */
            std::shared_ptr<String> name;
            /* constructors */
            Unit(String* name);
            Unit(const Unit& obj);

            virtual std::string getName() { return name->eval(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitUnit(this); }
            virtual ~Unit() {}
            virtual std::string getType() { return "Unit"; }
            virtual Unit* clone() { return new Unit(*this); }
    };

    class DoubleUnit : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Double> values;
            std::shared_ptr<Unit> unit;
            /* constructors */
            DoubleUnit(Double* values, Unit* unit);
            DoubleUnit(const DoubleUnit& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitDoubleUnit(this); }
            virtual ~DoubleUnit() {}
            virtual std::string getType() { return "DoubleUnit"; }
            virtual DoubleUnit* clone() { return new DoubleUnit(*this); }
    };

    class LocalVariable : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            /* constructors */
            LocalVariable(Identifier* name);
            LocalVariable(const LocalVariable& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitLocalVariable(this); }
            virtual ~LocalVariable() {}
            virtual std::string getType() { return "LocalVariable"; }
            virtual LocalVariable* clone() { return new LocalVariable(*this); }
    };

    class Limits : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Double> min;
            std::shared_ptr<Double> max;
            /* constructors */
            Limits(Double* min, Double* max);
            Limits(const Limits& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitLimits(this); }
            virtual ~Limits() {}
            virtual std::string getType() { return "Limits"; }
            virtual Limits* clone() { return new Limits(*this); }
    };

    class NumberRange : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Number> min;
            std::shared_ptr<Number> max;
            /* constructors */
            NumberRange(Number* min, Number* max);
            NumberRange(const NumberRange& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNumberRange(this); }
            virtual ~NumberRange() {}
            virtual std::string getType() { return "NumberRange"; }
            virtual NumberRange* clone() { return new NumberRange(*this); }
    };

    class PlotVariable : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Integer> index;
            /* constructors */
            PlotVariable(Identifier* name, Integer* index);
            PlotVariable(const PlotVariable& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitPlotVariable(this); }
            virtual ~PlotVariable() {}
            virtual std::string getType() { return "PlotVariable"; }
            virtual PlotVariable* clone() { return new PlotVariable(*this); }
    };

    class BinaryOperator : public Expression {
        public:
            /* member variables */
            BinaryOp value;
            /* constructors */
            BinaryOperator(BinaryOp value);
            BinaryOperator(const BinaryOperator& obj);

            BinaryOperator() {}

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitBinaryOperator(this); }
            virtual ~BinaryOperator() {}
            virtual std::string getType() { return "BinaryOperator"; }
            virtual BinaryOperator* clone() { return new BinaryOperator(*this); }
            std::string  eval() { return BinaryOpNames[value]; }
    };

    class UnaryOperator : public Expression {
        public:
            /* member variables */
            UnaryOp value;
            /* constructors */
            UnaryOperator(UnaryOp value);
            UnaryOperator(const UnaryOperator& obj);

            UnaryOperator() {}

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitUnaryOperator(this); }
            virtual ~UnaryOperator() {}
            virtual std::string getType() { return "UnaryOperator"; }
            virtual UnaryOperator* clone() { return new UnaryOperator(*this); }
            std::string  eval() { return UnaryOpNames[value]; }
    };

    class ReactionOperator : public Expression {
        public:
            /* member variables */
            ReactionOp value;
            /* constructors */
            ReactionOperator(ReactionOp value);
            ReactionOperator(const ReactionOperator& obj);

            ReactionOperator() {}

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitReactionOperator(this); }
            virtual ~ReactionOperator() {}
            virtual std::string getType() { return "ReactionOperator"; }
            virtual ReactionOperator* clone() { return new ReactionOperator(*this); }
            std::string  eval() { return ReactionOpNames[value]; }
    };

    class BinaryExpression : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Expression> lhs;
            BinaryOperator op;
            std::shared_ptr<Expression> rhs;
            /* constructors */
            BinaryExpression(Expression* lhs, BinaryOperator op, Expression* rhs);
            BinaryExpression(const BinaryExpression& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitBinaryExpression(this); }
            virtual ~BinaryExpression() {}
            virtual std::string getType() { return "BinaryExpression"; }
            virtual BinaryExpression* clone() { return new BinaryExpression(*this); }
    };

    class UnaryExpression : public Expression {
        public:
            /* member variables */
            UnaryOperator op;
            std::shared_ptr<Expression> expression;
            /* constructors */
            UnaryExpression(UnaryOperator op, Expression* expression);
            UnaryExpression(const UnaryExpression& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitUnaryExpression(this); }
            virtual ~UnaryExpression() {}
            virtual std::string getType() { return "UnaryExpression"; }
            virtual UnaryExpression* clone() { return new UnaryExpression(*this); }
    };

    class NonLinEuation : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Expression> lhs;
            std::shared_ptr<Expression> rhs;
            /* constructors */
            NonLinEuation(Expression* lhs, Expression* rhs);
            NonLinEuation(const NonLinEuation& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNonLinEuation(this); }
            virtual ~NonLinEuation() {}
            virtual std::string getType() { return "NonLinEuation"; }
            virtual NonLinEuation* clone() { return new NonLinEuation(*this); }
    };

    class LinEquation : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Expression> leftlinexpr;
            std::shared_ptr<Expression> linexpr;
            /* constructors */
            LinEquation(Expression* leftlinexpr, Expression* linexpr);
            LinEquation(const LinEquation& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitLinEquation(this); }
            virtual ~LinEquation() {}
            virtual std::string getType() { return "LinEquation"; }
            virtual LinEquation* clone() { return new LinEquation(*this); }
    };

    class FunctionCall : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            ExpressionVector arguments;
            /* constructors */
            FunctionCall(Name* name, ExpressionVector arguments);
            FunctionCall(const FunctionCall& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitFunctionCall(this); }
            virtual ~FunctionCall() {}
            virtual std::string getType() { return "FunctionCall"; }
            virtual FunctionCall* clone() { return new FunctionCall(*this); }
    };

    class FirstLastTypeIndex : public Expression {
        public:
            /* member variables */
            FirstLastType value;
            /* constructors */
            FirstLastTypeIndex(FirstLastType value);
            FirstLastTypeIndex(const FirstLastTypeIndex& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitFirstLastTypeIndex(this); }
            virtual ~FirstLastTypeIndex() {}
            virtual std::string getType() { return "FirstLastTypeIndex"; }
            virtual FirstLastTypeIndex* clone() { return new FirstLastTypeIndex(*this); }
            std::string  eval() { return FirstLastTypeNames[value]; }
    };

    class Watch : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Expression> expression;
            std::shared_ptr<Expression> value;
            /* constructors */
            Watch(Expression* expression, Expression* value);
            Watch(const Watch& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitWatch(this); }
            virtual ~Watch() {}
            virtual std::string getType() { return "Watch"; }
            virtual Watch* clone() { return new Watch(*this); }
    };

    class QueueExpressionType : public Expression {
        public:
            /* member variables */
            QueueType value;
            /* constructors */
            QueueExpressionType(QueueType value);
            QueueExpressionType(const QueueExpressionType& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitQueueExpressionType(this); }
            virtual ~QueueExpressionType() {}
            virtual std::string getType() { return "QueueExpressionType"; }
            virtual QueueExpressionType* clone() { return new QueueExpressionType(*this); }
            std::string  eval() { return QueueTypeNames[value]; }
    };

    class Match : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Expression> expression;
            /* constructors */
            Match(Identifier* name, Expression* expression);
            Match(const Match& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitMatch(this); }
            virtual ~Match() {}
            virtual std::string getType() { return "Match"; }
            virtual Match* clone() { return new Match(*this); }
    };

    class BABlockType : public Expression {
        public:
            /* member variables */
            BAType value;
            /* constructors */
            BABlockType(BAType value);
            BABlockType(const BABlockType& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitBABlockType(this); }
            virtual ~BABlockType() {}
            virtual std::string getType() { return "BABlockType"; }
            virtual BABlockType* clone() { return new BABlockType(*this); }
            std::string  eval() { return BATypeNames[value]; }
    };

    class UnitDef : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Unit> unit1;
            std::shared_ptr<Unit> unit2;
            /* constructors */
            UnitDef(Unit* unit1, Unit* unit2);
            UnitDef(const UnitDef& obj);

            virtual std::string getName() { return unit1->getName(); }
            virtual ModToken* getToken() { return unit1->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitUnitDef(this); }
            virtual ~UnitDef() {}
            virtual std::string getType() { return "UnitDef"; }
            virtual UnitDef* clone() { return new UnitDef(*this); }
    };

    class FactorDef : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<Double> value;
            std::shared_ptr<Unit> unit1;
            std::shared_ptr<Boolean> gt;
            std::shared_ptr<Unit> unit2;
            std::shared_ptr<ModToken> token;
            /* constructors */
            FactorDef(Name* name, Double* value, Unit* unit1, Boolean* gt, Unit* unit2);
            FactorDef(const FactorDef& obj);

            virtual std::string getName() { return name->getName(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitFactorDef(this); }
            virtual ~FactorDef() {}
            virtual std::string getType() { return "FactorDef"; }
            virtual FactorDef* clone() { return new FactorDef(*this); }
            virtual ModToken* getToken() { return token.get(); }
            virtual void setToken(ModToken& tok) { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
    };

    class Valence : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Name> type;
            std::shared_ptr<Double> value;
            /* constructors */
            Valence(Name* type, Double* value);
            Valence(const Valence& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitValence(this); }
            virtual ~Valence() {}
            virtual std::string getType() { return "Valence"; }
            virtual Valence* clone() { return new Valence(*this); }
    };

    class UnitState : public Statement {
        public:
            /* member variables */
            UnitStateType value;
            /* constructors */
            UnitState(UnitStateType value);
            UnitState(const UnitState& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitUnitState(this); }
            virtual ~UnitState() {}
            virtual std::string getType() { return "UnitState"; }
            virtual UnitState* clone() { return new UnitState(*this); }
            std::string  eval() { return UnitStateTypeNames[value]; }
    };

    class LocalListStatement : public Statement {
        public:
            /* member variables */
            LocalVariableVector variables;
            /* constructors */
            LocalListStatement(LocalVariableVector variables);
            LocalListStatement(const LocalListStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitLocalListStatement(this); }
            virtual ~LocalListStatement() {}
            virtual std::string getType() { return "LocalListStatement"; }
            virtual LocalListStatement* clone() { return new LocalListStatement(*this); }
    };

    class Model : public Statement {
        public:
            /* member variables */
            std::shared_ptr<String> title;
            /* constructors */
            Model(String* title);
            Model(const Model& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitModel(this); }
            virtual ~Model() {}
            virtual std::string getType() { return "Model"; }
            virtual Model* clone() { return new Model(*this); }
    };

    class Define : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<Integer> value;
            /* constructors */
            Define(Name* name, Integer* value);
            Define(const Define& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitDefine(this); }
            virtual ~Define() {}
            virtual std::string getType() { return "Define"; }
            virtual Define* clone() { return new Define(*this); }
    };

    class Include : public Statement {
        public:
            /* member variables */
            std::shared_ptr<String> filename;
            /* constructors */
            Include(String* filename);
            Include(const Include& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitInclude(this); }
            virtual ~Include() {}
            virtual std::string getType() { return "Include"; }
            virtual Include* clone() { return new Include(*this); }
    };

    class ParamAssign : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Number> value;
            std::shared_ptr<Unit> unit;
            std::shared_ptr<Limits> limit;
            /* constructors */
            ParamAssign(Identifier* name, Number* value, Unit* unit, Limits* limit);
            ParamAssign(const ParamAssign& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitParamAssign(this); }
            virtual ~ParamAssign() {}
            virtual std::string getType() { return "ParamAssign"; }
            virtual ParamAssign* clone() { return new ParamAssign(*this); }
    };

    class Stepped : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            NumberVector values;
            std::shared_ptr<Unit> unit;
            /* constructors */
            Stepped(Name* name, NumberVector values, Unit* unit);
            Stepped(const Stepped& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitStepped(this); }
            virtual ~Stepped() {}
            virtual std::string getType() { return "Stepped"; }
            virtual Stepped* clone() { return new Stepped(*this); }
    };

    class IndependentDef : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Boolean> sweep;
            std::shared_ptr<Name> name;
            std::shared_ptr<Number> from;
            std::shared_ptr<Number> to;
            std::shared_ptr<Integer> with;
            std::shared_ptr<Number> opstart;
            std::shared_ptr<Unit> unit;
            /* constructors */
            IndependentDef(Boolean* sweep, Name* name, Number* from, Number* to, Integer* with, Number* opstart, Unit* unit);
            IndependentDef(const IndependentDef& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitIndependentDef(this); }
            virtual ~IndependentDef() {}
            virtual std::string getType() { return "IndependentDef"; }
            virtual IndependentDef* clone() { return new IndependentDef(*this); }
    };

    class DependentDef : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Integer> index;
            std::shared_ptr<Number> from;
            std::shared_ptr<Number> to;
            std::shared_ptr<Number> opstart;
            std::shared_ptr<Unit> unit;
            std::shared_ptr<Double> abstol;
            /* constructors */
            DependentDef(Identifier* name, Integer* index, Number* from, Number* to, Number* opstart, Unit* unit, Double* abstol);
            DependentDef(const DependentDef& obj);

            virtual std::string getName() { return name->getName(); }
            virtual ModToken* getToken() { return name->getToken(); }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitDependentDef(this); }
            virtual ~DependentDef() {}
            virtual std::string getType() { return "DependentDef"; }
            virtual DependentDef* clone() { return new DependentDef(*this); }
    };

    class PlotDeclaration : public Statement {
        public:
            /* member variables */
            PlotVariableVector pvlist;
            std::shared_ptr<PlotVariable> name;
            /* constructors */
            PlotDeclaration(PlotVariableVector pvlist, PlotVariable* name);
            PlotDeclaration(const PlotDeclaration& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitPlotDeclaration(this); }
            virtual ~PlotDeclaration() {}
            virtual std::string getType() { return "PlotDeclaration"; }
            virtual PlotDeclaration* clone() { return new PlotDeclaration(*this); }
    };

    class ConductanceHint : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> conductance;
            std::shared_ptr<Name> ion;
            /* constructors */
            ConductanceHint(Name* conductance, Name* ion);
            ConductanceHint(const ConductanceHint& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitConductanceHint(this); }
            virtual ~ConductanceHint() {}
            virtual std::string getType() { return "ConductanceHint"; }
            virtual ConductanceHint* clone() { return new ConductanceHint(*this); }
    };

    class ExpressionStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Expression> expression;
            /* constructors */
            ExpressionStatement(Expression* expression);
            ExpressionStatement(const ExpressionStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitExpressionStatement(this); }
            virtual ~ExpressionStatement() {}
            virtual std::string getType() { return "ExpressionStatement"; }
            virtual ExpressionStatement* clone() { return new ExpressionStatement(*this); }
    };

    class ProtectStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Expression> expression;
            /* constructors */
            ProtectStatement(Expression* expression);
            ProtectStatement(const ProtectStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitProtectStatement(this); }
            virtual ~ProtectStatement() {}
            virtual std::string getType() { return "ProtectStatement"; }
            virtual ProtectStatement* clone() { return new ProtectStatement(*this); }
    };

    class FromStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<Expression> from;
            std::shared_ptr<Expression> to;
            std::shared_ptr<Expression> opinc;
            std::shared_ptr<StatementBlock> statementblock;
            /* constructors */
            FromStatement(Name* name, Expression* from, Expression* to, Expression* opinc, StatementBlock* statementblock);
            FromStatement(const FromStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitFromStatement(this); }
            virtual ~FromStatement() {}
            virtual std::string getType() { return "FromStatement"; }
            virtual FromStatement* clone() { return new FromStatement(*this); }
    };

    class ForAllStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<StatementBlock> statementblock;
            /* constructors */
            ForAllStatement(Name* name, StatementBlock* statementblock);
            ForAllStatement(const ForAllStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitForAllStatement(this); }
            virtual ~ForAllStatement() {}
            virtual std::string getType() { return "ForAllStatement"; }
            virtual ForAllStatement* clone() { return new ForAllStatement(*this); }
    };

    class WhileStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Expression> condition;
            std::shared_ptr<StatementBlock> statementblock;
            /* constructors */
            WhileStatement(Expression* condition, StatementBlock* statementblock);
            WhileStatement(const WhileStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitWhileStatement(this); }
            virtual ~WhileStatement() {}
            virtual std::string getType() { return "WhileStatement"; }
            virtual WhileStatement* clone() { return new WhileStatement(*this); }
    };

    class IfStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Expression> condition;
            std::shared_ptr<StatementBlock> statementblock;
            ElseIfStatementVector elseifs;
            std::shared_ptr<ElseStatement> elses;
            /* constructors */
            IfStatement(Expression* condition, StatementBlock* statementblock, ElseIfStatementVector elseifs, ElseStatement* elses);
            IfStatement(const IfStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitIfStatement(this); }
            virtual ~IfStatement() {}
            virtual std::string getType() { return "IfStatement"; }
            virtual IfStatement* clone() { return new IfStatement(*this); }
    };

    class ElseIfStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Expression> condition;
            std::shared_ptr<StatementBlock> statementblock;
            /* constructors */
            ElseIfStatement(Expression* condition, StatementBlock* statementblock);
            ElseIfStatement(const ElseIfStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitElseIfStatement(this); }
            virtual ~ElseIfStatement() {}
            virtual std::string getType() { return "ElseIfStatement"; }
            virtual ElseIfStatement* clone() { return new ElseIfStatement(*this); }
    };

    class ElseStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            /* constructors */
            ElseStatement(StatementBlock* statementblock);
            ElseStatement(const ElseStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitElseStatement(this); }
            virtual ~ElseStatement() {}
            virtual std::string getType() { return "ElseStatement"; }
            virtual ElseStatement* clone() { return new ElseStatement(*this); }
    };

    class PartialEquation : public Statement {
        public:
            /* member variables */
            std::shared_ptr<PrimeName> prime;
            std::shared_ptr<Name> name1;
            std::shared_ptr<Name> name2;
            std::shared_ptr<Name> name3;
            /* constructors */
            PartialEquation(PrimeName* prime, Name* name1, Name* name2, Name* name3);
            PartialEquation(const PartialEquation& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitPartialEquation(this); }
            virtual ~PartialEquation() {}
            virtual std::string getType() { return "PartialEquation"; }
            virtual PartialEquation* clone() { return new PartialEquation(*this); }
    };

    class PartialBoundary : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> del;
            std::shared_ptr<Identifier> name;
            std::shared_ptr<FirstLastTypeIndex> index;
            std::shared_ptr<Expression> expression;
            std::shared_ptr<Name> name1;
            std::shared_ptr<Name> del2;
            std::shared_ptr<Name> name2;
            std::shared_ptr<Name> name3;
            /* constructors */
            PartialBoundary(Name* del, Identifier* name, FirstLastTypeIndex* index, Expression* expression, Name* name1, Name* del2, Name* name2, Name* name3);
            PartialBoundary(const PartialBoundary& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitPartialBoundary(this); }
            virtual ~PartialBoundary() {}
            virtual std::string getType() { return "PartialBoundary"; }
            virtual PartialBoundary* clone() { return new PartialBoundary(*this); }
    };

    class WatchStatement : public Statement {
        public:
            /* member variables */
            WatchVector statements;
            /* constructors */
            WatchStatement(WatchVector statements);
            WatchStatement(const WatchStatement& obj);

            void addWatch(Watch *s) {
                statements.push_back(std::shared_ptr<Watch>(s));
            }
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitWatchStatement(this); }
            virtual ~WatchStatement() {}
            virtual std::string getType() { return "WatchStatement"; }
            virtual WatchStatement* clone() { return new WatchStatement(*this); }
    };

    class MutexLock : public Statement {
        public:
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitMutexLock(this); }
            virtual ~MutexLock() {}
            virtual std::string getType() { return "MutexLock"; }
            virtual MutexLock* clone() { return new MutexLock(*this); }
    };

    class MutexUnlock : public Statement {
        public:
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitMutexUnlock(this); }
            virtual ~MutexUnlock() {}
            virtual std::string getType() { return "MutexUnlock"; }
            virtual MutexUnlock* clone() { return new MutexUnlock(*this); }
    };

    class Reset : public Statement {
        public:
            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitReset(this); }
            virtual ~Reset() {}
            virtual std::string getType() { return "Reset"; }
            virtual Reset* clone() { return new Reset(*this); }
    };

    class Sens : public Statement {
        public:
            /* member variables */
            VarNameVector senslist;
            /* constructors */
            Sens(VarNameVector senslist);
            Sens(const Sens& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitSens(this); }
            virtual ~Sens() {}
            virtual std::string getType() { return "Sens"; }
            virtual Sens* clone() { return new Sens(*this); }
    };

    class Conserve : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Expression> react;
            std::shared_ptr<Expression> expr;
            /* constructors */
            Conserve(Expression* react, Expression* expr);
            Conserve(const Conserve& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitConserve(this); }
            virtual ~Conserve() {}
            virtual std::string getType() { return "Conserve"; }
            virtual Conserve* clone() { return new Conserve(*this); }
    };

    class Compartment : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<Expression> expression;
            NameVector names;
            /* constructors */
            Compartment(Name* name, Expression* expression, NameVector names);
            Compartment(const Compartment& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitCompartment(this); }
            virtual ~Compartment() {}
            virtual std::string getType() { return "Compartment"; }
            virtual Compartment* clone() { return new Compartment(*this); }
    };

    class LDifuse : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<Expression> expression;
            NameVector names;
            /* constructors */
            LDifuse(Name* name, Expression* expression, NameVector names);
            LDifuse(const LDifuse& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitLDifuse(this); }
            virtual ~LDifuse() {}
            virtual std::string getType() { return "LDifuse"; }
            virtual LDifuse* clone() { return new LDifuse(*this); }
    };

    class ReactionStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Expression> react1;
            ReactionOperator op;
            std::shared_ptr<Expression> react2;
            std::shared_ptr<Expression> expr1;
            std::shared_ptr<Expression> expr2;
            /* constructors */
            ReactionStatement(Expression* react1, ReactionOperator op, Expression* react2, Expression* expr1, Expression* expr2);
            ReactionStatement(const ReactionStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitReactionStatement(this); }
            virtual ~ReactionStatement() {}
            virtual std::string getType() { return "ReactionStatement"; }
            virtual ReactionStatement* clone() { return new ReactionStatement(*this); }
    };

    class LagStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Name> byname;
            /* constructors */
            LagStatement(Identifier* name, Name* byname);
            LagStatement(const LagStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitLagStatement(this); }
            virtual ~LagStatement() {}
            virtual std::string getType() { return "LagStatement"; }
            virtual LagStatement* clone() { return new LagStatement(*this); }
    };

    class QueueStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<QueueExpressionType> qype;
            std::shared_ptr<Identifier> name;
            /* constructors */
            QueueStatement(QueueExpressionType* qype, Identifier* name);
            QueueStatement(const QueueStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitQueueStatement(this); }
            virtual ~QueueStatement() {}
            virtual std::string getType() { return "QueueStatement"; }
            virtual QueueStatement* clone() { return new QueueStatement(*this); }
    };

    class ConstantStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<Number> value;
            std::shared_ptr<Unit> unit;
            /* constructors */
            ConstantStatement(Name* name, Number* value, Unit* unit);
            ConstantStatement(const ConstantStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitConstantStatement(this); }
            virtual ~ConstantStatement() {}
            virtual std::string getType() { return "ConstantStatement"; }
            virtual ConstantStatement* clone() { return new ConstantStatement(*this); }
    };

    class TableStatement : public Statement {
        public:
            /* member variables */
            NameVector tablst;
            NameVector dependlst;
            std::shared_ptr<Expression> from;
            std::shared_ptr<Expression> to;
            std::shared_ptr<Integer> with;
            /* constructors */
            TableStatement(NameVector tablst, NameVector dependlst, Expression* from, Expression* to, Integer* with);
            TableStatement(const TableStatement& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitTableStatement(this); }
            virtual ~TableStatement() {}
            virtual std::string getType() { return "TableStatement"; }
            virtual TableStatement* clone() { return new TableStatement(*this); }
    };

    class NrnSuffix : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> type;
            std::shared_ptr<Name> name;
            /* constructors */
            NrnSuffix(Name* type, Name* name);
            NrnSuffix(const NrnSuffix& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNrnSuffix(this); }
            virtual ~NrnSuffix() {}
            virtual std::string getType() { return "NrnSuffix"; }
            virtual NrnSuffix* clone() { return new NrnSuffix(*this); }
    };

    class NrnUseion : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            ReadIonVarVector readlist;
            WriteIonVarVector writelist;
            std::shared_ptr<Valence> valence;
            /* constructors */
            NrnUseion(Name* name, ReadIonVarVector readlist, WriteIonVarVector writelist, Valence* valence);
            NrnUseion(const NrnUseion& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNrnUseion(this); }
            virtual ~NrnUseion() {}
            virtual std::string getType() { return "NrnUseion"; }
            virtual NrnUseion* clone() { return new NrnUseion(*this); }
    };

    class NrnNonspecific : public Statement {
        public:
            /* member variables */
            NonspeCurVarVector currents;
            /* constructors */
            NrnNonspecific(NonspeCurVarVector currents);
            NrnNonspecific(const NrnNonspecific& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNrnNonspecific(this); }
            virtual ~NrnNonspecific() {}
            virtual std::string getType() { return "NrnNonspecific"; }
            virtual NrnNonspecific* clone() { return new NrnNonspecific(*this); }
    };

    class NrnElctrodeCurrent : public Statement {
        public:
            /* member variables */
            ElectrodeCurVarVector ecurrents;
            /* constructors */
            NrnElctrodeCurrent(ElectrodeCurVarVector ecurrents);
            NrnElctrodeCurrent(const NrnElctrodeCurrent& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNrnElctrodeCurrent(this); }
            virtual ~NrnElctrodeCurrent() {}
            virtual std::string getType() { return "NrnElctrodeCurrent"; }
            virtual NrnElctrodeCurrent* clone() { return new NrnElctrodeCurrent(*this); }
    };

    class NrnSection : public Statement {
        public:
            /* member variables */
            SectionVarVector sections;
            /* constructors */
            NrnSection(SectionVarVector sections);
            NrnSection(const NrnSection& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNrnSection(this); }
            virtual ~NrnSection() {}
            virtual std::string getType() { return "NrnSection"; }
            virtual NrnSection* clone() { return new NrnSection(*this); }
    };

    class NrnRange : public Statement {
        public:
            /* member variables */
            RangeVarVector range_vars;
            /* constructors */
            NrnRange(RangeVarVector range_vars);
            NrnRange(const NrnRange& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNrnRange(this); }
            virtual ~NrnRange() {}
            virtual std::string getType() { return "NrnRange"; }
            virtual NrnRange* clone() { return new NrnRange(*this); }
    };

    class NrnGlobal : public Statement {
        public:
            /* member variables */
            GlobalVarVector global_vars;
            /* constructors */
            NrnGlobal(GlobalVarVector global_vars);
            NrnGlobal(const NrnGlobal& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNrnGlobal(this); }
            virtual ~NrnGlobal() {}
            virtual std::string getType() { return "NrnGlobal"; }
            virtual NrnGlobal* clone() { return new NrnGlobal(*this); }
    };

    class NrnPointer : public Statement {
        public:
            /* member variables */
            PointerVarVector pointers;
            /* constructors */
            NrnPointer(PointerVarVector pointers);
            NrnPointer(const NrnPointer& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNrnPointer(this); }
            virtual ~NrnPointer() {}
            virtual std::string getType() { return "NrnPointer"; }
            virtual NrnPointer* clone() { return new NrnPointer(*this); }
    };

    class NrnBbcorePtr : public Statement {
        public:
            /* member variables */
            BbcorePointerVarVector bbcore_pointers;
            /* constructors */
            NrnBbcorePtr(BbcorePointerVarVector bbcore_pointers);
            NrnBbcorePtr(const NrnBbcorePtr& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNrnBbcorePtr(this); }
            virtual ~NrnBbcorePtr() {}
            virtual std::string getType() { return "NrnBbcorePtr"; }
            virtual NrnBbcorePtr* clone() { return new NrnBbcorePtr(*this); }
    };

    class NrnExternal : public Statement {
        public:
            /* member variables */
            ExternVarVector externals;
            /* constructors */
            NrnExternal(ExternVarVector externals);
            NrnExternal(const NrnExternal& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNrnExternal(this); }
            virtual ~NrnExternal() {}
            virtual std::string getType() { return "NrnExternal"; }
            virtual NrnExternal* clone() { return new NrnExternal(*this); }
    };

    class NrnThreadSafe : public Statement {
        public:
            /* member variables */
            ThreadsafeVarVector threadsafe;
            /* constructors */
            NrnThreadSafe(ThreadsafeVarVector threadsafe);
            NrnThreadSafe(const NrnThreadSafe& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitNrnThreadSafe(this); }
            virtual ~NrnThreadSafe() {}
            virtual std::string getType() { return "NrnThreadSafe"; }
            virtual NrnThreadSafe* clone() { return new NrnThreadSafe(*this); }
    };

    class Verbatim : public Statement {
        public:
            /* member variables */
            std::shared_ptr<String> statement;
            /* constructors */
            Verbatim(String* statement);
            Verbatim(const Verbatim& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitVerbatim(this); }
            virtual ~Verbatim() {}
            virtual std::string getType() { return "Verbatim"; }
            virtual Verbatim* clone() { return new Verbatim(*this); }
    };

    class Comment : public Statement {
        public:
            /* member variables */
            std::shared_ptr<String> comment;
            /* constructors */
            Comment(String* comment);
            Comment(const Comment& obj);

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitComment(this); }
            virtual ~Comment() {}
            virtual std::string getType() { return "Comment"; }
            virtual Comment* clone() { return new Comment(*this); }
    };

    class Program : public  AST {
        public:
            /* member variables */
            StatementVector statements;
            BlockVector blocks;
            void* symtab = nullptr;

            /* constructors */
            Program(StatementVector statements, BlockVector blocks);
            Program(const Program& obj);

            void addStatement(Statement *s) {
                statements.push_back(std::shared_ptr<Statement>(s));
            }
            void addBlock(Block *s) {
                blocks.push_back(std::shared_ptr<Block>(s));
            }
            Program() {}

            virtual void visitChildren(Visitor* v);
            virtual void accept(Visitor* v) { v->visitProgram(this); }
            virtual ~Program() {}
            virtual std::string getType() { return "Program"; }
            virtual Program* clone() { return new Program(*this); }
            virtual void setBlockSymbolTable(void *s) { symtab = s; }
            virtual void* getBlockSymbolTable() { return symtab; }
    };



} // namespace ast
