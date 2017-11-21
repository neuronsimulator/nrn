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
            virtual ~Statement() {}

            virtual void visitChildren (Visitor* v) ;
            virtual void accept(Visitor* v)  { v->visitStatement(this); }
            virtual std::string getType()  { return "Statement"; }
            virtual Statement* clone()  { return new Statement(*this); }
    };

    class Expression : public AST {
        public:
            virtual ~Expression() {}

            virtual void visitChildren (Visitor* v) ;
            virtual void accept(Visitor* v)  { v->visitExpression(this); }
            virtual std::string getType()  { return "Expression"; }
            virtual Expression* clone()  { return new Expression(*this); }
    };

    class Block : public Expression {
        public:
            virtual ~Block() {}

            virtual void visitChildren (Visitor* v) ;
            virtual void accept(Visitor* v)  { v->visitBlock(this); }
            virtual std::string getType()  { return "Block"; }
            virtual Block* clone()  { return new Block(*this); }
    };

    class Identifier : public Expression {
        public:
            virtual ~Identifier() {}

            virtual void visitChildren (Visitor* v) ;
            virtual void accept(Visitor* v)  { v->visitIdentifier(this); }
            virtual std::string getType()  { return "Identifier"; }
            virtual Identifier* clone()  { return new Identifier(*this); }
            virtual void setName(std::string name) { std::cout << "ERROR : setName() not implemented! "; abort(); }
    };

    class Number : public Expression {
        public:
            virtual ~Number() {}

            virtual void visitChildren (Visitor* v) ;
            virtual void accept(Visitor* v)  { v->visitNumber(this); }
            virtual std::string getType()  { return "Number"; }
            virtual Number* clone()  { return new Number(*this); }
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
            virtual ~String() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitString(this); }
            std::string getType()  override { return "String"; }
            String* clone()  override { return new String(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            std::string eval() { return value; }
            void set(std::string _value)  { value = _value; }
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
            virtual ~Integer() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitInteger(this); }
            std::string getType()  override { return "Integer"; }
            Integer* clone()  override { return new Integer(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void negate() override { value = -value; }
            int eval() { return value; }
            void set(int _value)  { value = _value; }
    };

    class Float : public Number {
        public:
            /* member variables */
            float value;
            /* constructors */
            Float(float value);
            Float(const Float& obj);
            virtual ~Float() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitFloat(this); }
            std::string getType()  override { return "Float"; }
            Float* clone()  override { return new Float(*this); }
            void negate() override { value = -value; }
            float eval() { return value; }
            void set(float _value)  { value = _value; }
    };

    class Double : public Number {
        public:
            /* member variables */
            double value;
            std::shared_ptr<ModToken> token;
            /* constructors */
            Double(double value);
            Double(const Double& obj);
            virtual ~Double() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitDouble(this); }
            std::string getType()  override { return "Double"; }
            Double* clone()  override { return new Double(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void negate() override { value = -value; }
            double eval() { return value; }
            void set(double _value)  { value = _value; }
    };

    class Boolean : public Number {
        public:
            /* member variables */
            int value;
            /* constructors */
            Boolean(int value);
            Boolean(const Boolean& obj);
            virtual ~Boolean() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitBoolean(this); }
            std::string getType()  override { return "Boolean"; }
            Boolean* clone()  override { return new Boolean(*this); }
            void negate() override { value = !value; }
            bool eval() { return value; }
            void set(bool _value)  { value = _value; }
    };

    class Name : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<String> value;
            std::shared_ptr<ModToken> token;
            /* constructors */
            Name(String* value);
            Name(const Name& obj);
            virtual ~Name() {}

            std::string getName() override { return value->eval(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitName(this); }
            std::string getType()  override { return "Name"; }
            Name* clone()  override { return new Name(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void setName(std::string name) override { value->set(name); }
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
            virtual ~PrimeName() {}

            std::string getName() override { return value->eval(); }
            int getOrder()  { return order->eval(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitPrimeName(this); }
            std::string getType()  override { return "PrimeName"; }
            PrimeName* clone()  override { return new PrimeName(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
    };

    class VarName : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Integer> at_index;
            /* constructors */
            VarName(Identifier* name, Integer* at_index);
            VarName(const VarName& obj);
            virtual ~VarName() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitVarName(this); }
            std::string getType()  override { return "VarName"; }
            VarName* clone()  override { return new VarName(*this); }
    };

    class IndexedName : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Expression> index;
            /* constructors */
            IndexedName(Identifier* name, Expression* index);
            IndexedName(const IndexedName& obj);
            virtual ~IndexedName() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitIndexedName(this); }
            std::string getType()  override { return "IndexedName"; }
            IndexedName* clone()  override { return new IndexedName(*this); }
    };

    class Argument : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Unit> unit;
            /* constructors */
            Argument(Identifier* name, Unit* unit);
            Argument(const Argument& obj);
            virtual ~Argument() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitArgument(this); }
            std::string getType()  override { return "Argument"; }
            Argument* clone()  override { return new Argument(*this); }
    };

    class ReactVarName : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Integer> value;
            std::shared_ptr<VarName> name;
            /* constructors */
            ReactVarName(Integer* value, VarName* name);
            ReactVarName(const ReactVarName& obj);
            virtual ~ReactVarName() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitReactVarName(this); }
            std::string getType()  override { return "ReactVarName"; }
            ReactVarName* clone()  override { return new ReactVarName(*this); }
    };

    class ReadIonVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            ReadIonVar(Name* name);
            ReadIonVar(const ReadIonVar& obj);
            virtual ~ReadIonVar() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitReadIonVar(this); }
            std::string getType()  override { return "ReadIonVar"; }
            ReadIonVar* clone()  override { return new ReadIonVar(*this); }
    };

    class WriteIonVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            WriteIonVar(Name* name);
            WriteIonVar(const WriteIonVar& obj);
            virtual ~WriteIonVar() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitWriteIonVar(this); }
            std::string getType()  override { return "WriteIonVar"; }
            WriteIonVar* clone()  override { return new WriteIonVar(*this); }
    };

    class NonspeCurVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            NonspeCurVar(Name* name);
            NonspeCurVar(const NonspeCurVar& obj);
            virtual ~NonspeCurVar() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNonspeCurVar(this); }
            std::string getType()  override { return "NonspeCurVar"; }
            NonspeCurVar* clone()  override { return new NonspeCurVar(*this); }
    };

    class ElectrodeCurVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            ElectrodeCurVar(Name* name);
            ElectrodeCurVar(const ElectrodeCurVar& obj);
            virtual ~ElectrodeCurVar() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitElectrodeCurVar(this); }
            std::string getType()  override { return "ElectrodeCurVar"; }
            ElectrodeCurVar* clone()  override { return new ElectrodeCurVar(*this); }
    };

    class SectionVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            SectionVar(Name* name);
            SectionVar(const SectionVar& obj);
            virtual ~SectionVar() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitSectionVar(this); }
            std::string getType()  override { return "SectionVar"; }
            SectionVar* clone()  override { return new SectionVar(*this); }
    };

    class RangeVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            RangeVar(Name* name);
            RangeVar(const RangeVar& obj);
            virtual ~RangeVar() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitRangeVar(this); }
            std::string getType()  override { return "RangeVar"; }
            RangeVar* clone()  override { return new RangeVar(*this); }
    };

    class GlobalVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            GlobalVar(Name* name);
            GlobalVar(const GlobalVar& obj);
            virtual ~GlobalVar() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitGlobalVar(this); }
            std::string getType()  override { return "GlobalVar"; }
            GlobalVar* clone()  override { return new GlobalVar(*this); }
    };

    class PointerVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            PointerVar(Name* name);
            PointerVar(const PointerVar& obj);
            virtual ~PointerVar() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitPointerVar(this); }
            std::string getType()  override { return "PointerVar"; }
            PointerVar* clone()  override { return new PointerVar(*this); }
    };

    class BbcorePointerVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            BbcorePointerVar(Name* name);
            BbcorePointerVar(const BbcorePointerVar& obj);
            virtual ~BbcorePointerVar() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitBbcorePointerVar(this); }
            std::string getType()  override { return "BbcorePointerVar"; }
            BbcorePointerVar* clone()  override { return new BbcorePointerVar(*this); }
    };

    class ExternVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            ExternVar(Name* name);
            ExternVar(const ExternVar& obj);
            virtual ~ExternVar() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitExternVar(this); }
            std::string getType()  override { return "ExternVar"; }
            ExternVar* clone()  override { return new ExternVar(*this); }
    };

    class ThreadsafeVar : public Identifier {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            /* constructors */
            ThreadsafeVar(Name* name);
            ThreadsafeVar(const ThreadsafeVar& obj);
            virtual ~ThreadsafeVar() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitThreadsafeVar(this); }
            std::string getType()  override { return "ThreadsafeVar"; }
            ThreadsafeVar* clone()  override { return new ThreadsafeVar(*this); }
    };

    class ParamBlock : public Block {
        public:
            /* member variables */
            ParamAssignVector statements;
            void* symtab = nullptr;

            /* constructors */
            ParamBlock(ParamAssignVector statements);
            ParamBlock(const ParamBlock& obj);
            virtual ~ParamBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitParamBlock(this); }
            std::string getType()  override { return "ParamBlock"; }
            ParamBlock* clone()  override { return new ParamBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class StepBlock : public Block {
        public:
            /* member variables */
            SteppedVector statements;
            void* symtab = nullptr;

            /* constructors */
            StepBlock(SteppedVector statements);
            StepBlock(const StepBlock& obj);
            virtual ~StepBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitStepBlock(this); }
            std::string getType()  override { return "StepBlock"; }
            StepBlock* clone()  override { return new StepBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class IndependentBlock : public Block {
        public:
            /* member variables */
            IndependentDefVector definitions;
            void* symtab = nullptr;

            /* constructors */
            IndependentBlock(IndependentDefVector definitions);
            IndependentBlock(const IndependentBlock& obj);
            virtual ~IndependentBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitIndependentBlock(this); }
            std::string getType()  override { return "IndependentBlock"; }
            IndependentBlock* clone()  override { return new IndependentBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class DependentBlock : public Block {
        public:
            /* member variables */
            DependentDefVector definitions;
            void* symtab = nullptr;

            /* constructors */
            DependentBlock(DependentDefVector definitions);
            DependentBlock(const DependentBlock& obj);
            virtual ~DependentBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitDependentBlock(this); }
            std::string getType()  override { return "DependentBlock"; }
            DependentBlock* clone()  override { return new DependentBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class StateBlock : public Block {
        public:
            /* member variables */
            DependentDefVector definitions;
            void* symtab = nullptr;

            /* constructors */
            StateBlock(DependentDefVector definitions);
            StateBlock(const StateBlock& obj);
            virtual ~StateBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitStateBlock(this); }
            std::string getType()  override { return "StateBlock"; }
            StateBlock* clone()  override { return new StateBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class PlotBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<PlotDeclaration> plot;
            void* symtab = nullptr;

            /* constructors */
            PlotBlock(PlotDeclaration* plot);
            PlotBlock(const PlotBlock& obj);
            virtual ~PlotBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitPlotBlock(this); }
            std::string getType()  override { return "PlotBlock"; }
            PlotBlock* clone()  override { return new PlotBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class InitialBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            InitialBlock(StatementBlock* statementblock);
            InitialBlock(const InitialBlock& obj);
            virtual ~InitialBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitInitialBlock(this); }
            std::string getType()  override { return "InitialBlock"; }
            InitialBlock* clone()  override { return new InitialBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class ConstructorBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            ConstructorBlock(StatementBlock* statementblock);
            ConstructorBlock(const ConstructorBlock& obj);
            virtual ~ConstructorBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitConstructorBlock(this); }
            std::string getType()  override { return "ConstructorBlock"; }
            ConstructorBlock* clone()  override { return new ConstructorBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class DestructorBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            DestructorBlock(StatementBlock* statementblock);
            DestructorBlock(const DestructorBlock& obj);
            virtual ~DestructorBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitDestructorBlock(this); }
            std::string getType()  override { return "DestructorBlock"; }
            DestructorBlock* clone()  override { return new DestructorBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~StatementBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitStatementBlock(this); }
            std::string getType()  override { return "StatementBlock"; }
            StatementBlock* clone()  override { return new StatementBlock(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~DerivativeBlock() {}

            std::string getName() override { return name->getName(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitDerivativeBlock(this); }
            std::string getType()  override { return "DerivativeBlock"; }
            DerivativeBlock* clone()  override { return new DerivativeBlock(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~LinearBlock() {}

            std::string getName() override { return name->getName(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitLinearBlock(this); }
            std::string getType()  override { return "LinearBlock"; }
            LinearBlock* clone()  override { return new LinearBlock(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~NonLinearBlock() {}

            std::string getName() override { return name->getName(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNonLinearBlock(this); }
            std::string getType()  override { return "NonLinearBlock"; }
            NonLinearBlock* clone()  override { return new NonLinearBlock(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~DiscreteBlock() {}

            std::string getName() override { return name->getName(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitDiscreteBlock(this); }
            std::string getType()  override { return "DiscreteBlock"; }
            DiscreteBlock* clone()  override { return new DiscreteBlock(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~PartialBlock() {}

            std::string getName() override { return name->getName(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitPartialBlock(this); }
            std::string getType()  override { return "PartialBlock"; }
            PartialBlock* clone()  override { return new PartialBlock(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~FunctionTableBlock() {}

            std::string getName() override { return name->getName(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitFunctionTableBlock(this); }
            std::string getType()  override { return "FunctionTableBlock"; }
            FunctionTableBlock* clone()  override { return new FunctionTableBlock(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~FunctionBlock() {}

            std::string getName() override { return name->getName(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitFunctionBlock(this); }
            std::string getType()  override { return "FunctionBlock"; }
            FunctionBlock* clone()  override { return new FunctionBlock(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~ProcedureBlock() {}

            std::string getName() override { return name->getName(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitProcedureBlock(this); }
            std::string getType()  override { return "ProcedureBlock"; }
            ProcedureBlock* clone()  override { return new ProcedureBlock(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~NetReceiveBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNetReceiveBlock(this); }
            std::string getType()  override { return "NetReceiveBlock"; }
            NetReceiveBlock* clone()  override { return new NetReceiveBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~SolveBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitSolveBlock(this); }
            std::string getType()  override { return "SolveBlock"; }
            SolveBlock* clone()  override { return new SolveBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class BreakpointBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            BreakpointBlock(StatementBlock* statementblock);
            BreakpointBlock(const BreakpointBlock& obj);
            virtual ~BreakpointBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitBreakpointBlock(this); }
            std::string getType()  override { return "BreakpointBlock"; }
            BreakpointBlock* clone()  override { return new BreakpointBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class TerminalBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            TerminalBlock(StatementBlock* statementblock);
            TerminalBlock(const TerminalBlock& obj);
            virtual ~TerminalBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitTerminalBlock(this); }
            std::string getType()  override { return "TerminalBlock"; }
            TerminalBlock* clone()  override { return new TerminalBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class BeforeBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<BABlock> block;
            void* symtab = nullptr;

            /* constructors */
            BeforeBlock(BABlock* block);
            BeforeBlock(const BeforeBlock& obj);
            virtual ~BeforeBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitBeforeBlock(this); }
            std::string getType()  override { return "BeforeBlock"; }
            BeforeBlock* clone()  override { return new BeforeBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class AfterBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<BABlock> block;
            void* symtab = nullptr;

            /* constructors */
            AfterBlock(BABlock* block);
            AfterBlock(const AfterBlock& obj);
            virtual ~AfterBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitAfterBlock(this); }
            std::string getType()  override { return "AfterBlock"; }
            AfterBlock* clone()  override { return new AfterBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~BABlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitBABlock(this); }
            std::string getType()  override { return "BABlock"; }
            BABlock* clone()  override { return new BABlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~ForNetcon() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitForNetcon(this); }
            std::string getType()  override { return "ForNetcon"; }
            ForNetcon* clone()  override { return new ForNetcon(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
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
            virtual ~KineticBlock() {}

            std::string getName() override { return name->getName(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitKineticBlock(this); }
            std::string getType()  override { return "KineticBlock"; }
            KineticBlock* clone()  override { return new KineticBlock(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class MatchBlock : public Block {
        public:
            /* member variables */
            MatchVector matchs;
            void* symtab = nullptr;

            /* constructors */
            MatchBlock(MatchVector matchs);
            MatchBlock(const MatchBlock& obj);
            virtual ~MatchBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitMatchBlock(this); }
            std::string getType()  override { return "MatchBlock"; }
            MatchBlock* clone()  override { return new MatchBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class UnitBlock : public Block {
        public:
            /* member variables */
            ExpressionVector definitions;
            void* symtab = nullptr;

            /* constructors */
            UnitBlock(ExpressionVector definitions);
            UnitBlock(const UnitBlock& obj);
            virtual ~UnitBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitUnitBlock(this); }
            std::string getType()  override { return "UnitBlock"; }
            UnitBlock* clone()  override { return new UnitBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class ConstantBlock : public Block {
        public:
            /* member variables */
            ConstantStatementVector statements;
            void* symtab = nullptr;

            /* constructors */
            ConstantBlock(ConstantStatementVector statements);
            ConstantBlock(const ConstantBlock& obj);
            virtual ~ConstantBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitConstantBlock(this); }
            std::string getType()  override { return "ConstantBlock"; }
            ConstantBlock* clone()  override { return new ConstantBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class NeuronBlock : public Block {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            void* symtab = nullptr;

            /* constructors */
            NeuronBlock(StatementBlock* statementblock);
            NeuronBlock(const NeuronBlock& obj);
            virtual ~NeuronBlock() {}

            std::string getName() override { return getType(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNeuronBlock(this); }
            std::string getType()  override { return "NeuronBlock"; }
            NeuronBlock* clone()  override { return new NeuronBlock(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };

    class Unit : public Expression {
        public:
            /* member variables */
            std::shared_ptr<String> name;
            /* constructors */
            Unit(String* name);
            Unit(const Unit& obj);
            virtual ~Unit() {}

            std::string getName() override { return name->eval(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitUnit(this); }
            std::string getType()  override { return "Unit"; }
            Unit* clone()  override { return new Unit(*this); }
    };

    class DoubleUnit : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Double> values;
            std::shared_ptr<Unit> unit;
            /* constructors */
            DoubleUnit(Double* values, Unit* unit);
            DoubleUnit(const DoubleUnit& obj);
            virtual ~DoubleUnit() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitDoubleUnit(this); }
            std::string getType()  override { return "DoubleUnit"; }
            DoubleUnit* clone()  override { return new DoubleUnit(*this); }
    };

    class LocalVariable : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            /* constructors */
            LocalVariable(Identifier* name);
            LocalVariable(const LocalVariable& obj);
            virtual ~LocalVariable() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitLocalVariable(this); }
            std::string getType()  override { return "LocalVariable"; }
            LocalVariable* clone()  override { return new LocalVariable(*this); }
    };

    class Limits : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Double> min;
            std::shared_ptr<Double> max;
            /* constructors */
            Limits(Double* min, Double* max);
            Limits(const Limits& obj);
            virtual ~Limits() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitLimits(this); }
            std::string getType()  override { return "Limits"; }
            Limits* clone()  override { return new Limits(*this); }
    };

    class NumberRange : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Number> min;
            std::shared_ptr<Number> max;
            /* constructors */
            NumberRange(Number* min, Number* max);
            NumberRange(const NumberRange& obj);
            virtual ~NumberRange() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNumberRange(this); }
            std::string getType()  override { return "NumberRange"; }
            NumberRange* clone()  override { return new NumberRange(*this); }
    };

    class PlotVariable : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Integer> index;
            /* constructors */
            PlotVariable(Identifier* name, Integer* index);
            PlotVariable(const PlotVariable& obj);
            virtual ~PlotVariable() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitPlotVariable(this); }
            std::string getType()  override { return "PlotVariable"; }
            PlotVariable* clone()  override { return new PlotVariable(*this); }
    };

    class BinaryOperator : public Expression {
        public:
            /* member variables */
            BinaryOp value;
            /* constructors */
            BinaryOperator(BinaryOp value);
            BinaryOperator(const BinaryOperator& obj);
            virtual ~BinaryOperator() {}

            BinaryOperator() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitBinaryOperator(this); }
            std::string getType()  override { return "BinaryOperator"; }
            BinaryOperator* clone()  override { return new BinaryOperator(*this); }
            std::string  eval() { return BinaryOpNames[value]; }
    };

    class UnaryOperator : public Expression {
        public:
            /* member variables */
            UnaryOp value;
            /* constructors */
            UnaryOperator(UnaryOp value);
            UnaryOperator(const UnaryOperator& obj);
            virtual ~UnaryOperator() {}

            UnaryOperator() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitUnaryOperator(this); }
            std::string getType()  override { return "UnaryOperator"; }
            UnaryOperator* clone()  override { return new UnaryOperator(*this); }
            std::string  eval() { return UnaryOpNames[value]; }
    };

    class ReactionOperator : public Expression {
        public:
            /* member variables */
            ReactionOp value;
            /* constructors */
            ReactionOperator(ReactionOp value);
            ReactionOperator(const ReactionOperator& obj);
            virtual ~ReactionOperator() {}

            ReactionOperator() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitReactionOperator(this); }
            std::string getType()  override { return "ReactionOperator"; }
            ReactionOperator* clone()  override { return new ReactionOperator(*this); }
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
            virtual ~BinaryExpression() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitBinaryExpression(this); }
            std::string getType()  override { return "BinaryExpression"; }
            BinaryExpression* clone()  override { return new BinaryExpression(*this); }
    };

    class UnaryExpression : public Expression {
        public:
            /* member variables */
            UnaryOperator op;
            std::shared_ptr<Expression> expression;
            /* constructors */
            UnaryExpression(UnaryOperator op, Expression* expression);
            UnaryExpression(const UnaryExpression& obj);
            virtual ~UnaryExpression() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitUnaryExpression(this); }
            std::string getType()  override { return "UnaryExpression"; }
            UnaryExpression* clone()  override { return new UnaryExpression(*this); }
    };

    class NonLinEuation : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Expression> lhs;
            std::shared_ptr<Expression> rhs;
            /* constructors */
            NonLinEuation(Expression* lhs, Expression* rhs);
            NonLinEuation(const NonLinEuation& obj);
            virtual ~NonLinEuation() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNonLinEuation(this); }
            std::string getType()  override { return "NonLinEuation"; }
            NonLinEuation* clone()  override { return new NonLinEuation(*this); }
    };

    class LinEquation : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Expression> leftlinexpr;
            std::shared_ptr<Expression> linexpr;
            /* constructors */
            LinEquation(Expression* leftlinexpr, Expression* linexpr);
            LinEquation(const LinEquation& obj);
            virtual ~LinEquation() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitLinEquation(this); }
            std::string getType()  override { return "LinEquation"; }
            LinEquation* clone()  override { return new LinEquation(*this); }
    };

    class FunctionCall : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            ExpressionVector arguments;
            /* constructors */
            FunctionCall(Name* name, ExpressionVector arguments);
            FunctionCall(const FunctionCall& obj);
            virtual ~FunctionCall() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitFunctionCall(this); }
            std::string getType()  override { return "FunctionCall"; }
            FunctionCall* clone()  override { return new FunctionCall(*this); }
    };

    class FirstLastTypeIndex : public Expression {
        public:
            /* member variables */
            FirstLastType value;
            /* constructors */
            FirstLastTypeIndex(FirstLastType value);
            FirstLastTypeIndex(const FirstLastTypeIndex& obj);
            virtual ~FirstLastTypeIndex() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitFirstLastTypeIndex(this); }
            std::string getType()  override { return "FirstLastTypeIndex"; }
            FirstLastTypeIndex* clone()  override { return new FirstLastTypeIndex(*this); }
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
            virtual ~Watch() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitWatch(this); }
            std::string getType()  override { return "Watch"; }
            Watch* clone()  override { return new Watch(*this); }
    };

    class QueueExpressionType : public Expression {
        public:
            /* member variables */
            QueueType value;
            /* constructors */
            QueueExpressionType(QueueType value);
            QueueExpressionType(const QueueExpressionType& obj);
            virtual ~QueueExpressionType() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitQueueExpressionType(this); }
            std::string getType()  override { return "QueueExpressionType"; }
            QueueExpressionType* clone()  override { return new QueueExpressionType(*this); }
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
            virtual ~Match() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitMatch(this); }
            std::string getType()  override { return "Match"; }
            Match* clone()  override { return new Match(*this); }
    };

    class BABlockType : public Expression {
        public:
            /* member variables */
            BAType value;
            /* constructors */
            BABlockType(BAType value);
            BABlockType(const BABlockType& obj);
            virtual ~BABlockType() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitBABlockType(this); }
            std::string getType()  override { return "BABlockType"; }
            BABlockType* clone()  override { return new BABlockType(*this); }
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
            virtual ~UnitDef() {}

            std::string getName() override { return unit1->getName(); }
            ModToken* getToken() override { return unit1->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitUnitDef(this); }
            std::string getType()  override { return "UnitDef"; }
            UnitDef* clone()  override { return new UnitDef(*this); }
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
            virtual ~FactorDef() {}

            std::string getName() override { return name->getName(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitFactorDef(this); }
            std::string getType()  override { return "FactorDef"; }
            FactorDef* clone()  override { return new FactorDef(*this); }
            ModToken* getToken()  override { return token.get(); }
            void setToken(ModToken& tok)  { token = std::shared_ptr<ModToken>(new ModToken(tok)); }
    };

    class Valence : public Expression {
        public:
            /* member variables */
            std::shared_ptr<Name> type;
            std::shared_ptr<Double> value;
            /* constructors */
            Valence(Name* type, Double* value);
            Valence(const Valence& obj);
            virtual ~Valence() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitValence(this); }
            std::string getType()  override { return "Valence"; }
            Valence* clone()  override { return new Valence(*this); }
    };

    class UnitState : public Statement {
        public:
            /* member variables */
            UnitStateType value;
            /* constructors */
            UnitState(UnitStateType value);
            UnitState(const UnitState& obj);
            virtual ~UnitState() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitUnitState(this); }
            std::string getType()  override { return "UnitState"; }
            UnitState* clone()  override { return new UnitState(*this); }
            std::string  eval() { return UnitStateTypeNames[value]; }
    };

    class LocalListStatement : public Statement {
        public:
            /* member variables */
            LocalVariableVector variables;
            /* constructors */
            LocalListStatement(LocalVariableVector variables);
            LocalListStatement(const LocalListStatement& obj);
            virtual ~LocalListStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitLocalListStatement(this); }
            std::string getType()  override { return "LocalListStatement"; }
            LocalListStatement* clone()  override { return new LocalListStatement(*this); }
    };

    class Model : public Statement {
        public:
            /* member variables */
            std::shared_ptr<String> title;
            /* constructors */
            Model(String* title);
            Model(const Model& obj);
            virtual ~Model() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitModel(this); }
            std::string getType()  override { return "Model"; }
            Model* clone()  override { return new Model(*this); }
    };

    class Define : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<Integer> value;
            /* constructors */
            Define(Name* name, Integer* value);
            Define(const Define& obj);
            virtual ~Define() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitDefine(this); }
            std::string getType()  override { return "Define"; }
            Define* clone()  override { return new Define(*this); }
    };

    class Include : public Statement {
        public:
            /* member variables */
            std::shared_ptr<String> filename;
            /* constructors */
            Include(String* filename);
            Include(const Include& obj);
            virtual ~Include() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitInclude(this); }
            std::string getType()  override { return "Include"; }
            Include* clone()  override { return new Include(*this); }
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
            virtual ~ParamAssign() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitParamAssign(this); }
            std::string getType()  override { return "ParamAssign"; }
            ParamAssign* clone()  override { return new ParamAssign(*this); }
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
            virtual ~Stepped() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitStepped(this); }
            std::string getType()  override { return "Stepped"; }
            Stepped* clone()  override { return new Stepped(*this); }
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
            virtual ~IndependentDef() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitIndependentDef(this); }
            std::string getType()  override { return "IndependentDef"; }
            IndependentDef* clone()  override { return new IndependentDef(*this); }
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
            virtual ~DependentDef() {}

            std::string getName() override { return name->getName(); }
            ModToken* getToken() override { return name->getToken(); }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitDependentDef(this); }
            std::string getType()  override { return "DependentDef"; }
            DependentDef* clone()  override { return new DependentDef(*this); }
    };

    class PlotDeclaration : public Statement {
        public:
            /* member variables */
            PlotVariableVector pvlist;
            std::shared_ptr<PlotVariable> name;
            /* constructors */
            PlotDeclaration(PlotVariableVector pvlist, PlotVariable* name);
            PlotDeclaration(const PlotDeclaration& obj);
            virtual ~PlotDeclaration() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitPlotDeclaration(this); }
            std::string getType()  override { return "PlotDeclaration"; }
            PlotDeclaration* clone()  override { return new PlotDeclaration(*this); }
    };

    class ConductanceHint : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> conductance;
            std::shared_ptr<Name> ion;
            /* constructors */
            ConductanceHint(Name* conductance, Name* ion);
            ConductanceHint(const ConductanceHint& obj);
            virtual ~ConductanceHint() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitConductanceHint(this); }
            std::string getType()  override { return "ConductanceHint"; }
            ConductanceHint* clone()  override { return new ConductanceHint(*this); }
    };

    class ExpressionStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Expression> expression;
            /* constructors */
            ExpressionStatement(Expression* expression);
            ExpressionStatement(const ExpressionStatement& obj);
            virtual ~ExpressionStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitExpressionStatement(this); }
            std::string getType()  override { return "ExpressionStatement"; }
            ExpressionStatement* clone()  override { return new ExpressionStatement(*this); }
    };

    class ProtectStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Expression> expression;
            /* constructors */
            ProtectStatement(Expression* expression);
            ProtectStatement(const ProtectStatement& obj);
            virtual ~ProtectStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitProtectStatement(this); }
            std::string getType()  override { return "ProtectStatement"; }
            ProtectStatement* clone()  override { return new ProtectStatement(*this); }
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
            virtual ~FromStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitFromStatement(this); }
            std::string getType()  override { return "FromStatement"; }
            FromStatement* clone()  override { return new FromStatement(*this); }
    };

    class ForAllStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> name;
            std::shared_ptr<StatementBlock> statementblock;
            /* constructors */
            ForAllStatement(Name* name, StatementBlock* statementblock);
            ForAllStatement(const ForAllStatement& obj);
            virtual ~ForAllStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitForAllStatement(this); }
            std::string getType()  override { return "ForAllStatement"; }
            ForAllStatement* clone()  override { return new ForAllStatement(*this); }
    };

    class WhileStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Expression> condition;
            std::shared_ptr<StatementBlock> statementblock;
            /* constructors */
            WhileStatement(Expression* condition, StatementBlock* statementblock);
            WhileStatement(const WhileStatement& obj);
            virtual ~WhileStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitWhileStatement(this); }
            std::string getType()  override { return "WhileStatement"; }
            WhileStatement* clone()  override { return new WhileStatement(*this); }
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
            virtual ~IfStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitIfStatement(this); }
            std::string getType()  override { return "IfStatement"; }
            IfStatement* clone()  override { return new IfStatement(*this); }
    };

    class ElseIfStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Expression> condition;
            std::shared_ptr<StatementBlock> statementblock;
            /* constructors */
            ElseIfStatement(Expression* condition, StatementBlock* statementblock);
            ElseIfStatement(const ElseIfStatement& obj);
            virtual ~ElseIfStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitElseIfStatement(this); }
            std::string getType()  override { return "ElseIfStatement"; }
            ElseIfStatement* clone()  override { return new ElseIfStatement(*this); }
    };

    class ElseStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<StatementBlock> statementblock;
            /* constructors */
            ElseStatement(StatementBlock* statementblock);
            ElseStatement(const ElseStatement& obj);
            virtual ~ElseStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitElseStatement(this); }
            std::string getType()  override { return "ElseStatement"; }
            ElseStatement* clone()  override { return new ElseStatement(*this); }
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
            virtual ~PartialEquation() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitPartialEquation(this); }
            std::string getType()  override { return "PartialEquation"; }
            PartialEquation* clone()  override { return new PartialEquation(*this); }
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
            virtual ~PartialBoundary() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitPartialBoundary(this); }
            std::string getType()  override { return "PartialBoundary"; }
            PartialBoundary* clone()  override { return new PartialBoundary(*this); }
    };

    class WatchStatement : public Statement {
        public:
            /* member variables */
            WatchVector statements;
            /* constructors */
            WatchStatement(WatchVector statements);
            WatchStatement(const WatchStatement& obj);
            virtual ~WatchStatement() {}

            void addWatch(Watch *s) {
                statements.push_back(std::shared_ptr<Watch>(s));
            }
            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitWatchStatement(this); }
            std::string getType()  override { return "WatchStatement"; }
            WatchStatement* clone()  override { return new WatchStatement(*this); }
    };

    class MutexLock : public Statement {
        public:
            virtual ~MutexLock() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitMutexLock(this); }
            std::string getType()  override { return "MutexLock"; }
            MutexLock* clone()  override { return new MutexLock(*this); }
    };

    class MutexUnlock : public Statement {
        public:
            virtual ~MutexUnlock() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitMutexUnlock(this); }
            std::string getType()  override { return "MutexUnlock"; }
            MutexUnlock* clone()  override { return new MutexUnlock(*this); }
    };

    class Reset : public Statement {
        public:
            virtual ~Reset() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitReset(this); }
            std::string getType()  override { return "Reset"; }
            Reset* clone()  override { return new Reset(*this); }
    };

    class Sens : public Statement {
        public:
            /* member variables */
            VarNameVector senslist;
            /* constructors */
            Sens(VarNameVector senslist);
            Sens(const Sens& obj);
            virtual ~Sens() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitSens(this); }
            std::string getType()  override { return "Sens"; }
            Sens* clone()  override { return new Sens(*this); }
    };

    class Conserve : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Expression> react;
            std::shared_ptr<Expression> expr;
            /* constructors */
            Conserve(Expression* react, Expression* expr);
            Conserve(const Conserve& obj);
            virtual ~Conserve() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitConserve(this); }
            std::string getType()  override { return "Conserve"; }
            Conserve* clone()  override { return new Conserve(*this); }
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
            virtual ~Compartment() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitCompartment(this); }
            std::string getType()  override { return "Compartment"; }
            Compartment* clone()  override { return new Compartment(*this); }
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
            virtual ~LDifuse() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitLDifuse(this); }
            std::string getType()  override { return "LDifuse"; }
            LDifuse* clone()  override { return new LDifuse(*this); }
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
            virtual ~ReactionStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitReactionStatement(this); }
            std::string getType()  override { return "ReactionStatement"; }
            ReactionStatement* clone()  override { return new ReactionStatement(*this); }
    };

    class LagStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Identifier> name;
            std::shared_ptr<Name> byname;
            /* constructors */
            LagStatement(Identifier* name, Name* byname);
            LagStatement(const LagStatement& obj);
            virtual ~LagStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitLagStatement(this); }
            std::string getType()  override { return "LagStatement"; }
            LagStatement* clone()  override { return new LagStatement(*this); }
    };

    class QueueStatement : public Statement {
        public:
            /* member variables */
            std::shared_ptr<QueueExpressionType> qype;
            std::shared_ptr<Identifier> name;
            /* constructors */
            QueueStatement(QueueExpressionType* qype, Identifier* name);
            QueueStatement(const QueueStatement& obj);
            virtual ~QueueStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitQueueStatement(this); }
            std::string getType()  override { return "QueueStatement"; }
            QueueStatement* clone()  override { return new QueueStatement(*this); }
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
            virtual ~ConstantStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitConstantStatement(this); }
            std::string getType()  override { return "ConstantStatement"; }
            ConstantStatement* clone()  override { return new ConstantStatement(*this); }
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
            virtual ~TableStatement() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitTableStatement(this); }
            std::string getType()  override { return "TableStatement"; }
            TableStatement* clone()  override { return new TableStatement(*this); }
    };

    class NrnSuffix : public Statement {
        public:
            /* member variables */
            std::shared_ptr<Name> type;
            std::shared_ptr<Name> name;
            /* constructors */
            NrnSuffix(Name* type, Name* name);
            NrnSuffix(const NrnSuffix& obj);
            virtual ~NrnSuffix() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNrnSuffix(this); }
            std::string getType()  override { return "NrnSuffix"; }
            NrnSuffix* clone()  override { return new NrnSuffix(*this); }
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
            virtual ~NrnUseion() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNrnUseion(this); }
            std::string getType()  override { return "NrnUseion"; }
            NrnUseion* clone()  override { return new NrnUseion(*this); }
    };

    class NrnNonspecific : public Statement {
        public:
            /* member variables */
            NonspeCurVarVector currents;
            /* constructors */
            NrnNonspecific(NonspeCurVarVector currents);
            NrnNonspecific(const NrnNonspecific& obj);
            virtual ~NrnNonspecific() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNrnNonspecific(this); }
            std::string getType()  override { return "NrnNonspecific"; }
            NrnNonspecific* clone()  override { return new NrnNonspecific(*this); }
    };

    class NrnElctrodeCurrent : public Statement {
        public:
            /* member variables */
            ElectrodeCurVarVector ecurrents;
            /* constructors */
            NrnElctrodeCurrent(ElectrodeCurVarVector ecurrents);
            NrnElctrodeCurrent(const NrnElctrodeCurrent& obj);
            virtual ~NrnElctrodeCurrent() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNrnElctrodeCurrent(this); }
            std::string getType()  override { return "NrnElctrodeCurrent"; }
            NrnElctrodeCurrent* clone()  override { return new NrnElctrodeCurrent(*this); }
    };

    class NrnSection : public Statement {
        public:
            /* member variables */
            SectionVarVector sections;
            /* constructors */
            NrnSection(SectionVarVector sections);
            NrnSection(const NrnSection& obj);
            virtual ~NrnSection() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNrnSection(this); }
            std::string getType()  override { return "NrnSection"; }
            NrnSection* clone()  override { return new NrnSection(*this); }
    };

    class NrnRange : public Statement {
        public:
            /* member variables */
            RangeVarVector range_vars;
            /* constructors */
            NrnRange(RangeVarVector range_vars);
            NrnRange(const NrnRange& obj);
            virtual ~NrnRange() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNrnRange(this); }
            std::string getType()  override { return "NrnRange"; }
            NrnRange* clone()  override { return new NrnRange(*this); }
    };

    class NrnGlobal : public Statement {
        public:
            /* member variables */
            GlobalVarVector global_vars;
            /* constructors */
            NrnGlobal(GlobalVarVector global_vars);
            NrnGlobal(const NrnGlobal& obj);
            virtual ~NrnGlobal() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNrnGlobal(this); }
            std::string getType()  override { return "NrnGlobal"; }
            NrnGlobal* clone()  override { return new NrnGlobal(*this); }
    };

    class NrnPointer : public Statement {
        public:
            /* member variables */
            PointerVarVector pointers;
            /* constructors */
            NrnPointer(PointerVarVector pointers);
            NrnPointer(const NrnPointer& obj);
            virtual ~NrnPointer() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNrnPointer(this); }
            std::string getType()  override { return "NrnPointer"; }
            NrnPointer* clone()  override { return new NrnPointer(*this); }
    };

    class NrnBbcorePtr : public Statement {
        public:
            /* member variables */
            BbcorePointerVarVector bbcore_pointers;
            /* constructors */
            NrnBbcorePtr(BbcorePointerVarVector bbcore_pointers);
            NrnBbcorePtr(const NrnBbcorePtr& obj);
            virtual ~NrnBbcorePtr() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNrnBbcorePtr(this); }
            std::string getType()  override { return "NrnBbcorePtr"; }
            NrnBbcorePtr* clone()  override { return new NrnBbcorePtr(*this); }
    };

    class NrnExternal : public Statement {
        public:
            /* member variables */
            ExternVarVector externals;
            /* constructors */
            NrnExternal(ExternVarVector externals);
            NrnExternal(const NrnExternal& obj);
            virtual ~NrnExternal() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNrnExternal(this); }
            std::string getType()  override { return "NrnExternal"; }
            NrnExternal* clone()  override { return new NrnExternal(*this); }
    };

    class NrnThreadSafe : public Statement {
        public:
            /* member variables */
            ThreadsafeVarVector threadsafe;
            /* constructors */
            NrnThreadSafe(ThreadsafeVarVector threadsafe);
            NrnThreadSafe(const NrnThreadSafe& obj);
            virtual ~NrnThreadSafe() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitNrnThreadSafe(this); }
            std::string getType()  override { return "NrnThreadSafe"; }
            NrnThreadSafe* clone()  override { return new NrnThreadSafe(*this); }
    };

    class Verbatim : public Statement {
        public:
            /* member variables */
            std::shared_ptr<String> statement;
            /* constructors */
            Verbatim(String* statement);
            Verbatim(const Verbatim& obj);
            virtual ~Verbatim() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitVerbatim(this); }
            std::string getType()  override { return "Verbatim"; }
            Verbatim* clone()  override { return new Verbatim(*this); }
    };

    class Comment : public Statement {
        public:
            /* member variables */
            std::shared_ptr<String> comment;
            /* constructors */
            Comment(String* comment);
            Comment(const Comment& obj);
            virtual ~Comment() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitComment(this); }
            std::string getType()  override { return "Comment"; }
            Comment* clone()  override { return new Comment(*this); }
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
            virtual ~Program() {}

            void addStatement(Statement *s) {
                statements.push_back(std::shared_ptr<Statement>(s));
            }
            void addBlock(Block *s) {
                blocks.push_back(std::shared_ptr<Block>(s));
            }
            Program() {}

            void visitChildren (Visitor* v)  override;
            void accept(Visitor* v)  override { v->visitProgram(this); }
            std::string getType()  override { return "Program"; }
            Program* clone()  override { return new Program(*this); }
            void setBlockSymbolTable(void *s)  { symtab = s; }
            void* getBlockSymbolTable()  { return symtab; }
    };



} // namespace ast
