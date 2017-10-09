#ifndef __AST_HPP
#define __AST_HPP

/* headers */
#include <iostream>
#include <list>
#include <vector>
#include <stack>
#include <string>
#include <sstream>

#include "utils/commonutils.hpp"
#include "lexer/modtoken.hpp"

namespace ast {

	/* enumaration of all binary operators in the language */
	typedef enum {BOP_ADDITION, BOP_SUBTRACTION, BOP_MULTIPLICATION, BOP_DIVISION, BOP_POWER, BOP_AND, BOP_OR, BOP_GREATER, BOP_LESS, BOP_GREATER_EQUAL, BOP_LESS_EQUAL, BOP_ASSIGN, BOP_NOT_EQUAL, BOP_EXACT_EQUAL} BinaryOp;
	static const std::string BinaryOpNames[] = {"+", "-", "*", "/", "^", "&&", "||", ">", "<", ">=", "<=", "=", "!=", "=="};

	/* enumaration of all unary operators in the language */
	typedef enum {UOP_NOT, UOP_NEGATION} UnaryOp;
	static const std::string UnaryOpNames[] = {"!", "-"};

	/* enumaration of types used in partial equation */
	typedef enum {PEQ_FIRST, PEQ_LAST} FirstLastType;
	static const std::string FirstLastTypeNames[] = {"FIRST", "LAST"};

	/* enumaration of queue types */
	typedef enum {PUT_QUEUE, GET_QUEUE} QueueType;
	static const std::string QueueTypeNames[] = {"PUTQ", "GETQ"};

	/* enumaration of type used for BEFORE or AFTER block */
	typedef enum {BATYPE_BREAKPOINT, BATYPE_SOLVE, BATYPE_INITIAL, BATYPE_STEP} BAType;
	static const std::string BATypeNames[] = {"BREAKPOINT", "SOLVE", "INITIAL", "STEP"};

	/* enumaration of type used for UNIT_ON or UNIT_OFF state*/
	typedef enum {UNIT_ON, UNIT_OFF} UnitStateType;
	static const std::string UnitStateTypeNames[] = {"UNITSON", "UNITSOFF"};

	/* enumaration of type used for Reaction statement */
	typedef enum {LTMINUSGT, LTLT, MINUSGT} ReactionOp;
	static const std::string ReactionOpNames[] = {"<->", "<<", "->"};

	/* forward declarations of AST Node classes */
	class ExpressionNode;
	class StatementNode;
	class IdentifierNode;
	class BlockNode;
	class NumberNode;
	class StringNode;
	class IntegerNode;
	class FloatNode;
	class DoubleNode;
	class BooleanNode;
	class NameNode;
	class PrimeNameNode;
	class VarNameNode;
	class IndexedNameNode;
	class UnitNode;
	class UnitStateNode;
	class DoubleUnitNode;
	class ArgumentNode;
	class LocalVariableNode;
	class LocalListStatementNode;
	class LimitsNode;
	class NumberRangeNode;
	class ProgramNode;
	class ModelNode;
	class DefineNode;
	class IncludeNode;
	class ParamBlockNode;
	class ParamAssignNode;
	class StepBlockNode;
	class SteppedNode;
	class IndependentBlockNode;
	class IndependentDefNode;
	class DependentDefNode;
	class DependentBlockNode;
	class StateBlockNode;
	class PlotBlockNode;
	class PlotDeclarationNode;
	class PlotVariableNode;
	class InitialBlockNode;
	class ConstructorBlockNode;
	class DestructorBlockNode;
	class ConductanceHintNode;
	class ExpressionStatementNode;
	class ProtectStatementNode;
	class StatementBlockNode;
	class BinaryOperatorNode;
	class UnaryOperatorNode;
	class ReactionOperatorNode;
	class BinaryExpressionNode;
	class UnaryExpressionNode;
	class NonLinEuationNode;
	class LinEquationNode;
	class FunctionCallNode;
	class FromStatementNode;
	class ForAllStatementNode;
	class WhileStatementNode;
	class IfStatementNode;
	class ElseIfStatementNode;
	class ElseStatementNode;
	class DerivativeBlockNode;
	class LinearBlockNode;
	class NonLinearBlockNode;
	class DiscreteBlockNode;
	class PartialBlockNode;
	class PartialEquationNode;
	class FirstLastTypeIndexNode;
	class PartialBoundaryNode;
	class FunctionTableBlockNode;
	class FunctionBlockNode;
	class ProcedureBlockNode;
	class NetReceiveBlockNode;
	class SolveBlockNode;
	class BreakpointBlockNode;
	class TerminalBlockNode;
	class BeforeBlockNode;
	class AfterBlockNode;
	class BABlockTypeNode;
	class BABlockNode;
	class WatchStatementNode;
	class WatchNode;
	class ForNetconNode;
	class MutexLockNode;
	class MutexUnlockNode;
	class ResetNode;
	class SensNode;
	class ConserveNode;
	class CompartmentNode;
	class LDifuseNode;
	class KineticBlockNode;
	class ReactionStatementNode;
	class ReactVarNameNode;
	class LagStatementNode;
	class QueueStatementNode;
	class QueueExpressionTypeNode;
	class MatchBlockNode;
	class MatchNode;
	class UnitBlockNode;
	class UnitDefNode;
	class FactordefNode;
	class ConstantStatementNode;
	class ConstantBlockNode;
	class TableStatementNode;
	class NeuronBlockNode;
	class ReadIonVarNode;
	class WriteIonVarNode;
	class NonspeCurVarNode;
	class ElectrodeCurVarNode;
	class SectionVarNode;
	class RangeVarNode;
	class GlobalVarNode;
	class PointerVarNode;
	class BbcorePointerVarNode;
	class ExternVarNode;
	class ThreadsafeVarNode;
	class NrnSuffixNode;
	class NrnUseionNode;
	class NrnNonspecificNode;
	class NrnElctrodeCurrentNode;
	class NrnSectionNode;
	class NrnRangeNode;
	class NrnGlobalNode;
	class NrnPointerNode;
	class NrnBbcorePtrNode;
	class NrnExternalNode;
	class NrnThreadSafeNode;
	class VerbatimNode;
	class CommentNode;
	class ValenceNode;

	/* abstract node classes */
	class ASTNode;
	class ExpressionNode;
	class NumberNode;
	class IdentifierNode;
	class StatementNode;
	class BlockNode;

	/* std::vector for convenience */
	typedef std::vector<ExpressionNode*> ExpressionNodeList;
	typedef std::vector<StatementNode*> StatementNodeList;
	typedef std::vector<IdentifierNode*> IdentifierNodeList;
	typedef std::vector<BlockNode*> BlockNodeList;
	typedef std::vector<NumberNode*> NumberNodeList;
	typedef std::vector<StringNode*> StringNodeList;
	typedef std::vector<IntegerNode*> IntegerNodeList;
	typedef std::vector<FloatNode*> FloatNodeList;
	typedef std::vector<DoubleNode*> DoubleNodeList;
	typedef std::vector<BooleanNode*> BooleanNodeList;
	typedef std::vector<NameNode*> NameNodeList;
	typedef std::vector<PrimeNameNode*> PrimeNameNodeList;
	typedef std::vector<VarNameNode*> VarNameNodeList;
	typedef std::vector<IndexedNameNode*> IndexedNameNodeList;
	typedef std::vector<UnitNode*> UnitNodeList;
	typedef std::vector<UnitStateNode*> UnitStateNodeList;
	typedef std::vector<DoubleUnitNode*> DoubleUnitNodeList;
	typedef std::vector<ArgumentNode*> ArgumentNodeList;
	typedef std::vector<LocalVariableNode*> LocalVariableNodeList;
	typedef std::vector<LocalListStatementNode*> LocalListStatementNodeList;
	typedef std::vector<LimitsNode*> LimitsNodeList;
	typedef std::vector<NumberRangeNode*> NumberRangeNodeList;
	typedef std::vector<ProgramNode*> ProgramNodeList;
	typedef std::vector<ModelNode*> ModelNodeList;
	typedef std::vector<DefineNode*> DefineNodeList;
	typedef std::vector<IncludeNode*> IncludeNodeList;
	typedef std::vector<ParamBlockNode*> ParamBlockNodeList;
	typedef std::vector<ParamAssignNode*> ParamAssignNodeList;
	typedef std::vector<StepBlockNode*> StepBlockNodeList;
	typedef std::vector<SteppedNode*> SteppedNodeList;
	typedef std::vector<IndependentBlockNode*> IndependentBlockNodeList;
	typedef std::vector<IndependentDefNode*> IndependentDefNodeList;
	typedef std::vector<DependentDefNode*> DependentDefNodeList;
	typedef std::vector<DependentBlockNode*> DependentBlockNodeList;
	typedef std::vector<StateBlockNode*> StateBlockNodeList;
	typedef std::vector<PlotBlockNode*> PlotBlockNodeList;
	typedef std::vector<PlotDeclarationNode*> PlotDeclarationNodeList;
	typedef std::vector<PlotVariableNode*> PlotVariableNodeList;
	typedef std::vector<InitialBlockNode*> InitialBlockNodeList;
	typedef std::vector<ConstructorBlockNode*> ConstructorBlockNodeList;
	typedef std::vector<DestructorBlockNode*> DestructorBlockNodeList;
	typedef std::vector<ConductanceHintNode*> ConductanceHintNodeList;
	typedef std::vector<ExpressionStatementNode*> ExpressionStatementNodeList;
	typedef std::vector<ProtectStatementNode*> ProtectStatementNodeList;
	typedef std::vector<StatementBlockNode*> StatementBlockNodeList;
	typedef std::vector<BinaryOperatorNode*> BinaryOperatorNodeList;
	typedef std::vector<UnaryOperatorNode*> UnaryOperatorNodeList;
	typedef std::vector<ReactionOperatorNode*> ReactionOperatorNodeList;
	typedef std::vector<BinaryExpressionNode*> BinaryExpressionNodeList;
	typedef std::vector<UnaryExpressionNode*> UnaryExpressionNodeList;
	typedef std::vector<NonLinEuationNode*> NonLinEuationNodeList;
	typedef std::vector<LinEquationNode*> LinEquationNodeList;
	typedef std::vector<FunctionCallNode*> FunctionCallNodeList;
	typedef std::vector<FromStatementNode*> FromStatementNodeList;
	typedef std::vector<ForAllStatementNode*> ForAllStatementNodeList;
	typedef std::vector<WhileStatementNode*> WhileStatementNodeList;
	typedef std::vector<IfStatementNode*> IfStatementNodeList;
	typedef std::vector<ElseIfStatementNode*> ElseIfStatementNodeList;
	typedef std::vector<ElseStatementNode*> ElseStatementNodeList;
	typedef std::vector<DerivativeBlockNode*> DerivativeBlockNodeList;
	typedef std::vector<LinearBlockNode*> LinearBlockNodeList;
	typedef std::vector<NonLinearBlockNode*> NonLinearBlockNodeList;
	typedef std::vector<DiscreteBlockNode*> DiscreteBlockNodeList;
	typedef std::vector<PartialBlockNode*> PartialBlockNodeList;
	typedef std::vector<PartialEquationNode*> PartialEquationNodeList;
	typedef std::vector<FirstLastTypeIndexNode*> FirstLastTypeIndexNodeList;
	typedef std::vector<PartialBoundaryNode*> PartialBoundaryNodeList;
	typedef std::vector<FunctionTableBlockNode*> FunctionTableBlockNodeList;
	typedef std::vector<FunctionBlockNode*> FunctionBlockNodeList;
	typedef std::vector<ProcedureBlockNode*> ProcedureBlockNodeList;
	typedef std::vector<NetReceiveBlockNode*> NetReceiveBlockNodeList;
	typedef std::vector<SolveBlockNode*> SolveBlockNodeList;
	typedef std::vector<BreakpointBlockNode*> BreakpointBlockNodeList;
	typedef std::vector<TerminalBlockNode*> TerminalBlockNodeList;
	typedef std::vector<BeforeBlockNode*> BeforeBlockNodeList;
	typedef std::vector<AfterBlockNode*> AfterBlockNodeList;
	typedef std::vector<BABlockTypeNode*> BABlockTypeNodeList;
	typedef std::vector<BABlockNode*> BABlockNodeList;
	typedef std::vector<WatchStatementNode*> WatchStatementNodeList;
	typedef std::vector<WatchNode*> WatchNodeList;
	typedef std::vector<ForNetconNode*> ForNetconNodeList;
	typedef std::vector<MutexLockNode*> MutexLockNodeList;
	typedef std::vector<MutexUnlockNode*> MutexUnlockNodeList;
	typedef std::vector<ResetNode*> ResetNodeList;
	typedef std::vector<SensNode*> SensNodeList;
	typedef std::vector<ConserveNode*> ConserveNodeList;
	typedef std::vector<CompartmentNode*> CompartmentNodeList;
	typedef std::vector<LDifuseNode*> LDifuseNodeList;
	typedef std::vector<KineticBlockNode*> KineticBlockNodeList;
	typedef std::vector<ReactionStatementNode*> ReactionStatementNodeList;
	typedef std::vector<ReactVarNameNode*> ReactVarNameNodeList;
	typedef std::vector<LagStatementNode*> LagStatementNodeList;
	typedef std::vector<QueueStatementNode*> QueueStatementNodeList;
	typedef std::vector<QueueExpressionTypeNode*> QueueExpressionTypeNodeList;
	typedef std::vector<MatchBlockNode*> MatchBlockNodeList;
	typedef std::vector<MatchNode*> MatchNodeList;
	typedef std::vector<UnitBlockNode*> UnitBlockNodeList;
	typedef std::vector<UnitDefNode*> UnitDefNodeList;
	typedef std::vector<FactordefNode*> FactordefNodeList;
	typedef std::vector<ConstantStatementNode*> ConstantStatementNodeList;
	typedef std::vector<ConstantBlockNode*> ConstantBlockNodeList;
	typedef std::vector<TableStatementNode*> TableStatementNodeList;
	typedef std::vector<NeuronBlockNode*> NeuronBlockNodeList;
	typedef std::vector<ReadIonVarNode*> ReadIonVarNodeList;
	typedef std::vector<WriteIonVarNode*> WriteIonVarNodeList;
	typedef std::vector<NonspeCurVarNode*> NonspeCurVarNodeList;
	typedef std::vector<ElectrodeCurVarNode*> ElectrodeCurVarNodeList;
	typedef std::vector<SectionVarNode*> SectionVarNodeList;
	typedef std::vector<RangeVarNode*> RangeVarNodeList;
	typedef std::vector<GlobalVarNode*> GlobalVarNodeList;
	typedef std::vector<PointerVarNode*> PointerVarNodeList;
	typedef std::vector<BbcorePointerVarNode*> BbcorePointerVarNodeList;
	typedef std::vector<ExternVarNode*> ExternVarNodeList;
	typedef std::vector<ThreadsafeVarNode*> ThreadsafeVarNodeList;
	typedef std::vector<NrnSuffixNode*> NrnSuffixNodeList;
	typedef std::vector<NrnUseionNode*> NrnUseionNodeList;
	typedef std::vector<NrnNonspecificNode*> NrnNonspecificNodeList;
	typedef std::vector<NrnElctrodeCurrentNode*> NrnElctrodeCurrentNodeList;
	typedef std::vector<NrnSectionNode*> NrnSectionNodeList;
	typedef std::vector<NrnRangeNode*> NrnRangeNodeList;
	typedef std::vector<NrnGlobalNode*> NrnGlobalNodeList;
	typedef std::vector<NrnPointerNode*> NrnPointerNodeList;
	typedef std::vector<NrnBbcorePtrNode*> NrnBbcorePtrNodeList;
	typedef std::vector<NrnExternalNode*> NrnExternalNodeList;
	typedef std::vector<NrnThreadSafeNode*> NrnThreadSafeNodeList;
	typedef std::vector<VerbatimNode*> VerbatimNodeList;
	typedef std::vector<CommentNode*> CommentNodeList;
	typedef std::vector<ValenceNode*> ValenceNodeList;
	typedef std::vector<ASTNode*> ASTNodeList;
	typedef std::vector<ExpressionNode*> ExpressionNodeList;
	typedef std::vector<NumberNode*> NumberNodeList;
	typedef std::vector<IdentifierNode*> IdentifierNodeList;
	typedef std::vector<StatementNode*> StatementNodeList;
	typedef std::vector<BlockNode*> BlockNodeList;
	/* basic types */

	/* @todo: how to do this */
	#include "visitors/visitor.hpp"
	/* check order of inclusion */
	#ifdef YYSTYPE_IS_TRIVIAL
	#error Make sure to include this file BEFORE parser.hpp
	#endif

	/* define YYSTYPE (type of all $$, $N variables) as a union
	 * of all necessary pointers (including lists). This will
	 * be used in %type specifiers and in lexer.
	 */
	typedef union {
	  ExpressionNode *expression_ptr;
	  StatementNode *statement_ptr;
	  IdentifierNode *identifier_ptr;
	  BlockNode *block_ptr;
	  NumberNode *number_ptr;
	  StringNode *string_ptr;
	  IntegerNode *integer_ptr;
	  NameNode *name_ptr;
	  FloatNode *float_ptr;
	  DoubleNode *double_ptr;
	  BooleanNode *boolean_ptr;
	  PrimeNameNode *primename_ptr;
	  VarNameNode *varname_ptr;
	  IndexedNameNode *indexedname_ptr;
	  UnitNode *unit_ptr;
	  UnitStateNode *unitstate_ptr;
	  DoubleUnitNode *doubleunit_ptr;
	  ArgumentNode *argument_ptr;
	  LocalVariableNode *localvariable_ptr;
	  LocalListStatementNode *localliststatement_ptr;
	  LocalVariableNodeList *localvariable_list_ptr;
	  LimitsNode *limits_ptr;
	  NumberRangeNode *numberrange_ptr;
	  ProgramNode *program_ptr;
	  StatementNodeList *statement_list_ptr;
	  BlockNodeList *block_list_ptr;
	  ModelNode *model_ptr;
	  DefineNode *define_ptr;
	  IncludeNode *include_ptr;
	  ParamBlockNode *paramblock_ptr;
	  ParamAssignNodeList *paramassign_list_ptr;
	  ParamAssignNode *paramassign_ptr;
	  StepBlockNode *stepblock_ptr;
	  SteppedNodeList *stepped_list_ptr;
	  SteppedNode *stepped_ptr;
	  NumberNodeList *number_list_ptr;
	  IndependentBlockNode *independentblock_ptr;
	  IndependentDefNodeList *independentdef_list_ptr;
	  IndependentDefNode *independentdef_ptr;
	  DependentDefNode *dependentdef_ptr;
	  DependentBlockNode *dependentblock_ptr;
	  DependentDefNodeList *dependentdef_list_ptr;
	  StateBlockNode *stateblock_ptr;
	  PlotBlockNode *plotblock_ptr;
	  PlotDeclarationNode *plotdeclaration_ptr;
	  PlotVariableNodeList *plotvariable_list_ptr;
	  PlotVariableNode *plotvariable_ptr;
	  InitialBlockNode *initialblock_ptr;
	  StatementBlockNode *statementblock_ptr;
	  ConstructorBlockNode *constructorblock_ptr;
	  DestructorBlockNode *destructorblock_ptr;
	  ConductanceHintNode *conductancehint_ptr;
	  ExpressionStatementNode *expressionstatement_ptr;
	  ProtectStatementNode *protectstatement_ptr;
	  BinaryOperatorNode *binaryoperator_ptr;
	  UnaryOperatorNode *unaryoperator_ptr;
	  ReactionOperatorNode *reactionoperator_ptr;
	  BinaryExpressionNode *binaryexpression_ptr;
	  UnaryExpressionNode *unaryexpression_ptr;
	  NonLinEuationNode *nonlineuation_ptr;
	  LinEquationNode *linequation_ptr;
	  FunctionCallNode *functioncall_ptr;
	  ExpressionNodeList *expression_list_ptr;
	  FromStatementNode *fromstatement_ptr;
	  ForAllStatementNode *forallstatement_ptr;
	  WhileStatementNode *whilestatement_ptr;
	  IfStatementNode *ifstatement_ptr;
	  ElseIfStatementNodeList *elseifstatement_list_ptr;
	  ElseStatementNode *elsestatement_ptr;
	  ElseIfStatementNode *elseifstatement_ptr;
	  DerivativeBlockNode *derivativeblock_ptr;
	  LinearBlockNode *linearblock_ptr;
	  NameNodeList *name_list_ptr;
	  NonLinearBlockNode *nonlinearblock_ptr;
	  DiscreteBlockNode *discreteblock_ptr;
	  PartialBlockNode *partialblock_ptr;
	  PartialEquationNode *partialequation_ptr;
	  FirstLastTypeIndexNode *firstlasttypeindex_ptr;
	  PartialBoundaryNode *partialboundary_ptr;
	  FunctionTableBlockNode *functiontableblock_ptr;
	  ArgumentNodeList *argument_list_ptr;
	  FunctionBlockNode *functionblock_ptr;
	  ProcedureBlockNode *procedureblock_ptr;
	  NetReceiveBlockNode *netreceiveblock_ptr;
	  SolveBlockNode *solveblock_ptr;
	  BreakpointBlockNode *breakpointblock_ptr;
	  TerminalBlockNode *terminalblock_ptr;
	  BeforeBlockNode *beforeblock_ptr;
	  BABlockNode *bablock_ptr;
	  AfterBlockNode *afterblock_ptr;
	  BABlockTypeNode *bablocktype_ptr;
	  WatchStatementNode *watchstatement_ptr;
	  WatchNodeList *watch_list_ptr;
	  WatchNode *watch_ptr;
	  ForNetconNode *fornetcon_ptr;
	  MutexLockNode *mutexlock_ptr;
	  MutexUnlockNode *mutexunlock_ptr;
	  ResetNode *reset_ptr;
	  SensNode *sens_ptr;
	  VarNameNodeList *varname_list_ptr;
	  ConserveNode *conserve_ptr;
	  CompartmentNode *compartment_ptr;
	  LDifuseNode *ldifuse_ptr;
	  KineticBlockNode *kineticblock_ptr;
	  ReactionStatementNode *reactionstatement_ptr;
	  ReactVarNameNode *reactvarname_ptr;
	  LagStatementNode *lagstatement_ptr;
	  QueueStatementNode *queuestatement_ptr;
	  QueueExpressionTypeNode *queueexpressiontype_ptr;
	  MatchBlockNode *matchblock_ptr;
	  MatchNodeList *match_list_ptr;
	  MatchNode *match_ptr;
	  UnitBlockNode *unitblock_ptr;
	  UnitDefNode *unitdef_ptr;
	  FactordefNode *factordef_ptr;
	  ConstantStatementNode *constantstatement_ptr;
	  ConstantBlockNode *constantblock_ptr;
	  ConstantStatementNodeList *constantstatement_list_ptr;
	  TableStatementNode *tablestatement_ptr;
	  NeuronBlockNode *neuronblock_ptr;
	  ReadIonVarNode *readionvar_ptr;
	  WriteIonVarNode *writeionvar_ptr;
	  NonspeCurVarNode *nonspecurvar_ptr;
	  ElectrodeCurVarNode *electrodecurvar_ptr;
	  SectionVarNode *sectionvar_ptr;
	  RangeVarNode *rangevar_ptr;
	  GlobalVarNode *globalvar_ptr;
	  PointerVarNode *pointervar_ptr;
	  BbcorePointerVarNode *bbcorepointervar_ptr;
	  ExternVarNode *externvar_ptr;
	  ThreadsafeVarNode *threadsafevar_ptr;
	  NrnSuffixNode *nrnsuffix_ptr;
	  NrnUseionNode *nrnuseion_ptr;
	  ReadIonVarNodeList *readionvar_list_ptr;
	  WriteIonVarNodeList *writeionvar_list_ptr;
	  ValenceNode *valence_ptr;
	  NrnNonspecificNode *nrnnonspecific_ptr;
	  NonspeCurVarNodeList *nonspecurvar_list_ptr;
	  NrnElctrodeCurrentNode *nrnelctrodecurrent_ptr;
	  ElectrodeCurVarNodeList *electrodecurvar_list_ptr;
	  NrnSectionNode *nrnsection_ptr;
	  SectionVarNodeList *sectionvar_list_ptr;
	  NrnRangeNode *nrnrange_ptr;
	  RangeVarNodeList *rangevar_list_ptr;
	  NrnGlobalNode *nrnglobal_ptr;
	  GlobalVarNodeList *globalvar_list_ptr;
	  NrnPointerNode *nrnpointer_ptr;
	  PointerVarNodeList *pointervar_list_ptr;
	  NrnBbcorePtrNode *nrnbbcoreptr_ptr;
	  BbcorePointerVarNodeList *bbcorepointervar_list_ptr;
	  NrnExternalNode *nrnexternal_ptr;
	  ExternVarNodeList *externvar_list_ptr;
	  NrnThreadSafeNode *nrnthreadsafe_ptr;
	  ThreadsafeVarNodeList *threadsafevar_list_ptr;
	  VerbatimNode *verbatim_ptr;
	  CommentNode *comment_ptr;
	  ASTNode *ast_ptr;
	  char* base_char_ptr;
	  int base_int;
	} astnode_union;

	//#define YYSTYPE astnode_union


/* define abstract base class for all AST Nodes
 * this also serves to define the visitable objects.
 */
class ASTNode {

    public:
    /* all AST nodes have a member which stores their
     * basetype (int, bool, none, object). Further type
     * information will come from the symbol table.
     * BaseType basetype;
     */

    /* all AST nodes provide visit children and accept methods */
    virtual void visitChildren(Visitor* v) = 0;
    virtual void accept(Visitor* v) = 0;
    virtual std::string getNodeType() = 0;
    /* @todo: please revisit this. adding quickly for symtab */
    virtual std::string getName() { return "";}
    virtual ModToken* getToken() { /*std::cout << "\n ERROR: getToken not implemented!";*/ return NULL; }
    //virtual ASTNode* clone() { std::cout << "\n ERROR: clone() not implemented! \n"; abort(); }
};



	/* Define all other AST nodes */

	/* ast Node for Expression */
	class ExpressionNode : public ASTNode {
		public:

		    virtual ~ExpressionNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitExpressionNode(this); }
		    virtual std::string getNodeType() { return "Expression"; }
		    virtual ExpressionNode* clone() { return new ExpressionNode(*this); }
	};

	/* ast Node for Statement */
	class StatementNode : public ASTNode {
		public:

		    virtual ~StatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitStatementNode(this); }
		    virtual std::string getNodeType() { return "Statement"; }
		    virtual StatementNode* clone() { return new StatementNode(*this); }
	};

	/* ast Node for Identifier */
	class IdentifierNode : public ExpressionNode {
		public:

		    virtual ~IdentifierNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitIdentifierNode(this); }
		    virtual std::string getNodeType() { return "Identifier"; }
		    virtual IdentifierNode* clone() { return new IdentifierNode(*this); }
		    virtual void setName(std::string name) { std::cout << "ERROR : setName() not implemented! "; abort(); }
	};

	/* ast Node for Block */
	class BlockNode : public ExpressionNode {
		public:

		    virtual ~BlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitBlockNode(this); }
		    virtual std::string getNodeType() { return "Block"; }
		    virtual BlockNode* clone() { return new BlockNode(*this); }
	};

	/* ast Node for Number */
	class NumberNode : public ExpressionNode {
		public:

		    virtual ~NumberNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNumberNode(this); }
		    virtual std::string getNodeType() { return "Number"; }
		    virtual NumberNode* clone() { return new NumberNode(*this); }
		    virtual void negate() { std::cout << "ERROR : negate() not implemented! "; abort(); }
	};

	/* ast Node for String */
	class StringNode : public ExpressionNode {
		public:
		    /* member variables */
		    std::string value;
		    ModToken *token;

		    /* constructor */
		    StringNode(std::string value);

		    /* copy constructor */
		    StringNode(const StringNode& obj);

		    virtual ~StringNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitStringNode(this); }
		    virtual std::string getNodeType() { return "String"; }
		    virtual StringNode* clone() { return new StringNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    std::string eval() { return value; }
		    virtual void set(std::string _value) { value = _value; }
	};

	/* ast Node for Integer */
	class IntegerNode : public NumberNode {
		public:
		    /* member variables */
		    int value;
		    NameNode *macroname;
		    ModToken *token;

		    /* constructor */
		    IntegerNode(int value, NameNode *macroname);

		    /* copy constructor */
		    IntegerNode(const IntegerNode& obj);

		    virtual ~IntegerNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitIntegerNode(this); }
		    virtual std::string getNodeType() { return "Integer"; }
		    virtual IntegerNode* clone() { return new IntegerNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    void negate() { value = -value; }
		    int eval() { return value; }
		    virtual void set(int _value) { value = _value; }
	};

	/* ast Node for Float */
	class FloatNode : public NumberNode {
		public:
		    /* member variables */
		    float value;

		    /* constructor */
		    FloatNode(float value);

		    /* copy constructor */
		    FloatNode(const FloatNode& obj);

		    virtual ~FloatNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitFloatNode(this); }
		    virtual std::string getNodeType() { return "Float"; }
		    virtual FloatNode* clone() { return new FloatNode(*this); }
		    void negate() { value = -value; }
		    float eval() { return value; }
		    virtual void set(float _value) { value = _value; }
	};

	/* ast Node for Double */
	class DoubleNode : public NumberNode {
		public:
		    /* member variables */
		    double value;
		    ModToken *token;

		    /* constructor */
		    DoubleNode(double value);

		    /* copy constructor */
		    DoubleNode(const DoubleNode& obj);

		    virtual ~DoubleNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitDoubleNode(this); }
		    virtual std::string getNodeType() { return "Double"; }
		    virtual DoubleNode* clone() { return new DoubleNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    void negate() { value = -value; }
		    double eval() { return value; }
		    virtual void set(double _value) { value = _value; }
	};

	/* ast Node for Boolean */
	class BooleanNode : public NumberNode {
		public:
		    /* member variables */
		    int value;

		    /* constructor */
		    BooleanNode(int value);

		    /* copy constructor */
		    BooleanNode(const BooleanNode& obj);

		    virtual ~BooleanNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitBooleanNode(this); }
		    virtual std::string getNodeType() { return "Boolean"; }
		    virtual BooleanNode* clone() { return new BooleanNode(*this); }
		    void negate() { value = !value; }
		    bool eval() { return value; }
		    virtual void set(bool _value) { value = _value; }
	};

	/* ast Node for Name */
	class NameNode : public IdentifierNode {
		public:
		    /* member variables */
		    StringNode *value;
		    ModToken *token;

		    /* constructor */
		    NameNode(StringNode *value);

		    /* copy constructor */
		    NameNode(const NameNode& obj);

		    virtual std::string getName() { return value->eval(); }

		    virtual ~NameNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNameNode(this); }
		    virtual std::string getNodeType() { return "Name"; }
		    virtual NameNode* clone() { return new NameNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    virtual void setName(std::string name) { value->set(name); }
	};

	/* ast Node for PrimeName */
	class PrimeNameNode : public IdentifierNode {
		public:
		    /* member variables */
		    StringNode *value;
		    IntegerNode *order;
		    ModToken *token;

		    /* constructor */
		    PrimeNameNode(StringNode *value, IntegerNode *order);

		    /* copy constructor */
		    PrimeNameNode(const PrimeNameNode& obj);

		    virtual std::string getName() { return value->eval(); }
		    virtual int getOrder() { return order->eval(); }

		    virtual ~PrimeNameNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitPrimeNameNode(this); }
		    virtual std::string getNodeType() { return "PrimeName"; }
		    virtual PrimeNameNode* clone() { return new PrimeNameNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
	};

	/* ast Node for VarName */
	class VarNameNode : public IdentifierNode {
		public:
		    /* member variables */
		    IdentifierNode *name;
		    IntegerNode *at_index;

		    /* constructor */
		    VarNameNode(IdentifierNode *name, IntegerNode *at_index);

		    /* copy constructor */
		    VarNameNode(const VarNameNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~VarNameNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitVarNameNode(this); }
		    virtual std::string getNodeType() { return "VarName"; }
		    virtual VarNameNode* clone() { return new VarNameNode(*this); }
	};

	/* ast Node for IndexedName */
	class IndexedNameNode : public IdentifierNode {
		public:
		    /* member variables */
		    IdentifierNode *name;
		    ExpressionNode *index;

		    /* constructor */
		    IndexedNameNode(IdentifierNode *name, ExpressionNode *index);

		    /* copy constructor */
		    IndexedNameNode(const IndexedNameNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~IndexedNameNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitIndexedNameNode(this); }
		    virtual std::string getNodeType() { return "IndexedName"; }
		    virtual IndexedNameNode* clone() { return new IndexedNameNode(*this); }
	};

	/* ast Node for Unit */
	class UnitNode : public ExpressionNode {
		public:
		    /* member variables */
		    StringNode *name;

		    /* constructor */
		    UnitNode(StringNode *name);

		    /* copy constructor */
		    UnitNode(const UnitNode& obj);

		    virtual std::string getName() { return name->eval(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~UnitNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitUnitNode(this); }
		    virtual std::string getNodeType() { return "Unit"; }
		    virtual UnitNode* clone() { return new UnitNode(*this); }
	};

	/* ast Node for UnitState */
	class UnitStateNode : public StatementNode {
		public:
		    /* member variables */
		    UnitStateType value;

		    /* constructor */
		    UnitStateNode(UnitStateType value);

		    /* copy constructor */
		    UnitStateNode(const UnitStateNode& obj);

		    virtual ~UnitStateNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitUnitStateNode(this); }
		    virtual std::string getNodeType() { return "UnitState"; }
		    virtual UnitStateNode* clone() { return new UnitStateNode(*this); }
		    std::string  eval() { return UnitStateTypeNames[value]; }
	};

	/* ast Node for DoubleUnit */
	class DoubleUnitNode : public ExpressionNode {
		public:
		    /* member variables */
		    DoubleNode *values;
		    UnitNode *unit;

		    /* constructor */
		    DoubleUnitNode(DoubleNode *values, UnitNode *unit);

		    /* copy constructor */
		    DoubleUnitNode(const DoubleUnitNode& obj);

		    virtual ~DoubleUnitNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitDoubleUnitNode(this); }
		    virtual std::string getNodeType() { return "DoubleUnit"; }
		    virtual DoubleUnitNode* clone() { return new DoubleUnitNode(*this); }
	};

	/* ast Node for Argument */
	class ArgumentNode : public IdentifierNode {
		public:
		    /* member variables */
		    IdentifierNode *name;
		    UnitNode *unit;

		    /* constructor */
		    ArgumentNode(IdentifierNode *name, UnitNode *unit);

		    /* copy constructor */
		    ArgumentNode(const ArgumentNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~ArgumentNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitArgumentNode(this); }
		    virtual std::string getNodeType() { return "Argument"; }
		    virtual ArgumentNode* clone() { return new ArgumentNode(*this); }
	};

	/* ast Node for LocalVariable */
	class LocalVariableNode : public ExpressionNode {
		public:
		    /* member variables */
		    IdentifierNode *name;

		    /* constructor */
		    LocalVariableNode(IdentifierNode *name);

		    /* copy constructor */
		    LocalVariableNode(const LocalVariableNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~LocalVariableNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitLocalVariableNode(this); }
		    virtual std::string getNodeType() { return "LocalVariable"; }
		    virtual LocalVariableNode* clone() { return new LocalVariableNode(*this); }
	};

	/* ast Node for LocalListStatement */
	class LocalListStatementNode : public StatementNode {
		public:
		    /* member variables */
		    LocalVariableNodeList *variables;

		    /* constructor */
		    LocalListStatementNode(LocalVariableNodeList *variables);

		    /* copy constructor */
		    LocalListStatementNode(const LocalListStatementNode& obj);

		    virtual ~LocalListStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitLocalListStatementNode(this); }
		    virtual std::string getNodeType() { return "LocalListStatement"; }
		    virtual LocalListStatementNode* clone() { return new LocalListStatementNode(*this); }
	};

	/* ast Node for Limits */
	class LimitsNode : public ExpressionNode {
		public:
		    /* member variables */
		    DoubleNode *min;
		    DoubleNode *max;

		    /* constructor */
		    LimitsNode(DoubleNode *min, DoubleNode *max);

		    /* copy constructor */
		    LimitsNode(const LimitsNode& obj);

		    virtual ~LimitsNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitLimitsNode(this); }
		    virtual std::string getNodeType() { return "Limits"; }
		    virtual LimitsNode* clone() { return new LimitsNode(*this); }
	};

	/* ast Node for NumberRange */
	class NumberRangeNode : public ExpressionNode {
		public:
		    /* member variables */
		    NumberNode *min;
		    NumberNode *max;

		    /* constructor */
		    NumberRangeNode(NumberNode *min, NumberNode *max);

		    /* copy constructor */
		    NumberRangeNode(const NumberRangeNode& obj);

		    virtual ~NumberRangeNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNumberRangeNode(this); }
		    virtual std::string getNodeType() { return "NumberRange"; }
		    virtual NumberRangeNode* clone() { return new NumberRangeNode(*this); }
	};

	/* ast Node for Program */
	class ProgramNode : public ASTNode {
		public:
		    /* member variables */
		    StatementNodeList *statements;
		    BlockNodeList *blocks;
		    void* symtab;

		    /* constructor */
		    ProgramNode(StatementNodeList *statements, BlockNodeList *blocks);

		    /* copy constructor */
		    ProgramNode(const ProgramNode& obj);

		    void addStatement(StatementNode *s) {
		        statements->push_back(s);
		    }

		    void addBlock(BlockNode *s) {
		        blocks->push_back(s);
		    }
		    ProgramNode() {
		        blocks = new BlockNodeList();
		        statements = new StatementNodeList();
		    }


		    virtual ~ProgramNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitProgramNode(this); }
		    virtual std::string getNodeType() { return "Program"; }
		    virtual ProgramNode* clone() { return new ProgramNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for Model */
	class ModelNode : public StatementNode {
		public:
		    /* member variables */
		    StringNode *title;

		    /* constructor */
		    ModelNode(StringNode *title);

		    /* copy constructor */
		    ModelNode(const ModelNode& obj);

		    virtual ~ModelNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitModelNode(this); }
		    virtual std::string getNodeType() { return "Model"; }
		    virtual ModelNode* clone() { return new ModelNode(*this); }
	};

	/* ast Node for Define */
	class DefineNode : public StatementNode {
		public:
		    /* member variables */
		    NameNode *name;
		    IntegerNode *value;

		    /* constructor */
		    DefineNode(NameNode *name, IntegerNode *value);

		    /* copy constructor */
		    DefineNode(const DefineNode& obj);

		    virtual ~DefineNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitDefineNode(this); }
		    virtual std::string getNodeType() { return "Define"; }
		    virtual DefineNode* clone() { return new DefineNode(*this); }
	};

	/* ast Node for Include */
	class IncludeNode : public StatementNode {
		public:
		    /* member variables */
		    StringNode *filename;

		    /* constructor */
		    IncludeNode(StringNode *filename);

		    /* copy constructor */
		    IncludeNode(const IncludeNode& obj);

		    virtual ~IncludeNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitIncludeNode(this); }
		    virtual std::string getNodeType() { return "Include"; }
		    virtual IncludeNode* clone() { return new IncludeNode(*this); }
	};

	/* ast Node for ParamBlock */
	class ParamBlockNode : public BlockNode {
		public:
		    /* member variables */
		    ParamAssignNodeList *statements;
		    void* symtab;

		    /* constructor */
		    ParamBlockNode(ParamAssignNodeList *statements);

		    /* copy constructor */
		    ParamBlockNode(const ParamBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~ParamBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitParamBlockNode(this); }
		    virtual std::string getNodeType() { return "ParamBlock"; }
		    virtual ParamBlockNode* clone() { return new ParamBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for ParamAssign */
	class ParamAssignNode : public StatementNode {
		public:
		    /* member variables */
		    IdentifierNode *name;
		    NumberNode *value;
		    UnitNode *unit;
		    LimitsNode *limit;

		    /* constructor */
		    ParamAssignNode(IdentifierNode *name, NumberNode *value, UnitNode *unit, LimitsNode *limit);

		    /* copy constructor */
		    ParamAssignNode(const ParamAssignNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~ParamAssignNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitParamAssignNode(this); }
		    virtual std::string getNodeType() { return "ParamAssign"; }
		    virtual ParamAssignNode* clone() { return new ParamAssignNode(*this); }
	};

	/* ast Node for StepBlock */
	class StepBlockNode : public BlockNode {
		public:
		    /* member variables */
		    SteppedNodeList *statements;
		    void* symtab;

		    /* constructor */
		    StepBlockNode(SteppedNodeList *statements);

		    /* copy constructor */
		    StepBlockNode(const StepBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~StepBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitStepBlockNode(this); }
		    virtual std::string getNodeType() { return "StepBlock"; }
		    virtual StepBlockNode* clone() { return new StepBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for Stepped */
	class SteppedNode : public StatementNode {
		public:
		    /* member variables */
		    NameNode *name;
		    NumberNodeList *values;
		    UnitNode *unit;

		    /* constructor */
		    SteppedNode(NameNode *name, NumberNodeList *values, UnitNode *unit);

		    /* copy constructor */
		    SteppedNode(const SteppedNode& obj);

		    virtual ~SteppedNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitSteppedNode(this); }
		    virtual std::string getNodeType() { return "Stepped"; }
		    virtual SteppedNode* clone() { return new SteppedNode(*this); }
	};

	/* ast Node for IndependentBlock */
	class IndependentBlockNode : public BlockNode {
		public:
		    /* member variables */
		    IndependentDefNodeList *definitions;
		    void* symtab;

		    /* constructor */
		    IndependentBlockNode(IndependentDefNodeList *definitions);

		    /* copy constructor */
		    IndependentBlockNode(const IndependentBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~IndependentBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitIndependentBlockNode(this); }
		    virtual std::string getNodeType() { return "IndependentBlock"; }
		    virtual IndependentBlockNode* clone() { return new IndependentBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for IndependentDef */
	class IndependentDefNode : public StatementNode {
		public:
		    /* member variables */
		    BooleanNode *sweep;
		    NameNode *name;
		    NumberNode *from;
		    NumberNode *to;
		    IntegerNode *with;
		    NumberNode *opstart;
		    UnitNode *unit;

		    /* constructor */
		    IndependentDefNode(BooleanNode *sweep, NameNode *name, NumberNode *from, NumberNode *to, IntegerNode *with, NumberNode *opstart, UnitNode *unit);

		    /* copy constructor */
		    IndependentDefNode(const IndependentDefNode& obj);

		    virtual ~IndependentDefNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitIndependentDefNode(this); }
		    virtual std::string getNodeType() { return "IndependentDef"; }
		    virtual IndependentDefNode* clone() { return new IndependentDefNode(*this); }
	};

	/* ast Node for DependentDef */
	class DependentDefNode : public StatementNode {
		public:
		    /* member variables */
		    IdentifierNode *name;
		    IntegerNode *index;
		    NumberNode *from;
		    NumberNode *to;
		    NumberNode *opstart;
		    UnitNode *unit;
		    DoubleNode *abstol;

		    /* constructor */
		    DependentDefNode(IdentifierNode *name, IntegerNode *index, NumberNode *from, NumberNode *to, NumberNode *opstart, UnitNode *unit, DoubleNode *abstol);

		    /* copy constructor */
		    DependentDefNode(const DependentDefNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~DependentDefNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitDependentDefNode(this); }
		    virtual std::string getNodeType() { return "DependentDef"; }
		    virtual DependentDefNode* clone() { return new DependentDefNode(*this); }
	};

	/* ast Node for DependentBlock */
	class DependentBlockNode : public BlockNode {
		public:
		    /* member variables */
		    DependentDefNodeList *definitions;
		    void* symtab;

		    /* constructor */
		    DependentBlockNode(DependentDefNodeList *definitions);

		    /* copy constructor */
		    DependentBlockNode(const DependentBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~DependentBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitDependentBlockNode(this); }
		    virtual std::string getNodeType() { return "DependentBlock"; }
		    virtual DependentBlockNode* clone() { return new DependentBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for StateBlock */
	class StateBlockNode : public BlockNode {
		public:
		    /* member variables */
		    DependentDefNodeList *definitions;
		    void* symtab;

		    /* constructor */
		    StateBlockNode(DependentDefNodeList *definitions);

		    /* copy constructor */
		    StateBlockNode(const StateBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~StateBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitStateBlockNode(this); }
		    virtual std::string getNodeType() { return "StateBlock"; }
		    virtual StateBlockNode* clone() { return new StateBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for PlotBlock */
	class PlotBlockNode : public BlockNode {
		public:
		    /* member variables */
		    PlotDeclarationNode *plot;
		    void* symtab;

		    /* constructor */
		    PlotBlockNode(PlotDeclarationNode *plot);

		    /* copy constructor */
		    PlotBlockNode(const PlotBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~PlotBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitPlotBlockNode(this); }
		    virtual std::string getNodeType() { return "PlotBlock"; }
		    virtual PlotBlockNode* clone() { return new PlotBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for PlotDeclaration */
	class PlotDeclarationNode : public StatementNode {
		public:
		    /* member variables */
		    PlotVariableNodeList *pvlist;
		    PlotVariableNode *name;

		    /* constructor */
		    PlotDeclarationNode(PlotVariableNodeList *pvlist, PlotVariableNode *name);

		    /* copy constructor */
		    PlotDeclarationNode(const PlotDeclarationNode& obj);

		    virtual ~PlotDeclarationNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitPlotDeclarationNode(this); }
		    virtual std::string getNodeType() { return "PlotDeclaration"; }
		    virtual PlotDeclarationNode* clone() { return new PlotDeclarationNode(*this); }
	};

	/* ast Node for PlotVariable */
	class PlotVariableNode : public ExpressionNode {
		public:
		    /* member variables */
		    IdentifierNode *name;
		    IntegerNode *index;

		    /* constructor */
		    PlotVariableNode(IdentifierNode *name, IntegerNode *index);

		    /* copy constructor */
		    PlotVariableNode(const PlotVariableNode& obj);

		    virtual ~PlotVariableNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitPlotVariableNode(this); }
		    virtual std::string getNodeType() { return "PlotVariable"; }
		    virtual PlotVariableNode* clone() { return new PlotVariableNode(*this); }
	};

	/* ast Node for InitialBlock */
	class InitialBlockNode : public BlockNode {
		public:
		    /* member variables */
		    StatementBlockNode *statementblock;
		    void* symtab;

		    /* constructor */
		    InitialBlockNode(StatementBlockNode *statementblock);

		    /* copy constructor */
		    InitialBlockNode(const InitialBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~InitialBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitInitialBlockNode(this); }
		    virtual std::string getNodeType() { return "InitialBlock"; }
		    virtual InitialBlockNode* clone() { return new InitialBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for ConstructorBlock */
	class ConstructorBlockNode : public BlockNode {
		public:
		    /* member variables */
		    StatementBlockNode *statementblock;
		    void* symtab;

		    /* constructor */
		    ConstructorBlockNode(StatementBlockNode *statementblock);

		    /* copy constructor */
		    ConstructorBlockNode(const ConstructorBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~ConstructorBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitConstructorBlockNode(this); }
		    virtual std::string getNodeType() { return "ConstructorBlock"; }
		    virtual ConstructorBlockNode* clone() { return new ConstructorBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for DestructorBlock */
	class DestructorBlockNode : public BlockNode {
		public:
		    /* member variables */
		    StatementBlockNode *statementblock;
		    void* symtab;

		    /* constructor */
		    DestructorBlockNode(StatementBlockNode *statementblock);

		    /* copy constructor */
		    DestructorBlockNode(const DestructorBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~DestructorBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitDestructorBlockNode(this); }
		    virtual std::string getNodeType() { return "DestructorBlock"; }
		    virtual DestructorBlockNode* clone() { return new DestructorBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for ConductanceHint */
	class ConductanceHintNode : public StatementNode {
		public:
		    /* member variables */
		    NameNode *conductance;
		    NameNode *ion;

		    /* constructor */
		    ConductanceHintNode(NameNode *conductance, NameNode *ion);

		    /* copy constructor */
		    ConductanceHintNode(const ConductanceHintNode& obj);

		    virtual ~ConductanceHintNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitConductanceHintNode(this); }
		    virtual std::string getNodeType() { return "ConductanceHint"; }
		    virtual ConductanceHintNode* clone() { return new ConductanceHintNode(*this); }
	};

	/* ast Node for ExpressionStatement */
	class ExpressionStatementNode : public StatementNode {
		public:
		    /* member variables */
		    ExpressionNode *expression;

		    /* constructor */
		    ExpressionStatementNode(ExpressionNode *expression);

		    /* copy constructor */
		    ExpressionStatementNode(const ExpressionStatementNode& obj);

		    virtual ~ExpressionStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitExpressionStatementNode(this); }
		    virtual std::string getNodeType() { return "ExpressionStatement"; }
		    virtual ExpressionStatementNode* clone() { return new ExpressionStatementNode(*this); }
	};

	/* ast Node for ProtectStatement */
	class ProtectStatementNode : public StatementNode {
		public:
		    /* member variables */
		    ExpressionNode *expression;

		    /* constructor */
		    ProtectStatementNode(ExpressionNode *expression);

		    /* copy constructor */
		    ProtectStatementNode(const ProtectStatementNode& obj);

		    virtual ~ProtectStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitProtectStatementNode(this); }
		    virtual std::string getNodeType() { return "ProtectStatement"; }
		    virtual ProtectStatementNode* clone() { return new ProtectStatementNode(*this); }
	};

	/* ast Node for StatementBlock */
	class StatementBlockNode : public BlockNode {
		public:
		    /* member variables */
		    StatementNodeList *statements;
		    ModToken *token;
		    void* symtab;

		    /* constructor */
		    StatementBlockNode(StatementNodeList *statements);

		    /* copy constructor */
		    StatementBlockNode(const StatementBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~StatementBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitStatementBlockNode(this); }
		    virtual std::string getNodeType() { return "StatementBlock"; }
		    virtual StatementBlockNode* clone() { return new StatementBlockNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for BinaryOperator */
	class BinaryOperatorNode : public ExpressionNode {
		public:
		    /* member variables */
		    BinaryOp value;

		    /* constructor */
		    BinaryOperatorNode(BinaryOp value);

		    /* copy constructor */
		    BinaryOperatorNode(const BinaryOperatorNode& obj);

		    virtual ~BinaryOperatorNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitBinaryOperatorNode(this); }
		    virtual std::string getNodeType() { return "BinaryOperator"; }
		    virtual BinaryOperatorNode* clone() { return new BinaryOperatorNode(*this); }
		    std::string  eval() { return BinaryOpNames[value]; }
	};

	/* ast Node for UnaryOperator */
	class UnaryOperatorNode : public ExpressionNode {
		public:
		    /* member variables */
		    UnaryOp value;

		    /* constructor */
		    UnaryOperatorNode(UnaryOp value);

		    /* copy constructor */
		    UnaryOperatorNode(const UnaryOperatorNode& obj);

		    virtual ~UnaryOperatorNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitUnaryOperatorNode(this); }
		    virtual std::string getNodeType() { return "UnaryOperator"; }
		    virtual UnaryOperatorNode* clone() { return new UnaryOperatorNode(*this); }
		    std::string  eval() { return UnaryOpNames[value]; }
	};

	/* ast Node for ReactionOperator */
	class ReactionOperatorNode : public ExpressionNode {
		public:
		    /* member variables */
		    ReactionOp value;

		    /* constructor */
		    ReactionOperatorNode(ReactionOp value);

		    /* copy constructor */
		    ReactionOperatorNode(const ReactionOperatorNode& obj);

		    virtual ~ReactionOperatorNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitReactionOperatorNode(this); }
		    virtual std::string getNodeType() { return "ReactionOperator"; }
		    virtual ReactionOperatorNode* clone() { return new ReactionOperatorNode(*this); }
		    std::string  eval() { return ReactionOpNames[value]; }
	};

	/* ast Node for BinaryExpression */
	class BinaryExpressionNode : public ExpressionNode {
		public:
		    /* member variables */
		    ExpressionNode *lhs;
		    BinaryOperatorNode *op;
		    ExpressionNode *rhs;

		    /* constructor */
		    BinaryExpressionNode(ExpressionNode *lhs, BinaryOperatorNode *op, ExpressionNode *rhs);

		    /* copy constructor */
		    BinaryExpressionNode(const BinaryExpressionNode& obj);

		    virtual ~BinaryExpressionNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitBinaryExpressionNode(this); }
		    virtual std::string getNodeType() { return "BinaryExpression"; }
		    virtual BinaryExpressionNode* clone() { return new BinaryExpressionNode(*this); }
	};

	/* ast Node for UnaryExpression */
	class UnaryExpressionNode : public ExpressionNode {
		public:
		    /* member variables */
		    UnaryOperatorNode *op;
		    ExpressionNode *expression;

		    /* constructor */
		    UnaryExpressionNode(UnaryOperatorNode *op, ExpressionNode *expression);

		    /* copy constructor */
		    UnaryExpressionNode(const UnaryExpressionNode& obj);

		    virtual ~UnaryExpressionNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitUnaryExpressionNode(this); }
		    virtual std::string getNodeType() { return "UnaryExpression"; }
		    virtual UnaryExpressionNode* clone() { return new UnaryExpressionNode(*this); }
	};

	/* ast Node for NonLinEuation */
	class NonLinEuationNode : public ExpressionNode {
		public:
		    /* member variables */
		    ExpressionNode *lhs;
		    ExpressionNode *rhs;

		    /* constructor */
		    NonLinEuationNode(ExpressionNode *lhs, ExpressionNode *rhs);

		    /* copy constructor */
		    NonLinEuationNode(const NonLinEuationNode& obj);

		    virtual ~NonLinEuationNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNonLinEuationNode(this); }
		    virtual std::string getNodeType() { return "NonLinEuation"; }
		    virtual NonLinEuationNode* clone() { return new NonLinEuationNode(*this); }
	};

	/* ast Node for LinEquation */
	class LinEquationNode : public ExpressionNode {
		public:
		    /* member variables */
		    ExpressionNode *leftlinexpr;
		    ExpressionNode *linexpr;

		    /* constructor */
		    LinEquationNode(ExpressionNode *leftlinexpr, ExpressionNode *linexpr);

		    /* copy constructor */
		    LinEquationNode(const LinEquationNode& obj);

		    virtual ~LinEquationNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitLinEquationNode(this); }
		    virtual std::string getNodeType() { return "LinEquation"; }
		    virtual LinEquationNode* clone() { return new LinEquationNode(*this); }
	};

	/* ast Node for FunctionCall */
	class FunctionCallNode : public ExpressionNode {
		public:
		    /* member variables */
		    NameNode *name;
		    ExpressionNodeList *arguments;

		    /* constructor */
		    FunctionCallNode(NameNode *name, ExpressionNodeList *arguments);

		    /* copy constructor */
		    FunctionCallNode(const FunctionCallNode& obj);

		    virtual ~FunctionCallNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitFunctionCallNode(this); }
		    virtual std::string getNodeType() { return "FunctionCall"; }
		    virtual FunctionCallNode* clone() { return new FunctionCallNode(*this); }
	};

	/* ast Node for FromStatement */
	class FromStatementNode : public StatementNode {
		public:
		    /* member variables */
		    NameNode *name;
		    ExpressionNode *from;
		    ExpressionNode *to;
		    ExpressionNode *opinc;
		    StatementBlockNode *statementblock;

		    /* constructor */
		    FromStatementNode(NameNode *name, ExpressionNode *from, ExpressionNode *to, ExpressionNode *opinc, StatementBlockNode *statementblock);

		    /* copy constructor */
		    FromStatementNode(const FromStatementNode& obj);

		    virtual ~FromStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitFromStatementNode(this); }
		    virtual std::string getNodeType() { return "FromStatement"; }
		    virtual FromStatementNode* clone() { return new FromStatementNode(*this); }
	};

	/* ast Node for ForAllStatement */
	class ForAllStatementNode : public StatementNode {
		public:
		    /* member variables */
		    NameNode *name;
		    StatementBlockNode *statementblock;

		    /* constructor */
		    ForAllStatementNode(NameNode *name, StatementBlockNode *statementblock);

		    /* copy constructor */
		    ForAllStatementNode(const ForAllStatementNode& obj);

		    virtual ~ForAllStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitForAllStatementNode(this); }
		    virtual std::string getNodeType() { return "ForAllStatement"; }
		    virtual ForAllStatementNode* clone() { return new ForAllStatementNode(*this); }
	};

	/* ast Node for WhileStatement */
	class WhileStatementNode : public StatementNode {
		public:
		    /* member variables */
		    ExpressionNode *condition;
		    StatementBlockNode *statementblock;

		    /* constructor */
		    WhileStatementNode(ExpressionNode *condition, StatementBlockNode *statementblock);

		    /* copy constructor */
		    WhileStatementNode(const WhileStatementNode& obj);

		    virtual ~WhileStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitWhileStatementNode(this); }
		    virtual std::string getNodeType() { return "WhileStatement"; }
		    virtual WhileStatementNode* clone() { return new WhileStatementNode(*this); }
	};

	/* ast Node for IfStatement */
	class IfStatementNode : public StatementNode {
		public:
		    /* member variables */
		    ExpressionNode *condition;
		    StatementBlockNode *statementblock;
		    ElseIfStatementNodeList *elseifs;
		    ElseStatementNode *elses;

		    /* constructor */
		    IfStatementNode(ExpressionNode *condition, StatementBlockNode *statementblock, ElseIfStatementNodeList *elseifs, ElseStatementNode *elses);

		    /* copy constructor */
		    IfStatementNode(const IfStatementNode& obj);

		    virtual ~IfStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitIfStatementNode(this); }
		    virtual std::string getNodeType() { return "IfStatement"; }
		    virtual IfStatementNode* clone() { return new IfStatementNode(*this); }
	};

	/* ast Node for ElseIfStatement */
	class ElseIfStatementNode : public StatementNode {
		public:
		    /* member variables */
		    ExpressionNode *condition;
		    StatementBlockNode *statementblock;

		    /* constructor */
		    ElseIfStatementNode(ExpressionNode *condition, StatementBlockNode *statementblock);

		    /* copy constructor */
		    ElseIfStatementNode(const ElseIfStatementNode& obj);

		    virtual ~ElseIfStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitElseIfStatementNode(this); }
		    virtual std::string getNodeType() { return "ElseIfStatement"; }
		    virtual ElseIfStatementNode* clone() { return new ElseIfStatementNode(*this); }
	};

	/* ast Node for ElseStatement */
	class ElseStatementNode : public StatementNode {
		public:
		    /* member variables */
		    StatementBlockNode *statementblock;

		    /* constructor */
		    ElseStatementNode(StatementBlockNode *statementblock);

		    /* copy constructor */
		    ElseStatementNode(const ElseStatementNode& obj);

		    virtual ~ElseStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitElseStatementNode(this); }
		    virtual std::string getNodeType() { return "ElseStatement"; }
		    virtual ElseStatementNode* clone() { return new ElseStatementNode(*this); }
	};

	/* ast Node for DerivativeBlock */
	class DerivativeBlockNode : public BlockNode {
		public:
		    /* member variables */
		    NameNode *name;
		    StatementBlockNode *statementblock;
		    ModToken *token;
		    void* symtab;

		    /* constructor */
		    DerivativeBlockNode(NameNode *name, StatementBlockNode *statementblock);

		    /* copy constructor */
		    DerivativeBlockNode(const DerivativeBlockNode& obj);

		    virtual std::string getName() { return name->getName(); }

		    virtual ~DerivativeBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitDerivativeBlockNode(this); }
		    virtual std::string getNodeType() { return "DerivativeBlock"; }
		    virtual DerivativeBlockNode* clone() { return new DerivativeBlockNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for LinearBlock */
	class LinearBlockNode : public BlockNode {
		public:
		    /* member variables */
		    NameNode *name;
		    NameNodeList *solvefor;
		    StatementBlockNode *statementblock;
		    ModToken *token;
		    void* symtab;

		    /* constructor */
		    LinearBlockNode(NameNode *name, NameNodeList *solvefor, StatementBlockNode *statementblock);

		    /* copy constructor */
		    LinearBlockNode(const LinearBlockNode& obj);

		    virtual std::string getName() { return name->getName(); }

		    virtual ~LinearBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitLinearBlockNode(this); }
		    virtual std::string getNodeType() { return "LinearBlock"; }
		    virtual LinearBlockNode* clone() { return new LinearBlockNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for NonLinearBlock */
	class NonLinearBlockNode : public BlockNode {
		public:
		    /* member variables */
		    NameNode *name;
		    NameNodeList *solvefor;
		    StatementBlockNode *statementblock;
		    ModToken *token;
		    void* symtab;

		    /* constructor */
		    NonLinearBlockNode(NameNode *name, NameNodeList *solvefor, StatementBlockNode *statementblock);

		    /* copy constructor */
		    NonLinearBlockNode(const NonLinearBlockNode& obj);

		    virtual std::string getName() { return name->getName(); }

		    virtual ~NonLinearBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNonLinearBlockNode(this); }
		    virtual std::string getNodeType() { return "NonLinearBlock"; }
		    virtual NonLinearBlockNode* clone() { return new NonLinearBlockNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for DiscreteBlock */
	class DiscreteBlockNode : public BlockNode {
		public:
		    /* member variables */
		    NameNode *name;
		    StatementBlockNode *statementblock;
		    ModToken *token;
		    void* symtab;

		    /* constructor */
		    DiscreteBlockNode(NameNode *name, StatementBlockNode *statementblock);

		    /* copy constructor */
		    DiscreteBlockNode(const DiscreteBlockNode& obj);

		    virtual std::string getName() { return name->getName(); }

		    virtual ~DiscreteBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitDiscreteBlockNode(this); }
		    virtual std::string getNodeType() { return "DiscreteBlock"; }
		    virtual DiscreteBlockNode* clone() { return new DiscreteBlockNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for PartialBlock */
	class PartialBlockNode : public BlockNode {
		public:
		    /* member variables */
		    NameNode *name;
		    StatementBlockNode *statementblock;
		    ModToken *token;
		    void* symtab;

		    /* constructor */
		    PartialBlockNode(NameNode *name, StatementBlockNode *statementblock);

		    /* copy constructor */
		    PartialBlockNode(const PartialBlockNode& obj);

		    virtual std::string getName() { return name->getName(); }

		    virtual ~PartialBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitPartialBlockNode(this); }
		    virtual std::string getNodeType() { return "PartialBlock"; }
		    virtual PartialBlockNode* clone() { return new PartialBlockNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for PartialEquation */
	class PartialEquationNode : public StatementNode {
		public:
		    /* member variables */
		    PrimeNameNode *prime;
		    NameNode *name1;
		    NameNode *name2;
		    NameNode *name3;

		    /* constructor */
		    PartialEquationNode(PrimeNameNode *prime, NameNode *name1, NameNode *name2, NameNode *name3);

		    /* copy constructor */
		    PartialEquationNode(const PartialEquationNode& obj);

		    virtual ~PartialEquationNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitPartialEquationNode(this); }
		    virtual std::string getNodeType() { return "PartialEquation"; }
		    virtual PartialEquationNode* clone() { return new PartialEquationNode(*this); }
	};

	/* ast Node for FirstLastTypeIndex */
	class FirstLastTypeIndexNode : public ExpressionNode {
		public:
		    /* member variables */
		    FirstLastType value;

		    /* constructor */
		    FirstLastTypeIndexNode(FirstLastType value);

		    /* copy constructor */
		    FirstLastTypeIndexNode(const FirstLastTypeIndexNode& obj);

		    virtual ~FirstLastTypeIndexNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitFirstLastTypeIndexNode(this); }
		    virtual std::string getNodeType() { return "FirstLastTypeIndex"; }
		    virtual FirstLastTypeIndexNode* clone() { return new FirstLastTypeIndexNode(*this); }
		    std::string  eval() { return FirstLastTypeNames[value]; }
	};

	/* ast Node for PartialBoundary */
	class PartialBoundaryNode : public StatementNode {
		public:
		    /* member variables */
		    NameNode *del;
		    IdentifierNode *name;
		    FirstLastTypeIndexNode *index;
		    ExpressionNode *expression;
		    NameNode *name1;
		    NameNode *del2;
		    NameNode *name2;
		    NameNode *name3;

		    /* constructor */
		    PartialBoundaryNode(NameNode *del, IdentifierNode *name, FirstLastTypeIndexNode *index, ExpressionNode *expression, NameNode *name1, NameNode *del2, NameNode *name2, NameNode *name3);

		    /* copy constructor */
		    PartialBoundaryNode(const PartialBoundaryNode& obj);

		    virtual ~PartialBoundaryNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitPartialBoundaryNode(this); }
		    virtual std::string getNodeType() { return "PartialBoundary"; }
		    virtual PartialBoundaryNode* clone() { return new PartialBoundaryNode(*this); }
	};

	/* ast Node for FunctionTableBlock */
	class FunctionTableBlockNode : public BlockNode {
		public:
		    /* member variables */
		    NameNode *name;
		    ArgumentNodeList *arguments;
		    UnitNode *unit;
		    ModToken *token;
		    void* symtab;

		    /* constructor */
		    FunctionTableBlockNode(NameNode *name, ArgumentNodeList *arguments, UnitNode *unit);

		    /* copy constructor */
		    FunctionTableBlockNode(const FunctionTableBlockNode& obj);

		    virtual std::string getName() { return name->getName(); }

		    virtual ~FunctionTableBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitFunctionTableBlockNode(this); }
		    virtual std::string getNodeType() { return "FunctionTableBlock"; }
		    virtual FunctionTableBlockNode* clone() { return new FunctionTableBlockNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for FunctionBlock */
	class FunctionBlockNode : public BlockNode {
		public:
		    /* member variables */
		    NameNode *name;
		    ArgumentNodeList *arguments;
		    UnitNode *unit;
		    StatementBlockNode *statementblock;
		    ModToken *token;
		    void* symtab;

		    /* constructor */
		    FunctionBlockNode(NameNode *name, ArgumentNodeList *arguments, UnitNode *unit, StatementBlockNode *statementblock);

		    /* copy constructor */
		    FunctionBlockNode(const FunctionBlockNode& obj);

		    virtual std::string getName() { return name->getName(); }

		    virtual ~FunctionBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitFunctionBlockNode(this); }
		    virtual std::string getNodeType() { return "FunctionBlock"; }
		    virtual FunctionBlockNode* clone() { return new FunctionBlockNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for ProcedureBlock */
	class ProcedureBlockNode : public BlockNode {
		public:
		    /* member variables */
		    NameNode *name;
		    ArgumentNodeList *arguments;
		    UnitNode *unit;
		    StatementBlockNode *statementblock;
		    ModToken *token;
		    void* symtab;

		    /* constructor */
		    ProcedureBlockNode(NameNode *name, ArgumentNodeList *arguments, UnitNode *unit, StatementBlockNode *statementblock);

		    /* copy constructor */
		    ProcedureBlockNode(const ProcedureBlockNode& obj);

		    virtual std::string getName() { return name->getName(); }

		    virtual ~ProcedureBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitProcedureBlockNode(this); }
		    virtual std::string getNodeType() { return "ProcedureBlock"; }
		    virtual ProcedureBlockNode* clone() { return new ProcedureBlockNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for NetReceiveBlock */
	class NetReceiveBlockNode : public BlockNode {
		public:
		    /* member variables */
		    ArgumentNodeList *arguments;
		    StatementBlockNode *statementblock;
		    void* symtab;

		    /* constructor */
		    NetReceiveBlockNode(ArgumentNodeList *arguments, StatementBlockNode *statementblock);

		    /* copy constructor */
		    NetReceiveBlockNode(const NetReceiveBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~NetReceiveBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNetReceiveBlockNode(this); }
		    virtual std::string getNodeType() { return "NetReceiveBlock"; }
		    virtual NetReceiveBlockNode* clone() { return new NetReceiveBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for SolveBlock */
	class SolveBlockNode : public BlockNode {
		public:
		    /* member variables */
		    NameNode *name;
		    NameNode *method;
		    StatementBlockNode *ifsolerr;
		    void* symtab;

		    /* constructor */
		    SolveBlockNode(NameNode *name, NameNode *method, StatementBlockNode *ifsolerr);

		    /* copy constructor */
		    SolveBlockNode(const SolveBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~SolveBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitSolveBlockNode(this); }
		    virtual std::string getNodeType() { return "SolveBlock"; }
		    virtual SolveBlockNode* clone() { return new SolveBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for BreakpointBlock */
	class BreakpointBlockNode : public BlockNode {
		public:
		    /* member variables */
		    StatementBlockNode *statementblock;
		    void* symtab;

		    /* constructor */
		    BreakpointBlockNode(StatementBlockNode *statementblock);

		    /* copy constructor */
		    BreakpointBlockNode(const BreakpointBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~BreakpointBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitBreakpointBlockNode(this); }
		    virtual std::string getNodeType() { return "BreakpointBlock"; }
		    virtual BreakpointBlockNode* clone() { return new BreakpointBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for TerminalBlock */
	class TerminalBlockNode : public BlockNode {
		public:
		    /* member variables */
		    StatementBlockNode *statementblock;
		    void* symtab;

		    /* constructor */
		    TerminalBlockNode(StatementBlockNode *statementblock);

		    /* copy constructor */
		    TerminalBlockNode(const TerminalBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~TerminalBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitTerminalBlockNode(this); }
		    virtual std::string getNodeType() { return "TerminalBlock"; }
		    virtual TerminalBlockNode* clone() { return new TerminalBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for BeforeBlock */
	class BeforeBlockNode : public BlockNode {
		public:
		    /* member variables */
		    BABlockNode *block;
		    void* symtab;

		    /* constructor */
		    BeforeBlockNode(BABlockNode *block);

		    /* copy constructor */
		    BeforeBlockNode(const BeforeBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~BeforeBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitBeforeBlockNode(this); }
		    virtual std::string getNodeType() { return "BeforeBlock"; }
		    virtual BeforeBlockNode* clone() { return new BeforeBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for AfterBlock */
	class AfterBlockNode : public BlockNode {
		public:
		    /* member variables */
		    BABlockNode *block;
		    void* symtab;

		    /* constructor */
		    AfterBlockNode(BABlockNode *block);

		    /* copy constructor */
		    AfterBlockNode(const AfterBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~AfterBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitAfterBlockNode(this); }
		    virtual std::string getNodeType() { return "AfterBlock"; }
		    virtual AfterBlockNode* clone() { return new AfterBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for BABlockType */
	class BABlockTypeNode : public ExpressionNode {
		public:
		    /* member variables */
		    BAType value;

		    /* constructor */
		    BABlockTypeNode(BAType value);

		    /* copy constructor */
		    BABlockTypeNode(const BABlockTypeNode& obj);

		    virtual ~BABlockTypeNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitBABlockTypeNode(this); }
		    virtual std::string getNodeType() { return "BABlockType"; }
		    virtual BABlockTypeNode* clone() { return new BABlockTypeNode(*this); }
		    std::string  eval() { return BATypeNames[value]; }
	};

	/* ast Node for BABlock */
	class BABlockNode : public BlockNode {
		public:
		    /* member variables */
		    BABlockTypeNode *type;
		    StatementBlockNode *statementblock;
		    void* symtab;

		    /* constructor */
		    BABlockNode(BABlockTypeNode *type, StatementBlockNode *statementblock);

		    /* copy constructor */
		    BABlockNode(const BABlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~BABlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitBABlockNode(this); }
		    virtual std::string getNodeType() { return "BABlock"; }
		    virtual BABlockNode* clone() { return new BABlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for WatchStatement */
	class WatchStatementNode : public StatementNode {
		public:
		    /* member variables */
		    WatchNodeList *statements;

		    /* constructor */
		    WatchStatementNode(WatchNodeList *statements);

		    /* copy constructor */
		    WatchStatementNode(const WatchStatementNode& obj);

		    void addWatch(WatchNode *s) {
		        statements->push_back(s);
		    }

		    virtual ~WatchStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitWatchStatementNode(this); }
		    virtual std::string getNodeType() { return "WatchStatement"; }
		    virtual WatchStatementNode* clone() { return new WatchStatementNode(*this); }
	};

	/* ast Node for Watch */
	class WatchNode : public ExpressionNode {
		public:
		    /* member variables */
		    ExpressionNode *expression;
		    ExpressionNode *value;

		    /* constructor */
		    WatchNode(ExpressionNode *expression, ExpressionNode *value);

		    /* copy constructor */
		    WatchNode(const WatchNode& obj);

		    virtual ~WatchNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitWatchNode(this); }
		    virtual std::string getNodeType() { return "Watch"; }
		    virtual WatchNode* clone() { return new WatchNode(*this); }
	};

	/* ast Node for ForNetcon */
	class ForNetconNode : public BlockNode {
		public:
		    /* member variables */
		    ArgumentNodeList *arguments;
		    StatementBlockNode *statementblock;
		    void* symtab;

		    /* constructor */
		    ForNetconNode(ArgumentNodeList *arguments, StatementBlockNode *statementblock);

		    /* copy constructor */
		    ForNetconNode(const ForNetconNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~ForNetconNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitForNetconNode(this); }
		    virtual std::string getNodeType() { return "ForNetcon"; }
		    virtual ForNetconNode* clone() { return new ForNetconNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for MutexLock */
	class MutexLockNode : public StatementNode {
		public:

		    virtual ~MutexLockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitMutexLockNode(this); }
		    virtual std::string getNodeType() { return "MutexLock"; }
		    virtual MutexLockNode* clone() { return new MutexLockNode(*this); }
	};

	/* ast Node for MutexUnlock */
	class MutexUnlockNode : public StatementNode {
		public:

		    virtual ~MutexUnlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitMutexUnlockNode(this); }
		    virtual std::string getNodeType() { return "MutexUnlock"; }
		    virtual MutexUnlockNode* clone() { return new MutexUnlockNode(*this); }
	};

	/* ast Node for Reset */
	class ResetNode : public StatementNode {
		public:

		    virtual ~ResetNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitResetNode(this); }
		    virtual std::string getNodeType() { return "Reset"; }
		    virtual ResetNode* clone() { return new ResetNode(*this); }
	};

	/* ast Node for Sens */
	class SensNode : public StatementNode {
		public:
		    /* member variables */
		    VarNameNodeList *senslist;

		    /* constructor */
		    SensNode(VarNameNodeList *senslist);

		    /* copy constructor */
		    SensNode(const SensNode& obj);

		    virtual ~SensNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitSensNode(this); }
		    virtual std::string getNodeType() { return "Sens"; }
		    virtual SensNode* clone() { return new SensNode(*this); }
	};

	/* ast Node for Conserve */
	class ConserveNode : public StatementNode {
		public:
		    /* member variables */
		    ExpressionNode *react;
		    ExpressionNode *expr;

		    /* constructor */
		    ConserveNode(ExpressionNode *react, ExpressionNode *expr);

		    /* copy constructor */
		    ConserveNode(const ConserveNode& obj);

		    virtual ~ConserveNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitConserveNode(this); }
		    virtual std::string getNodeType() { return "Conserve"; }
		    virtual ConserveNode* clone() { return new ConserveNode(*this); }
	};

	/* ast Node for Compartment */
	class CompartmentNode : public StatementNode {
		public:
		    /* member variables */
		    NameNode *name;
		    ExpressionNode *expression;
		    NameNodeList *names;

		    /* constructor */
		    CompartmentNode(NameNode *name, ExpressionNode *expression, NameNodeList *names);

		    /* copy constructor */
		    CompartmentNode(const CompartmentNode& obj);

		    virtual ~CompartmentNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitCompartmentNode(this); }
		    virtual std::string getNodeType() { return "Compartment"; }
		    virtual CompartmentNode* clone() { return new CompartmentNode(*this); }
	};

	/* ast Node for LDifuse */
	class LDifuseNode : public StatementNode {
		public:
		    /* member variables */
		    NameNode *name;
		    ExpressionNode *expression;
		    NameNodeList *names;

		    /* constructor */
		    LDifuseNode(NameNode *name, ExpressionNode *expression, NameNodeList *names);

		    /* copy constructor */
		    LDifuseNode(const LDifuseNode& obj);

		    virtual ~LDifuseNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitLDifuseNode(this); }
		    virtual std::string getNodeType() { return "LDifuse"; }
		    virtual LDifuseNode* clone() { return new LDifuseNode(*this); }
	};

	/* ast Node for KineticBlock */
	class KineticBlockNode : public BlockNode {
		public:
		    /* member variables */
		    NameNode *name;
		    NameNodeList *solvefor;
		    StatementBlockNode *statementblock;
		    ModToken *token;
		    void* symtab;

		    /* constructor */
		    KineticBlockNode(NameNode *name, NameNodeList *solvefor, StatementBlockNode *statementblock);

		    /* copy constructor */
		    KineticBlockNode(const KineticBlockNode& obj);

		    virtual std::string getName() { return name->getName(); }

		    virtual ~KineticBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitKineticBlockNode(this); }
		    virtual std::string getNodeType() { return "KineticBlock"; }
		    virtual KineticBlockNode* clone() { return new KineticBlockNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for ReactionStatement */
	class ReactionStatementNode : public StatementNode {
		public:
		    /* member variables */
		    ExpressionNode *react1;
		    ReactionOperatorNode *op;
		    ExpressionNode *react2;
		    ExpressionNode *expr1;
		    ExpressionNode *expr2;

		    /* constructor */
		    ReactionStatementNode(ExpressionNode *react1, ReactionOperatorNode *op, ExpressionNode *react2, ExpressionNode *expr1, ExpressionNode *expr2);

		    /* copy constructor */
		    ReactionStatementNode(const ReactionStatementNode& obj);

		    virtual ~ReactionStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitReactionStatementNode(this); }
		    virtual std::string getNodeType() { return "ReactionStatement"; }
		    virtual ReactionStatementNode* clone() { return new ReactionStatementNode(*this); }
	};

	/* ast Node for ReactVarName */
	class ReactVarNameNode : public IdentifierNode {
		public:
		    /* member variables */
		    IntegerNode *value;
		    VarNameNode *name;

		    /* constructor */
		    ReactVarNameNode(IntegerNode *value, VarNameNode *name);

		    /* copy constructor */
		    ReactVarNameNode(const ReactVarNameNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~ReactVarNameNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitReactVarNameNode(this); }
		    virtual std::string getNodeType() { return "ReactVarName"; }
		    virtual ReactVarNameNode* clone() { return new ReactVarNameNode(*this); }
	};

	/* ast Node for LagStatement */
	class LagStatementNode : public StatementNode {
		public:
		    /* member variables */
		    IdentifierNode *name;
		    NameNode *byname;

		    /* constructor */
		    LagStatementNode(IdentifierNode *name, NameNode *byname);

		    /* copy constructor */
		    LagStatementNode(const LagStatementNode& obj);

		    virtual ~LagStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitLagStatementNode(this); }
		    virtual std::string getNodeType() { return "LagStatement"; }
		    virtual LagStatementNode* clone() { return new LagStatementNode(*this); }
	};

	/* ast Node for QueueStatement */
	class QueueStatementNode : public StatementNode {
		public:
		    /* member variables */
		    QueueExpressionTypeNode *qype;
		    IdentifierNode *name;

		    /* constructor */
		    QueueStatementNode(QueueExpressionTypeNode *qype, IdentifierNode *name);

		    /* copy constructor */
		    QueueStatementNode(const QueueStatementNode& obj);

		    virtual ~QueueStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitQueueStatementNode(this); }
		    virtual std::string getNodeType() { return "QueueStatement"; }
		    virtual QueueStatementNode* clone() { return new QueueStatementNode(*this); }
	};

	/* ast Node for QueueExpressionType */
	class QueueExpressionTypeNode : public ExpressionNode {
		public:
		    /* member variables */
		    QueueType value;

		    /* constructor */
		    QueueExpressionTypeNode(QueueType value);

		    /* copy constructor */
		    QueueExpressionTypeNode(const QueueExpressionTypeNode& obj);

		    virtual ~QueueExpressionTypeNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitQueueExpressionTypeNode(this); }
		    virtual std::string getNodeType() { return "QueueExpressionType"; }
		    virtual QueueExpressionTypeNode* clone() { return new QueueExpressionTypeNode(*this); }
		    std::string  eval() { return QueueTypeNames[value]; }
	};

	/* ast Node for MatchBlock */
	class MatchBlockNode : public BlockNode {
		public:
		    /* member variables */
		    MatchNodeList *matchs;
		    void* symtab;

		    /* constructor */
		    MatchBlockNode(MatchNodeList *matchs);

		    /* copy constructor */
		    MatchBlockNode(const MatchBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~MatchBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitMatchBlockNode(this); }
		    virtual std::string getNodeType() { return "MatchBlock"; }
		    virtual MatchBlockNode* clone() { return new MatchBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for Match */
	class MatchNode : public ExpressionNode {
		public:
		    /* member variables */
		    IdentifierNode *name;
		    ExpressionNode *expression;

		    /* constructor */
		    MatchNode(IdentifierNode *name, ExpressionNode *expression);

		    /* copy constructor */
		    MatchNode(const MatchNode& obj);

		    virtual ~MatchNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitMatchNode(this); }
		    virtual std::string getNodeType() { return "Match"; }
		    virtual MatchNode* clone() { return new MatchNode(*this); }
	};

	/* ast Node for UnitBlock */
	class UnitBlockNode : public BlockNode {
		public:
		    /* member variables */
		    ExpressionNodeList *definitions;
		    void* symtab;

		    /* constructor */
		    UnitBlockNode(ExpressionNodeList *definitions);

		    /* copy constructor */
		    UnitBlockNode(const UnitBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~UnitBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitUnitBlockNode(this); }
		    virtual std::string getNodeType() { return "UnitBlock"; }
		    virtual UnitBlockNode* clone() { return new UnitBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for UnitDef */
	class UnitDefNode : public ExpressionNode {
		public:
		    /* member variables */
		    UnitNode *unit1;
		    UnitNode *unit2;

		    /* constructor */
		    UnitDefNode(UnitNode *unit1, UnitNode *unit2);

		    /* copy constructor */
		    UnitDefNode(const UnitDefNode& obj);

		    virtual std::string getName() { return unit1->getName(); }
		    virtual ModToken* getToken() { return unit1->getToken(); }

		    virtual ~UnitDefNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitUnitDefNode(this); }
		    virtual std::string getNodeType() { return "UnitDef"; }
		    virtual UnitDefNode* clone() { return new UnitDefNode(*this); }
	};

	/* ast Node for Factordef */
	class FactordefNode : public ExpressionNode {
		public:
		    /* member variables */
		    NameNode *name;
		    DoubleNode *value;
		    UnitNode *unit1;
		    BooleanNode *gt;
		    UnitNode *unit2;
		    ModToken *token;

		    /* constructor */
		    FactordefNode(NameNode *name, DoubleNode *value, UnitNode *unit1, BooleanNode *gt, UnitNode *unit2);

		    /* copy constructor */
		    FactordefNode(const FactordefNode& obj);

		    virtual std::string getName() { return name->getName(); }

		    virtual ~FactordefNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitFactordefNode(this); }
		    virtual std::string getNodeType() { return "Factordef"; }
		    virtual FactordefNode* clone() { return new FactordefNode(*this); }
		    virtual ModToken *getToken() { return token; }
		    virtual void setToken(ModToken *tok) { token = tok; }
	};

	/* ast Node for ConstantStatement */
	class ConstantStatementNode : public StatementNode {
		public:
		    /* member variables */
		    NameNode *name;
		    NumberNode *value;
		    UnitNode *unit;

		    /* constructor */
		    ConstantStatementNode(NameNode *name, NumberNode *value, UnitNode *unit);

		    /* copy constructor */
		    ConstantStatementNode(const ConstantStatementNode& obj);

		    virtual ~ConstantStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitConstantStatementNode(this); }
		    virtual std::string getNodeType() { return "ConstantStatement"; }
		    virtual ConstantStatementNode* clone() { return new ConstantStatementNode(*this); }
	};

	/* ast Node for ConstantBlock */
	class ConstantBlockNode : public BlockNode {
		public:
		    /* member variables */
		    ConstantStatementNodeList *statements;
		    void* symtab;

		    /* constructor */
		    ConstantBlockNode(ConstantStatementNodeList *statements);

		    /* copy constructor */
		    ConstantBlockNode(const ConstantBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~ConstantBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitConstantBlockNode(this); }
		    virtual std::string getNodeType() { return "ConstantBlock"; }
		    virtual ConstantBlockNode* clone() { return new ConstantBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for TableStatement */
	class TableStatementNode : public StatementNode {
		public:
		    /* member variables */
		    NameNodeList *tablst;
		    NameNodeList *dependlst;
		    ExpressionNode *from;
		    ExpressionNode *to;
		    IntegerNode *with;

		    /* constructor */
		    TableStatementNode(NameNodeList *tablst, NameNodeList *dependlst, ExpressionNode *from, ExpressionNode *to, IntegerNode *with);

		    /* copy constructor */
		    TableStatementNode(const TableStatementNode& obj);

		    virtual ~TableStatementNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitTableStatementNode(this); }
		    virtual std::string getNodeType() { return "TableStatement"; }
		    virtual TableStatementNode* clone() { return new TableStatementNode(*this); }
	};

	/* ast Node for NeuronBlock */
	class NeuronBlockNode : public BlockNode {
		public:
		    /* member variables */
		    StatementBlockNode *statementblock;
		    void* symtab;

		    /* constructor */
		    NeuronBlockNode(StatementBlockNode *statementblock);

		    /* copy constructor */
		    NeuronBlockNode(const NeuronBlockNode& obj);
		    std::string getName() { return getNodeType(); }

		    virtual ~NeuronBlockNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNeuronBlockNode(this); }
		    virtual std::string getNodeType() { return "NeuronBlock"; }
		    virtual NeuronBlockNode* clone() { return new NeuronBlockNode(*this); }
		    virtual void setBlockSymbolTable(void *s) { symtab = s; }
		    virtual void* getBlockSymbolTable() { return symtab; }
	};

	/* ast Node for ReadIonVar */
	class ReadIonVarNode : public IdentifierNode {
		public:
		    /* member variables */
		    NameNode *name;

		    /* constructor */
		    ReadIonVarNode(NameNode *name);

		    /* copy constructor */
		    ReadIonVarNode(const ReadIonVarNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~ReadIonVarNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitReadIonVarNode(this); }
		    virtual std::string getNodeType() { return "ReadIonVar"; }
		    virtual ReadIonVarNode* clone() { return new ReadIonVarNode(*this); }
	};

	/* ast Node for WriteIonVar */
	class WriteIonVarNode : public IdentifierNode {
		public:
		    /* member variables */
		    NameNode *name;

		    /* constructor */
		    WriteIonVarNode(NameNode *name);

		    /* copy constructor */
		    WriteIonVarNode(const WriteIonVarNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~WriteIonVarNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitWriteIonVarNode(this); }
		    virtual std::string getNodeType() { return "WriteIonVar"; }
		    virtual WriteIonVarNode* clone() { return new WriteIonVarNode(*this); }
	};

	/* ast Node for NonspeCurVar */
	class NonspeCurVarNode : public IdentifierNode {
		public:
		    /* member variables */
		    NameNode *name;

		    /* constructor */
		    NonspeCurVarNode(NameNode *name);

		    /* copy constructor */
		    NonspeCurVarNode(const NonspeCurVarNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~NonspeCurVarNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNonspeCurVarNode(this); }
		    virtual std::string getNodeType() { return "NonspeCurVar"; }
		    virtual NonspeCurVarNode* clone() { return new NonspeCurVarNode(*this); }
	};

	/* ast Node for ElectrodeCurVar */
	class ElectrodeCurVarNode : public IdentifierNode {
		public:
		    /* member variables */
		    NameNode *name;

		    /* constructor */
		    ElectrodeCurVarNode(NameNode *name);

		    /* copy constructor */
		    ElectrodeCurVarNode(const ElectrodeCurVarNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~ElectrodeCurVarNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitElectrodeCurVarNode(this); }
		    virtual std::string getNodeType() { return "ElectrodeCurVar"; }
		    virtual ElectrodeCurVarNode* clone() { return new ElectrodeCurVarNode(*this); }
	};

	/* ast Node for SectionVar */
	class SectionVarNode : public IdentifierNode {
		public:
		    /* member variables */
		    NameNode *name;

		    /* constructor */
		    SectionVarNode(NameNode *name);

		    /* copy constructor */
		    SectionVarNode(const SectionVarNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~SectionVarNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitSectionVarNode(this); }
		    virtual std::string getNodeType() { return "SectionVar"; }
		    virtual SectionVarNode* clone() { return new SectionVarNode(*this); }
	};

	/* ast Node for RangeVar */
	class RangeVarNode : public IdentifierNode {
		public:
		    /* member variables */
		    NameNode *name;

		    /* constructor */
		    RangeVarNode(NameNode *name);

		    /* copy constructor */
		    RangeVarNode(const RangeVarNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~RangeVarNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitRangeVarNode(this); }
		    virtual std::string getNodeType() { return "RangeVar"; }
		    virtual RangeVarNode* clone() { return new RangeVarNode(*this); }
	};

	/* ast Node for GlobalVar */
	class GlobalVarNode : public IdentifierNode {
		public:
		    /* member variables */
		    NameNode *name;

		    /* constructor */
		    GlobalVarNode(NameNode *name);

		    /* copy constructor */
		    GlobalVarNode(const GlobalVarNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~GlobalVarNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitGlobalVarNode(this); }
		    virtual std::string getNodeType() { return "GlobalVar"; }
		    virtual GlobalVarNode* clone() { return new GlobalVarNode(*this); }
	};

	/* ast Node for PointerVar */
	class PointerVarNode : public IdentifierNode {
		public:
		    /* member variables */
		    NameNode *name;

		    /* constructor */
		    PointerVarNode(NameNode *name);

		    /* copy constructor */
		    PointerVarNode(const PointerVarNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~PointerVarNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitPointerVarNode(this); }
		    virtual std::string getNodeType() { return "PointerVar"; }
		    virtual PointerVarNode* clone() { return new PointerVarNode(*this); }
	};

	/* ast Node for BbcorePointerVar */
	class BbcorePointerVarNode : public IdentifierNode {
		public:
		    /* member variables */
		    NameNode *name;

		    /* constructor */
		    BbcorePointerVarNode(NameNode *name);

		    /* copy constructor */
		    BbcorePointerVarNode(const BbcorePointerVarNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~BbcorePointerVarNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitBbcorePointerVarNode(this); }
		    virtual std::string getNodeType() { return "BbcorePointerVar"; }
		    virtual BbcorePointerVarNode* clone() { return new BbcorePointerVarNode(*this); }
	};

	/* ast Node for ExternVar */
	class ExternVarNode : public IdentifierNode {
		public:
		    /* member variables */
		    NameNode *name;

		    /* constructor */
		    ExternVarNode(NameNode *name);

		    /* copy constructor */
		    ExternVarNode(const ExternVarNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~ExternVarNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitExternVarNode(this); }
		    virtual std::string getNodeType() { return "ExternVar"; }
		    virtual ExternVarNode* clone() { return new ExternVarNode(*this); }
	};

	/* ast Node for ThreadsafeVar */
	class ThreadsafeVarNode : public IdentifierNode {
		public:
		    /* member variables */
		    NameNode *name;

		    /* constructor */
		    ThreadsafeVarNode(NameNode *name);

		    /* copy constructor */
		    ThreadsafeVarNode(const ThreadsafeVarNode& obj);

		    virtual std::string getName() { return name->getName(); }
		    virtual ModToken* getToken() { return name->getToken(); }

		    virtual ~ThreadsafeVarNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitThreadsafeVarNode(this); }
		    virtual std::string getNodeType() { return "ThreadsafeVar"; }
		    virtual ThreadsafeVarNode* clone() { return new ThreadsafeVarNode(*this); }
	};

	/* ast Node for NrnSuffix */
	class NrnSuffixNode : public StatementNode {
		public:
		    /* member variables */
		    NameNode *type;
		    NameNode *name;

		    /* constructor */
		    NrnSuffixNode(NameNode *type, NameNode *name);

		    /* copy constructor */
		    NrnSuffixNode(const NrnSuffixNode& obj);

		    virtual ~NrnSuffixNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNrnSuffixNode(this); }
		    virtual std::string getNodeType() { return "NrnSuffix"; }
		    virtual NrnSuffixNode* clone() { return new NrnSuffixNode(*this); }
	};

	/* ast Node for NrnUseion */
	class NrnUseionNode : public StatementNode {
		public:
		    /* member variables */
		    NameNode *name;
		    ReadIonVarNodeList *readlist;
		    WriteIonVarNodeList *writelist;
		    ValenceNode *valence;

		    /* constructor */
		    NrnUseionNode(NameNode *name, ReadIonVarNodeList *readlist, WriteIonVarNodeList *writelist, ValenceNode *valence);

		    /* copy constructor */
		    NrnUseionNode(const NrnUseionNode& obj);

		    virtual ~NrnUseionNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNrnUseionNode(this); }
		    virtual std::string getNodeType() { return "NrnUseion"; }
		    virtual NrnUseionNode* clone() { return new NrnUseionNode(*this); }
	};

	/* ast Node for NrnNonspecific */
	class NrnNonspecificNode : public StatementNode {
		public:
		    /* member variables */
		    NonspeCurVarNodeList *currents;

		    /* constructor */
		    NrnNonspecificNode(NonspeCurVarNodeList *currents);

		    /* copy constructor */
		    NrnNonspecificNode(const NrnNonspecificNode& obj);

		    virtual ~NrnNonspecificNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNrnNonspecificNode(this); }
		    virtual std::string getNodeType() { return "NrnNonspecific"; }
		    virtual NrnNonspecificNode* clone() { return new NrnNonspecificNode(*this); }
	};

	/* ast Node for NrnElctrodeCurrent */
	class NrnElctrodeCurrentNode : public StatementNode {
		public:
		    /* member variables */
		    ElectrodeCurVarNodeList *ecurrents;

		    /* constructor */
		    NrnElctrodeCurrentNode(ElectrodeCurVarNodeList *ecurrents);

		    /* copy constructor */
		    NrnElctrodeCurrentNode(const NrnElctrodeCurrentNode& obj);

		    virtual ~NrnElctrodeCurrentNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNrnElctrodeCurrentNode(this); }
		    virtual std::string getNodeType() { return "NrnElctrodeCurrent"; }
		    virtual NrnElctrodeCurrentNode* clone() { return new NrnElctrodeCurrentNode(*this); }
	};

	/* ast Node for NrnSection */
	class NrnSectionNode : public StatementNode {
		public:
		    /* member variables */
		    SectionVarNodeList *sections;

		    /* constructor */
		    NrnSectionNode(SectionVarNodeList *sections);

		    /* copy constructor */
		    NrnSectionNode(const NrnSectionNode& obj);

		    virtual ~NrnSectionNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNrnSectionNode(this); }
		    virtual std::string getNodeType() { return "NrnSection"; }
		    virtual NrnSectionNode* clone() { return new NrnSectionNode(*this); }
	};

	/* ast Node for NrnRange */
	class NrnRangeNode : public StatementNode {
		public:
		    /* member variables */
		    RangeVarNodeList *range_vars;

		    /* constructor */
		    NrnRangeNode(RangeVarNodeList *range_vars);

		    /* copy constructor */
		    NrnRangeNode(const NrnRangeNode& obj);

		    virtual ~NrnRangeNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNrnRangeNode(this); }
		    virtual std::string getNodeType() { return "NrnRange"; }
		    virtual NrnRangeNode* clone() { return new NrnRangeNode(*this); }
	};

	/* ast Node for NrnGlobal */
	class NrnGlobalNode : public StatementNode {
		public:
		    /* member variables */
		    GlobalVarNodeList *global_vars;

		    /* constructor */
		    NrnGlobalNode(GlobalVarNodeList *global_vars);

		    /* copy constructor */
		    NrnGlobalNode(const NrnGlobalNode& obj);

		    virtual ~NrnGlobalNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNrnGlobalNode(this); }
		    virtual std::string getNodeType() { return "NrnGlobal"; }
		    virtual NrnGlobalNode* clone() { return new NrnGlobalNode(*this); }
	};

	/* ast Node for NrnPointer */
	class NrnPointerNode : public StatementNode {
		public:
		    /* member variables */
		    PointerVarNodeList *pointers;

		    /* constructor */
		    NrnPointerNode(PointerVarNodeList *pointers);

		    /* copy constructor */
		    NrnPointerNode(const NrnPointerNode& obj);

		    virtual ~NrnPointerNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNrnPointerNode(this); }
		    virtual std::string getNodeType() { return "NrnPointer"; }
		    virtual NrnPointerNode* clone() { return new NrnPointerNode(*this); }
	};

	/* ast Node for NrnBbcorePtr */
	class NrnBbcorePtrNode : public StatementNode {
		public:
		    /* member variables */
		    BbcorePointerVarNodeList *bbcore_pointers;

		    /* constructor */
		    NrnBbcorePtrNode(BbcorePointerVarNodeList *bbcore_pointers);

		    /* copy constructor */
		    NrnBbcorePtrNode(const NrnBbcorePtrNode& obj);

		    virtual ~NrnBbcorePtrNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNrnBbcorePtrNode(this); }
		    virtual std::string getNodeType() { return "NrnBbcorePtr"; }
		    virtual NrnBbcorePtrNode* clone() { return new NrnBbcorePtrNode(*this); }
	};

	/* ast Node for NrnExternal */
	class NrnExternalNode : public StatementNode {
		public:
		    /* member variables */
		    ExternVarNodeList *externals;

		    /* constructor */
		    NrnExternalNode(ExternVarNodeList *externals);

		    /* copy constructor */
		    NrnExternalNode(const NrnExternalNode& obj);

		    virtual ~NrnExternalNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNrnExternalNode(this); }
		    virtual std::string getNodeType() { return "NrnExternal"; }
		    virtual NrnExternalNode* clone() { return new NrnExternalNode(*this); }
	};

	/* ast Node for NrnThreadSafe */
	class NrnThreadSafeNode : public StatementNode {
		public:
		    /* member variables */
		    ThreadsafeVarNodeList *threadsafe;

		    /* constructor */
		    NrnThreadSafeNode(ThreadsafeVarNodeList *threadsafe);

		    /* copy constructor */
		    NrnThreadSafeNode(const NrnThreadSafeNode& obj);

		    virtual ~NrnThreadSafeNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitNrnThreadSafeNode(this); }
		    virtual std::string getNodeType() { return "NrnThreadSafe"; }
		    virtual NrnThreadSafeNode* clone() { return new NrnThreadSafeNode(*this); }
	};

	/* ast Node for Verbatim */
	class VerbatimNode : public StatementNode {
		public:
		    /* member variables */
		    StringNode *statement;

		    /* constructor */
		    VerbatimNode(StringNode *statement);

		    /* copy constructor */
		    VerbatimNode(const VerbatimNode& obj);

		    virtual ~VerbatimNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitVerbatimNode(this); }
		    virtual std::string getNodeType() { return "Verbatim"; }
		    virtual VerbatimNode* clone() { return new VerbatimNode(*this); }
	};

	/* ast Node for Comment */
	class CommentNode : public StatementNode {
		public:
		    /* member variables */
		    StringNode *comment;

		    /* constructor */
		    CommentNode(StringNode *comment);

		    /* copy constructor */
		    CommentNode(const CommentNode& obj);

		    virtual ~CommentNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitCommentNode(this); }
		    virtual std::string getNodeType() { return "Comment"; }
		    virtual CommentNode* clone() { return new CommentNode(*this); }
	};

	/* ast Node for Valence */
	class ValenceNode : public ExpressionNode {
		public:
		    /* member variables */
		    NameNode *type;
		    DoubleNode *value;

		    /* constructor */
		    ValenceNode(NameNode *type, DoubleNode *value);

		    /* copy constructor */
		    ValenceNode(const ValenceNode& obj);

		    virtual ~ValenceNode();
		    virtual void visitChildren(Visitor* v);
		    virtual void accept(Visitor* v) { v->visitValenceNode(this); }
		    virtual std::string getNodeType() { return "Valence"; }
		    virtual ValenceNode* clone() { return new ValenceNode(*this); }
	};

}    //namespace ast

#endif
