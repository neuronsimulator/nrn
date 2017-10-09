#include "ast/ast.hpp"
/* for node constructors, all children are taken as
 * parameters, and must be passed in. Optional children
 * may be NULL pointers. List children are pointers to
 * std::vectors of the appropriate type (pointer to some
 * node type)
 */

namespace ast {

	/* visit Children method for Expression ast node */
	void ExpressionNode::visitChildren(Visitor* v) {
	}

	/* destructor for Expression ast node */
	ExpressionNode::~ExpressionNode() {
	}

	/* visit Children method for Statement ast node */
	void StatementNode::visitChildren(Visitor* v) {
	}

	/* destructor for Statement ast node */
	StatementNode::~StatementNode() {
	}

	/* visit Children method for Identifier ast node */
	void IdentifierNode::visitChildren(Visitor* v) {
	}

	/* destructor for Identifier ast node */
	IdentifierNode::~IdentifierNode() {
	}

	/* visit Children method for Block ast node */
	void BlockNode::visitChildren(Visitor* v) {
	}

	/* destructor for Block ast node */
	BlockNode::~BlockNode() {
	}

	/* visit Children method for Number ast node */
	void NumberNode::visitChildren(Visitor* v) {
	}

	/* destructor for Number ast node */
	NumberNode::~NumberNode() {
	}

	/* visit Children method for String ast node */
	void StringNode::visitChildren(Visitor* v) {
	    /* no children */
	}

	/* constructor for String ast node */
	StringNode::StringNode(std::string  value) {
	    this->value = value;
	    this->token = NULL;
	}

	/* copy constructor for String ast node */
	StringNode::StringNode(const StringNode& obj) {
	    this->value = obj.value;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	}

	/* destructor for String ast node */
	StringNode::~StringNode() {
	    if(token)
	        delete token;
	}

	/* visit Children method for Integer ast node */
	void IntegerNode::visitChildren(Visitor* v) {
	    /* no children */
	    if (this->macroname) {
	        this->macroname->accept(v);
	    }

	}

	/* constructor for Integer ast node */
	IntegerNode::IntegerNode(int  value, NameNode * macroname) {
	    this->value = value;
	    this->macroname = macroname;
	    this->token = NULL;
	}

	/* copy constructor for Integer ast node */
	IntegerNode::IntegerNode(const IntegerNode& obj) {
	    this->value = obj.value;
	    if(obj.macroname)
	        this->macroname = obj.macroname->clone();
	    else
	        this->macroname = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	}

	/* destructor for Integer ast node */
	IntegerNode::~IntegerNode() {
	    if(token)
	        delete token;
	}

	/* visit Children method for Float ast node */
	void FloatNode::visitChildren(Visitor* v) {
	    /* no children */
	}

	/* constructor for Float ast node */
	FloatNode::FloatNode(float  value) {
	    this->value = value;
	}

	/* copy constructor for Float ast node */
	FloatNode::FloatNode(const FloatNode& obj) {
	    this->value = obj.value;
	}

	/* destructor for Float ast node */
	FloatNode::~FloatNode() {
	}

	/* visit Children method for Double ast node */
	void DoubleNode::visitChildren(Visitor* v) {
	    /* no children */
	}

	/* constructor for Double ast node */
	DoubleNode::DoubleNode(double  value) {
	    this->value = value;
	    this->token = NULL;
	}

	/* copy constructor for Double ast node */
	DoubleNode::DoubleNode(const DoubleNode& obj) {
	    this->value = obj.value;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	}

	/* destructor for Double ast node */
	DoubleNode::~DoubleNode() {
	    if(token)
	        delete token;
	}

	/* visit Children method for Boolean ast node */
	void BooleanNode::visitChildren(Visitor* v) {
	    /* no children */
	}

	/* constructor for Boolean ast node */
	BooleanNode::BooleanNode(int  value) {
	    this->value = value;
	}

	/* copy constructor for Boolean ast node */
	BooleanNode::BooleanNode(const BooleanNode& obj) {
	    this->value = obj.value;
	}

	/* destructor for Boolean ast node */
	BooleanNode::~BooleanNode() {
	}

	/* visit Children method for Name ast node */
	void NameNode::visitChildren(Visitor* v) {
	    value->accept(v);
	}

	/* constructor for Name ast node */
	NameNode::NameNode(StringNode * value) {
	    this->value = value;
	    this->token = NULL;
	}

	/* copy constructor for Name ast node */
	NameNode::NameNode(const NameNode& obj) {
	    if(obj.value)
	        this->value = obj.value->clone();
	    else
	        this->value = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	}

	/* destructor for Name ast node */
	NameNode::~NameNode() {
	    if(value)
	        delete value;
	    if(token)
	        delete token;
	}

	/* visit Children method for PrimeName ast node */
	void PrimeNameNode::visitChildren(Visitor* v) {
	    value->accept(v);
	    order->accept(v);
	}

	/* constructor for PrimeName ast node */
	PrimeNameNode::PrimeNameNode(StringNode * value, IntegerNode * order) {
	    this->value = value;
	    this->order = order;
	    this->token = NULL;
	}

	/* copy constructor for PrimeName ast node */
	PrimeNameNode::PrimeNameNode(const PrimeNameNode& obj) {
	    if(obj.value)
	        this->value = obj.value->clone();
	    else
	        this->value = NULL;
	    if(obj.order)
	        this->order = obj.order->clone();
	    else
	        this->order = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	}

	/* destructor for PrimeName ast node */
	PrimeNameNode::~PrimeNameNode() {
	    if(value)
	        delete value;
	    if(order)
	        delete order;
	    if(token)
	        delete token;
	}

	/* visit Children method for VarName ast node */
	void VarNameNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->at_index) {
	        this->at_index->accept(v);
	    }

	}

	/* constructor for VarName ast node */
	VarNameNode::VarNameNode(IdentifierNode * name, IntegerNode * at_index) {
	    this->name = name;
	    this->at_index = at_index;
	}

	/* copy constructor for VarName ast node */
	VarNameNode::VarNameNode(const VarNameNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.at_index)
	        this->at_index = obj.at_index->clone();
	    else
	        this->at_index = NULL;
	}

	/* destructor for VarName ast node */
	VarNameNode::~VarNameNode() {
	    if(name)
	        delete name;
	    if(at_index)
	        delete at_index;
	}

	/* visit Children method for IndexedName ast node */
	void IndexedNameNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    index->accept(v);
	}

	/* constructor for IndexedName ast node */
	IndexedNameNode::IndexedNameNode(IdentifierNode * name, ExpressionNode * index) {
	    this->name = name;
	    this->index = index;
	}

	/* copy constructor for IndexedName ast node */
	IndexedNameNode::IndexedNameNode(const IndexedNameNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.index)
	        this->index = obj.index->clone();
	    else
	        this->index = NULL;
	}

	/* destructor for IndexedName ast node */
	IndexedNameNode::~IndexedNameNode() {
	    if(name)
	        delete name;
	    if(index)
	        delete index;
	}

	/* visit Children method for Unit ast node */
	void UnitNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for Unit ast node */
	UnitNode::UnitNode(StringNode * name) {
	    this->name = name;
	}

	/* copy constructor for Unit ast node */
	UnitNode::UnitNode(const UnitNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for Unit ast node */
	UnitNode::~UnitNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for UnitState ast node */
	void UnitStateNode::visitChildren(Visitor* v) {
	    /* no children */
	}

	/* constructor for UnitState ast node */
	UnitStateNode::UnitStateNode(UnitStateType  value) {
	    this->value = value;
	}

	/* copy constructor for UnitState ast node */
	UnitStateNode::UnitStateNode(const UnitStateNode& obj) {
	    this->value = obj.value;
	}

	/* destructor for UnitState ast node */
	UnitStateNode::~UnitStateNode() {
	}

	/* visit Children method for DoubleUnit ast node */
	void DoubleUnitNode::visitChildren(Visitor* v) {
	    values->accept(v);
	    if (this->unit) {
	        this->unit->accept(v);
	    }

	}

	/* constructor for DoubleUnit ast node */
	DoubleUnitNode::DoubleUnitNode(DoubleNode * values, UnitNode * unit) {
	    this->values = values;
	    this->unit = unit;
	}

	/* copy constructor for DoubleUnit ast node */
	DoubleUnitNode::DoubleUnitNode(const DoubleUnitNode& obj) {
	    if(obj.values)
	        this->values = obj.values->clone();
	    else
	        this->values = NULL;
	    if(obj.unit)
	        this->unit = obj.unit->clone();
	    else
	        this->unit = NULL;
	}

	/* destructor for DoubleUnit ast node */
	DoubleUnitNode::~DoubleUnitNode() {
	    if(values)
	        delete values;
	    if(unit)
	        delete unit;
	}

	/* visit Children method for Argument ast node */
	void ArgumentNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->unit) {
	        this->unit->accept(v);
	    }

	}

	/* constructor for Argument ast node */
	ArgumentNode::ArgumentNode(IdentifierNode * name, UnitNode * unit) {
	    this->name = name;
	    this->unit = unit;
	}

	/* copy constructor for Argument ast node */
	ArgumentNode::ArgumentNode(const ArgumentNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.unit)
	        this->unit = obj.unit->clone();
	    else
	        this->unit = NULL;
	}

	/* destructor for Argument ast node */
	ArgumentNode::~ArgumentNode() {
	    if(name)
	        delete name;
	    if(unit)
	        delete unit;
	}

	/* visit Children method for LocalVariable ast node */
	void LocalVariableNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for LocalVariable ast node */
	LocalVariableNode::LocalVariableNode(IdentifierNode * name) {
	    this->name = name;
	}

	/* copy constructor for LocalVariable ast node */
	LocalVariableNode::LocalVariableNode(const LocalVariableNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for LocalVariable ast node */
	LocalVariableNode::~LocalVariableNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for LocalListStatement ast node */
	void LocalListStatementNode::visitChildren(Visitor* v) {
	    if (this->variables) {
	        for(LocalVariableNodeList::iterator iter = this->variables->begin();
	            iter != this->variables->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for LocalListStatement ast node */
	LocalListStatementNode::LocalListStatementNode(LocalVariableNodeList * variables) {
	    this->variables = variables;
	}

	/* copy constructor for LocalListStatement ast node */
	LocalListStatementNode::LocalListStatementNode(const LocalListStatementNode& obj) {
	    // cloning list
	    if (obj.variables) {
	        this->variables = new LocalVariableNodeList();
	        for(LocalVariableNodeList::iterator iter = obj.variables->begin();
	            iter != obj.variables->end(); iter++) {
	            this->variables->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->variables = NULL;

	}

	/* destructor for LocalListStatement ast node */
	LocalListStatementNode::~LocalListStatementNode() {
	    if (variables) {
	        for(LocalVariableNodeList::iterator iter = this->variables->begin();
	            iter != this->variables->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(variables)
	        delete variables;
	}

	/* visit Children method for Limits ast node */
	void LimitsNode::visitChildren(Visitor* v) {
	    min->accept(v);
	    max->accept(v);
	}

	/* constructor for Limits ast node */
	LimitsNode::LimitsNode(DoubleNode * min, DoubleNode * max) {
	    this->min = min;
	    this->max = max;
	}

	/* copy constructor for Limits ast node */
	LimitsNode::LimitsNode(const LimitsNode& obj) {
	    if(obj.min)
	        this->min = obj.min->clone();
	    else
	        this->min = NULL;
	    if(obj.max)
	        this->max = obj.max->clone();
	    else
	        this->max = NULL;
	}

	/* destructor for Limits ast node */
	LimitsNode::~LimitsNode() {
	    if(min)
	        delete min;
	    if(max)
	        delete max;
	}

	/* visit Children method for NumberRange ast node */
	void NumberRangeNode::visitChildren(Visitor* v) {
	    min->accept(v);
	    max->accept(v);
	}

	/* constructor for NumberRange ast node */
	NumberRangeNode::NumberRangeNode(NumberNode * min, NumberNode * max) {
	    this->min = min;
	    this->max = max;
	}

	/* copy constructor for NumberRange ast node */
	NumberRangeNode::NumberRangeNode(const NumberRangeNode& obj) {
	    if(obj.min)
	        this->min = obj.min->clone();
	    else
	        this->min = NULL;
	    if(obj.max)
	        this->max = obj.max->clone();
	    else
	        this->max = NULL;
	}

	/* destructor for NumberRange ast node */
	NumberRangeNode::~NumberRangeNode() {
	    if(min)
	        delete min;
	    if(max)
	        delete max;
	}

	/* visit Children method for Program ast node */
	void ProgramNode::visitChildren(Visitor* v) {
	    if (this->statements) {
	        for(StatementNodeList::iterator iter = this->statements->begin();
	            iter != this->statements->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->blocks) {
	        for(BlockNodeList::iterator iter = this->blocks->begin();
	            iter != this->blocks->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for Program ast node */
	ProgramNode::ProgramNode(StatementNodeList * statements, BlockNodeList * blocks) {
	    this->statements = statements;
	    this->blocks = blocks;
	    this->symtab = NULL;
	}

	/* copy constructor for Program ast node */
	ProgramNode::ProgramNode(const ProgramNode& obj) {
	    // cloning list
	    if (obj.statements) {
	        this->statements = new StatementNodeList();
	        for(StatementNodeList::iterator iter = obj.statements->begin();
	            iter != obj.statements->end(); iter++) {
	            this->statements->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->statements = NULL;

	    // cloning list
	    if (obj.blocks) {
	        this->blocks = new BlockNodeList();
	        for(BlockNodeList::iterator iter = obj.blocks->begin();
	            iter != obj.blocks->end(); iter++) {
	            this->blocks->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->blocks = NULL;

	    this->symtab = NULL;
	}

	/* destructor for Program ast node */
	ProgramNode::~ProgramNode() {
	    if (statements) {
	        for(StatementNodeList::iterator iter = this->statements->begin();
	            iter != this->statements->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(statements)
	        delete statements;
	    if (blocks) {
	        for(BlockNodeList::iterator iter = this->blocks->begin();
	            iter != this->blocks->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(blocks)
	        delete blocks;
	}

	/* visit Children method for Model ast node */
	void ModelNode::visitChildren(Visitor* v) {
	    title->accept(v);
	}

	/* constructor for Model ast node */
	ModelNode::ModelNode(StringNode * title) {
	    this->title = title;
	}

	/* copy constructor for Model ast node */
	ModelNode::ModelNode(const ModelNode& obj) {
	    if(obj.title)
	        this->title = obj.title->clone();
	    else
	        this->title = NULL;
	}

	/* destructor for Model ast node */
	ModelNode::~ModelNode() {
	    if(title)
	        delete title;
	}

	/* visit Children method for Define ast node */
	void DefineNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    value->accept(v);
	}

	/* constructor for Define ast node */
	DefineNode::DefineNode(NameNode * name, IntegerNode * value) {
	    this->name = name;
	    this->value = value;
	}

	/* copy constructor for Define ast node */
	DefineNode::DefineNode(const DefineNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.value)
	        this->value = obj.value->clone();
	    else
	        this->value = NULL;
	}

	/* destructor for Define ast node */
	DefineNode::~DefineNode() {
	    if(name)
	        delete name;
	    if(value)
	        delete value;
	}

	/* visit Children method for Include ast node */
	void IncludeNode::visitChildren(Visitor* v) {
	    filename->accept(v);
	}

	/* constructor for Include ast node */
	IncludeNode::IncludeNode(StringNode * filename) {
	    this->filename = filename;
	}

	/* copy constructor for Include ast node */
	IncludeNode::IncludeNode(const IncludeNode& obj) {
	    if(obj.filename)
	        this->filename = obj.filename->clone();
	    else
	        this->filename = NULL;
	}

	/* destructor for Include ast node */
	IncludeNode::~IncludeNode() {
	    if(filename)
	        delete filename;
	}

	/* visit Children method for ParamBlock ast node */
	void ParamBlockNode::visitChildren(Visitor* v) {
	    if (this->statements) {
	        for(ParamAssignNodeList::iterator iter = this->statements->begin();
	            iter != this->statements->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for ParamBlock ast node */
	ParamBlockNode::ParamBlockNode(ParamAssignNodeList * statements) {
	    this->statements = statements;
	    this->symtab = NULL;
	}

	/* copy constructor for ParamBlock ast node */
	ParamBlockNode::ParamBlockNode(const ParamBlockNode& obj) {
	    // cloning list
	    if (obj.statements) {
	        this->statements = new ParamAssignNodeList();
	        for(ParamAssignNodeList::iterator iter = obj.statements->begin();
	            iter != obj.statements->end(); iter++) {
	            this->statements->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->statements = NULL;

	    this->symtab = NULL;
	}

	/* destructor for ParamBlock ast node */
	ParamBlockNode::~ParamBlockNode() {
	    if (statements) {
	        for(ParamAssignNodeList::iterator iter = this->statements->begin();
	            iter != this->statements->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(statements)
	        delete statements;
	}

	/* visit Children method for ParamAssign ast node */
	void ParamAssignNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->value) {
	        this->value->accept(v);
	    }

	    if (this->unit) {
	        this->unit->accept(v);
	    }

	    if (this->limit) {
	        this->limit->accept(v);
	    }

	}

	/* constructor for ParamAssign ast node */
	ParamAssignNode::ParamAssignNode(IdentifierNode * name, NumberNode * value, UnitNode * unit, LimitsNode * limit) {
	    this->name = name;
	    this->value = value;
	    this->unit = unit;
	    this->limit = limit;
	}

	/* copy constructor for ParamAssign ast node */
	ParamAssignNode::ParamAssignNode(const ParamAssignNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.value)
	        this->value = obj.value->clone();
	    else
	        this->value = NULL;
	    if(obj.unit)
	        this->unit = obj.unit->clone();
	    else
	        this->unit = NULL;
	    if(obj.limit)
	        this->limit = obj.limit->clone();
	    else
	        this->limit = NULL;
	}

	/* destructor for ParamAssign ast node */
	ParamAssignNode::~ParamAssignNode() {
	    if(name)
	        delete name;
	    if(value)
	        delete value;
	    if(unit)
	        delete unit;
	    if(limit)
	        delete limit;
	}

	/* visit Children method for StepBlock ast node */
	void StepBlockNode::visitChildren(Visitor* v) {
	    if (this->statements) {
	        for(SteppedNodeList::iterator iter = this->statements->begin();
	            iter != this->statements->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for StepBlock ast node */
	StepBlockNode::StepBlockNode(SteppedNodeList * statements) {
	    this->statements = statements;
	    this->symtab = NULL;
	}

	/* copy constructor for StepBlock ast node */
	StepBlockNode::StepBlockNode(const StepBlockNode& obj) {
	    // cloning list
	    if (obj.statements) {
	        this->statements = new SteppedNodeList();
	        for(SteppedNodeList::iterator iter = obj.statements->begin();
	            iter != obj.statements->end(); iter++) {
	            this->statements->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->statements = NULL;

	    this->symtab = NULL;
	}

	/* destructor for StepBlock ast node */
	StepBlockNode::~StepBlockNode() {
	    if (statements) {
	        for(SteppedNodeList::iterator iter = this->statements->begin();
	            iter != this->statements->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(statements)
	        delete statements;
	}

	/* visit Children method for Stepped ast node */
	void SteppedNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->values) {
	        for(NumberNodeList::iterator iter = this->values->begin();
	            iter != this->values->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    unit->accept(v);
	}

	/* constructor for Stepped ast node */
	SteppedNode::SteppedNode(NameNode * name, NumberNodeList * values, UnitNode * unit) {
	    this->name = name;
	    this->values = values;
	    this->unit = unit;
	}

	/* copy constructor for Stepped ast node */
	SteppedNode::SteppedNode(const SteppedNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    // cloning list
	    if (obj.values) {
	        this->values = new NumberNodeList();
	        for(NumberNodeList::iterator iter = obj.values->begin();
	            iter != obj.values->end(); iter++) {
	            this->values->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->values = NULL;

	    if(obj.unit)
	        this->unit = obj.unit->clone();
	    else
	        this->unit = NULL;
	}

	/* destructor for Stepped ast node */
	SteppedNode::~SteppedNode() {
	    if(name)
	        delete name;
	    if (values) {
	        for(NumberNodeList::iterator iter = this->values->begin();
	            iter != this->values->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(values)
	        delete values;
	    if(unit)
	        delete unit;
	}

	/* visit Children method for IndependentBlock ast node */
	void IndependentBlockNode::visitChildren(Visitor* v) {
	    if (this->definitions) {
	        for(IndependentDefNodeList::iterator iter = this->definitions->begin();
	            iter != this->definitions->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for IndependentBlock ast node */
	IndependentBlockNode::IndependentBlockNode(IndependentDefNodeList * definitions) {
	    this->definitions = definitions;
	    this->symtab = NULL;
	}

	/* copy constructor for IndependentBlock ast node */
	IndependentBlockNode::IndependentBlockNode(const IndependentBlockNode& obj) {
	    // cloning list
	    if (obj.definitions) {
	        this->definitions = new IndependentDefNodeList();
	        for(IndependentDefNodeList::iterator iter = obj.definitions->begin();
	            iter != obj.definitions->end(); iter++) {
	            this->definitions->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->definitions = NULL;

	    this->symtab = NULL;
	}

	/* destructor for IndependentBlock ast node */
	IndependentBlockNode::~IndependentBlockNode() {
	    if (definitions) {
	        for(IndependentDefNodeList::iterator iter = this->definitions->begin();
	            iter != this->definitions->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(definitions)
	        delete definitions;
	}

	/* visit Children method for IndependentDef ast node */
	void IndependentDefNode::visitChildren(Visitor* v) {
	    if (this->sweep) {
	        this->sweep->accept(v);
	    }

	    name->accept(v);
	    from->accept(v);
	    to->accept(v);
	    with->accept(v);
	    if (this->opstart) {
	        this->opstart->accept(v);
	    }

	    unit->accept(v);
	}

	/* constructor for IndependentDef ast node */
	IndependentDefNode::IndependentDefNode(BooleanNode * sweep, NameNode * name, NumberNode * from, NumberNode * to, IntegerNode * with, NumberNode * opstart, UnitNode * unit) {
	    this->sweep = sweep;
	    this->name = name;
	    this->from = from;
	    this->to = to;
	    this->with = with;
	    this->opstart = opstart;
	    this->unit = unit;
	}

	/* copy constructor for IndependentDef ast node */
	IndependentDefNode::IndependentDefNode(const IndependentDefNode& obj) {
	    if(obj.sweep)
	        this->sweep = obj.sweep->clone();
	    else
	        this->sweep = NULL;
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.from)
	        this->from = obj.from->clone();
	    else
	        this->from = NULL;
	    if(obj.to)
	        this->to = obj.to->clone();
	    else
	        this->to = NULL;
	    if(obj.with)
	        this->with = obj.with->clone();
	    else
	        this->with = NULL;
	    if(obj.opstart)
	        this->opstart = obj.opstart->clone();
	    else
	        this->opstart = NULL;
	    if(obj.unit)
	        this->unit = obj.unit->clone();
	    else
	        this->unit = NULL;
	}

	/* destructor for IndependentDef ast node */
	IndependentDefNode::~IndependentDefNode() {
	    if(sweep)
	        delete sweep;
	    if(name)
	        delete name;
	    if(from)
	        delete from;
	    if(to)
	        delete to;
	    if(with)
	        delete with;
	    if(opstart)
	        delete opstart;
	    if(unit)
	        delete unit;
	}

	/* visit Children method for DependentDef ast node */
	void DependentDefNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->index) {
	        this->index->accept(v);
	    }

	    if (this->from) {
	        this->from->accept(v);
	    }

	    if (this->to) {
	        this->to->accept(v);
	    }

	    if (this->opstart) {
	        this->opstart->accept(v);
	    }

	    if (this->unit) {
	        this->unit->accept(v);
	    }

	    if (this->abstol) {
	        this->abstol->accept(v);
	    }

	}

	/* constructor for DependentDef ast node */
	DependentDefNode::DependentDefNode(IdentifierNode * name, IntegerNode * index, NumberNode * from, NumberNode * to, NumberNode * opstart, UnitNode * unit, DoubleNode * abstol) {
	    this->name = name;
	    this->index = index;
	    this->from = from;
	    this->to = to;
	    this->opstart = opstart;
	    this->unit = unit;
	    this->abstol = abstol;
	}

	/* copy constructor for DependentDef ast node */
	DependentDefNode::DependentDefNode(const DependentDefNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.index)
	        this->index = obj.index->clone();
	    else
	        this->index = NULL;
	    if(obj.from)
	        this->from = obj.from->clone();
	    else
	        this->from = NULL;
	    if(obj.to)
	        this->to = obj.to->clone();
	    else
	        this->to = NULL;
	    if(obj.opstart)
	        this->opstart = obj.opstart->clone();
	    else
	        this->opstart = NULL;
	    if(obj.unit)
	        this->unit = obj.unit->clone();
	    else
	        this->unit = NULL;
	    if(obj.abstol)
	        this->abstol = obj.abstol->clone();
	    else
	        this->abstol = NULL;
	}

	/* destructor for DependentDef ast node */
	DependentDefNode::~DependentDefNode() {
	    if(name)
	        delete name;
	    if(index)
	        delete index;
	    if(from)
	        delete from;
	    if(to)
	        delete to;
	    if(opstart)
	        delete opstart;
	    if(unit)
	        delete unit;
	    if(abstol)
	        delete abstol;
	}

	/* visit Children method for DependentBlock ast node */
	void DependentBlockNode::visitChildren(Visitor* v) {
	    if (this->definitions) {
	        for(DependentDefNodeList::iterator iter = this->definitions->begin();
	            iter != this->definitions->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for DependentBlock ast node */
	DependentBlockNode::DependentBlockNode(DependentDefNodeList * definitions) {
	    this->definitions = definitions;
	    this->symtab = NULL;
	}

	/* copy constructor for DependentBlock ast node */
	DependentBlockNode::DependentBlockNode(const DependentBlockNode& obj) {
	    // cloning list
	    if (obj.definitions) {
	        this->definitions = new DependentDefNodeList();
	        for(DependentDefNodeList::iterator iter = obj.definitions->begin();
	            iter != obj.definitions->end(); iter++) {
	            this->definitions->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->definitions = NULL;

	    this->symtab = NULL;
	}

	/* destructor for DependentBlock ast node */
	DependentBlockNode::~DependentBlockNode() {
	    if (definitions) {
	        for(DependentDefNodeList::iterator iter = this->definitions->begin();
	            iter != this->definitions->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(definitions)
	        delete definitions;
	}

	/* visit Children method for StateBlock ast node */
	void StateBlockNode::visitChildren(Visitor* v) {
	    if (this->definitions) {
	        for(DependentDefNodeList::iterator iter = this->definitions->begin();
	            iter != this->definitions->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for StateBlock ast node */
	StateBlockNode::StateBlockNode(DependentDefNodeList * definitions) {
	    this->definitions = definitions;
	    this->symtab = NULL;
	}

	/* copy constructor for StateBlock ast node */
	StateBlockNode::StateBlockNode(const StateBlockNode& obj) {
	    // cloning list
	    if (obj.definitions) {
	        this->definitions = new DependentDefNodeList();
	        for(DependentDefNodeList::iterator iter = obj.definitions->begin();
	            iter != obj.definitions->end(); iter++) {
	            this->definitions->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->definitions = NULL;

	    this->symtab = NULL;
	}

	/* destructor for StateBlock ast node */
	StateBlockNode::~StateBlockNode() {
	    if (definitions) {
	        for(DependentDefNodeList::iterator iter = this->definitions->begin();
	            iter != this->definitions->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(definitions)
	        delete definitions;
	}

	/* visit Children method for PlotBlock ast node */
	void PlotBlockNode::visitChildren(Visitor* v) {
	    plot->accept(v);
	}

	/* constructor for PlotBlock ast node */
	PlotBlockNode::PlotBlockNode(PlotDeclarationNode * plot) {
	    this->plot = plot;
	    this->symtab = NULL;
	}

	/* copy constructor for PlotBlock ast node */
	PlotBlockNode::PlotBlockNode(const PlotBlockNode& obj) {
	    if(obj.plot)
	        this->plot = obj.plot->clone();
	    else
	        this->plot = NULL;
	    this->symtab = NULL;
	}

	/* destructor for PlotBlock ast node */
	PlotBlockNode::~PlotBlockNode() {
	    if(plot)
	        delete plot;
	}

	/* visit Children method for PlotDeclaration ast node */
	void PlotDeclarationNode::visitChildren(Visitor* v) {
	    if (this->pvlist) {
	        for(PlotVariableNodeList::iterator iter = this->pvlist->begin();
	            iter != this->pvlist->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    name->accept(v);
	}

	/* constructor for PlotDeclaration ast node */
	PlotDeclarationNode::PlotDeclarationNode(PlotVariableNodeList * pvlist, PlotVariableNode * name) {
	    this->pvlist = pvlist;
	    this->name = name;
	}

	/* copy constructor for PlotDeclaration ast node */
	PlotDeclarationNode::PlotDeclarationNode(const PlotDeclarationNode& obj) {
	    // cloning list
	    if (obj.pvlist) {
	        this->pvlist = new PlotVariableNodeList();
	        for(PlotVariableNodeList::iterator iter = obj.pvlist->begin();
	            iter != obj.pvlist->end(); iter++) {
	            this->pvlist->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->pvlist = NULL;

	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for PlotDeclaration ast node */
	PlotDeclarationNode::~PlotDeclarationNode() {
	    if (pvlist) {
	        for(PlotVariableNodeList::iterator iter = this->pvlist->begin();
	            iter != this->pvlist->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(pvlist)
	        delete pvlist;
	    if(name)
	        delete name;
	}

	/* visit Children method for PlotVariable ast node */
	void PlotVariableNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->index) {
	        this->index->accept(v);
	    }

	}

	/* constructor for PlotVariable ast node */
	PlotVariableNode::PlotVariableNode(IdentifierNode * name, IntegerNode * index) {
	    this->name = name;
	    this->index = index;
	}

	/* copy constructor for PlotVariable ast node */
	PlotVariableNode::PlotVariableNode(const PlotVariableNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.index)
	        this->index = obj.index->clone();
	    else
	        this->index = NULL;
	}

	/* destructor for PlotVariable ast node */
	PlotVariableNode::~PlotVariableNode() {
	    if(name)
	        delete name;
	    if(index)
	        delete index;
	}

	/* visit Children method for InitialBlock ast node */
	void InitialBlockNode::visitChildren(Visitor* v) {
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for InitialBlock ast node */
	InitialBlockNode::InitialBlockNode(StatementBlockNode * statementblock) {
	    this->statementblock = statementblock;
	    this->symtab = NULL;
	}

	/* copy constructor for InitialBlock ast node */
	InitialBlockNode::InitialBlockNode(const InitialBlockNode& obj) {
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    this->symtab = NULL;
	}

	/* destructor for InitialBlock ast node */
	InitialBlockNode::~InitialBlockNode() {
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for ConstructorBlock ast node */
	void ConstructorBlockNode::visitChildren(Visitor* v) {
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for ConstructorBlock ast node */
	ConstructorBlockNode::ConstructorBlockNode(StatementBlockNode * statementblock) {
	    this->statementblock = statementblock;
	    this->symtab = NULL;
	}

	/* copy constructor for ConstructorBlock ast node */
	ConstructorBlockNode::ConstructorBlockNode(const ConstructorBlockNode& obj) {
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    this->symtab = NULL;
	}

	/* destructor for ConstructorBlock ast node */
	ConstructorBlockNode::~ConstructorBlockNode() {
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for DestructorBlock ast node */
	void DestructorBlockNode::visitChildren(Visitor* v) {
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for DestructorBlock ast node */
	DestructorBlockNode::DestructorBlockNode(StatementBlockNode * statementblock) {
	    this->statementblock = statementblock;
	    this->symtab = NULL;
	}

	/* copy constructor for DestructorBlock ast node */
	DestructorBlockNode::DestructorBlockNode(const DestructorBlockNode& obj) {
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    this->symtab = NULL;
	}

	/* destructor for DestructorBlock ast node */
	DestructorBlockNode::~DestructorBlockNode() {
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for ConductanceHint ast node */
	void ConductanceHintNode::visitChildren(Visitor* v) {
	    conductance->accept(v);
	    if (this->ion) {
	        this->ion->accept(v);
	    }

	}

	/* constructor for ConductanceHint ast node */
	ConductanceHintNode::ConductanceHintNode(NameNode * conductance, NameNode * ion) {
	    this->conductance = conductance;
	    this->ion = ion;
	}

	/* copy constructor for ConductanceHint ast node */
	ConductanceHintNode::ConductanceHintNode(const ConductanceHintNode& obj) {
	    if(obj.conductance)
	        this->conductance = obj.conductance->clone();
	    else
	        this->conductance = NULL;
	    if(obj.ion)
	        this->ion = obj.ion->clone();
	    else
	        this->ion = NULL;
	}

	/* destructor for ConductanceHint ast node */
	ConductanceHintNode::~ConductanceHintNode() {
	    if(conductance)
	        delete conductance;
	    if(ion)
	        delete ion;
	}

	/* visit Children method for ExpressionStatement ast node */
	void ExpressionStatementNode::visitChildren(Visitor* v) {
	    expression->accept(v);
	}

	/* constructor for ExpressionStatement ast node */
	ExpressionStatementNode::ExpressionStatementNode(ExpressionNode * expression) {
	    this->expression = expression;
	}

	/* copy constructor for ExpressionStatement ast node */
	ExpressionStatementNode::ExpressionStatementNode(const ExpressionStatementNode& obj) {
	    if(obj.expression)
	        this->expression = obj.expression->clone();
	    else
	        this->expression = NULL;
	}

	/* destructor for ExpressionStatement ast node */
	ExpressionStatementNode::~ExpressionStatementNode() {
	    if(expression)
	        delete expression;
	}

	/* visit Children method for ProtectStatement ast node */
	void ProtectStatementNode::visitChildren(Visitor* v) {
	    expression->accept(v);
	}

	/* constructor for ProtectStatement ast node */
	ProtectStatementNode::ProtectStatementNode(ExpressionNode * expression) {
	    this->expression = expression;
	}

	/* copy constructor for ProtectStatement ast node */
	ProtectStatementNode::ProtectStatementNode(const ProtectStatementNode& obj) {
	    if(obj.expression)
	        this->expression = obj.expression->clone();
	    else
	        this->expression = NULL;
	}

	/* destructor for ProtectStatement ast node */
	ProtectStatementNode::~ProtectStatementNode() {
	    if(expression)
	        delete expression;
	}

	/* visit Children method for StatementBlock ast node */
	void StatementBlockNode::visitChildren(Visitor* v) {
	    if (this->statements) {
	        for(StatementNodeList::iterator iter = this->statements->begin();
	            iter != this->statements->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for StatementBlock ast node */
	StatementBlockNode::StatementBlockNode(StatementNodeList * statements) {
	    this->statements = statements;
	    this->token = NULL;
	    this->symtab = NULL;
	}

	/* copy constructor for StatementBlock ast node */
	StatementBlockNode::StatementBlockNode(const StatementBlockNode& obj) {
	    // cloning list
	    if (obj.statements) {
	        this->statements = new StatementNodeList();
	        for(StatementNodeList::iterator iter = obj.statements->begin();
	            iter != obj.statements->end(); iter++) {
	            this->statements->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->statements = NULL;

	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	    this->symtab = NULL;
	}

	/* destructor for StatementBlock ast node */
	StatementBlockNode::~StatementBlockNode() {
	    if (statements) {
	        for(StatementNodeList::iterator iter = this->statements->begin();
	            iter != this->statements->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(statements)
	        delete statements;
	    if(token)
	        delete token;
	}

	/* visit Children method for BinaryOperator ast node */
	void BinaryOperatorNode::visitChildren(Visitor* v) {
	    /* no children */
	}

	/* constructor for BinaryOperator ast node */
	BinaryOperatorNode::BinaryOperatorNode(BinaryOp  value) {
	    this->value = value;
	}

	/* copy constructor for BinaryOperator ast node */
	BinaryOperatorNode::BinaryOperatorNode(const BinaryOperatorNode& obj) {
	    this->value = obj.value;
	}

	/* destructor for BinaryOperator ast node */
	BinaryOperatorNode::~BinaryOperatorNode() {
	}

	/* visit Children method for UnaryOperator ast node */
	void UnaryOperatorNode::visitChildren(Visitor* v) {
	    /* no children */
	}

	/* constructor for UnaryOperator ast node */
	UnaryOperatorNode::UnaryOperatorNode(UnaryOp  value) {
	    this->value = value;
	}

	/* copy constructor for UnaryOperator ast node */
	UnaryOperatorNode::UnaryOperatorNode(const UnaryOperatorNode& obj) {
	    this->value = obj.value;
	}

	/* destructor for UnaryOperator ast node */
	UnaryOperatorNode::~UnaryOperatorNode() {
	}

	/* visit Children method for ReactionOperator ast node */
	void ReactionOperatorNode::visitChildren(Visitor* v) {
	    /* no children */
	}

	/* constructor for ReactionOperator ast node */
	ReactionOperatorNode::ReactionOperatorNode(ReactionOp  value) {
	    this->value = value;
	}

	/* copy constructor for ReactionOperator ast node */
	ReactionOperatorNode::ReactionOperatorNode(const ReactionOperatorNode& obj) {
	    this->value = obj.value;
	}

	/* destructor for ReactionOperator ast node */
	ReactionOperatorNode::~ReactionOperatorNode() {
	}

	/* visit Children method for BinaryExpression ast node */
	void BinaryExpressionNode::visitChildren(Visitor* v) {
	    lhs->accept(v);
	    op->accept(v);
	    rhs->accept(v);
	}

	/* constructor for BinaryExpression ast node */
	BinaryExpressionNode::BinaryExpressionNode(ExpressionNode * lhs, BinaryOperatorNode * op, ExpressionNode * rhs) {
	    this->lhs = lhs;
	    this->op = op;
	    this->rhs = rhs;
	}

	/* copy constructor for BinaryExpression ast node */
	BinaryExpressionNode::BinaryExpressionNode(const BinaryExpressionNode& obj) {
	    if(obj.lhs)
	        this->lhs = obj.lhs->clone();
	    else
	        this->lhs = NULL;
	    if(obj.op)
	        this->op = obj.op->clone();
	    else
	        this->op = NULL;
	    if(obj.rhs)
	        this->rhs = obj.rhs->clone();
	    else
	        this->rhs = NULL;
	}

	/* destructor for BinaryExpression ast node */
	BinaryExpressionNode::~BinaryExpressionNode() {
	    if(lhs)
	        delete lhs;
	    if(op)
	        delete op;
	    if(rhs)
	        delete rhs;
	}

	/* visit Children method for UnaryExpression ast node */
	void UnaryExpressionNode::visitChildren(Visitor* v) {
	    op->accept(v);
	    expression->accept(v);
	}

	/* constructor for UnaryExpression ast node */
	UnaryExpressionNode::UnaryExpressionNode(UnaryOperatorNode * op, ExpressionNode * expression) {
	    this->op = op;
	    this->expression = expression;
	}

	/* copy constructor for UnaryExpression ast node */
	UnaryExpressionNode::UnaryExpressionNode(const UnaryExpressionNode& obj) {
	    if(obj.op)
	        this->op = obj.op->clone();
	    else
	        this->op = NULL;
	    if(obj.expression)
	        this->expression = obj.expression->clone();
	    else
	        this->expression = NULL;
	}

	/* destructor for UnaryExpression ast node */
	UnaryExpressionNode::~UnaryExpressionNode() {
	    if(op)
	        delete op;
	    if(expression)
	        delete expression;
	}

	/* visit Children method for NonLinEuation ast node */
	void NonLinEuationNode::visitChildren(Visitor* v) {
	    lhs->accept(v);
	    rhs->accept(v);
	}

	/* constructor for NonLinEuation ast node */
	NonLinEuationNode::NonLinEuationNode(ExpressionNode * lhs, ExpressionNode * rhs) {
	    this->lhs = lhs;
	    this->rhs = rhs;
	}

	/* copy constructor for NonLinEuation ast node */
	NonLinEuationNode::NonLinEuationNode(const NonLinEuationNode& obj) {
	    if(obj.lhs)
	        this->lhs = obj.lhs->clone();
	    else
	        this->lhs = NULL;
	    if(obj.rhs)
	        this->rhs = obj.rhs->clone();
	    else
	        this->rhs = NULL;
	}

	/* destructor for NonLinEuation ast node */
	NonLinEuationNode::~NonLinEuationNode() {
	    if(lhs)
	        delete lhs;
	    if(rhs)
	        delete rhs;
	}

	/* visit Children method for LinEquation ast node */
	void LinEquationNode::visitChildren(Visitor* v) {
	    leftlinexpr->accept(v);
	    linexpr->accept(v);
	}

	/* constructor for LinEquation ast node */
	LinEquationNode::LinEquationNode(ExpressionNode * leftlinexpr, ExpressionNode * linexpr) {
	    this->leftlinexpr = leftlinexpr;
	    this->linexpr = linexpr;
	}

	/* copy constructor for LinEquation ast node */
	LinEquationNode::LinEquationNode(const LinEquationNode& obj) {
	    if(obj.leftlinexpr)
	        this->leftlinexpr = obj.leftlinexpr->clone();
	    else
	        this->leftlinexpr = NULL;
	    if(obj.linexpr)
	        this->linexpr = obj.linexpr->clone();
	    else
	        this->linexpr = NULL;
	}

	/* destructor for LinEquation ast node */
	LinEquationNode::~LinEquationNode() {
	    if(leftlinexpr)
	        delete leftlinexpr;
	    if(linexpr)
	        delete linexpr;
	}

	/* visit Children method for FunctionCall ast node */
	void FunctionCallNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->arguments) {
	        for(ExpressionNodeList::iterator iter = this->arguments->begin();
	            iter != this->arguments->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for FunctionCall ast node */
	FunctionCallNode::FunctionCallNode(NameNode * name, ExpressionNodeList * arguments) {
	    this->name = name;
	    this->arguments = arguments;
	}

	/* copy constructor for FunctionCall ast node */
	FunctionCallNode::FunctionCallNode(const FunctionCallNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    // cloning list
	    if (obj.arguments) {
	        this->arguments = new ExpressionNodeList();
	        for(ExpressionNodeList::iterator iter = obj.arguments->begin();
	            iter != obj.arguments->end(); iter++) {
	            this->arguments->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->arguments = NULL;

	}

	/* destructor for FunctionCall ast node */
	FunctionCallNode::~FunctionCallNode() {
	    if(name)
	        delete name;
	    if (arguments) {
	        for(ExpressionNodeList::iterator iter = this->arguments->begin();
	            iter != this->arguments->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(arguments)
	        delete arguments;
	}

	/* visit Children method for FromStatement ast node */
	void FromStatementNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    from->accept(v);
	    to->accept(v);
	    if (this->opinc) {
	        this->opinc->accept(v);
	    }

	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for FromStatement ast node */
	FromStatementNode::FromStatementNode(NameNode * name, ExpressionNode * from, ExpressionNode * to, ExpressionNode * opinc, StatementBlockNode * statementblock) {
	    this->name = name;
	    this->from = from;
	    this->to = to;
	    this->opinc = opinc;
	    this->statementblock = statementblock;
	}

	/* copy constructor for FromStatement ast node */
	FromStatementNode::FromStatementNode(const FromStatementNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.from)
	        this->from = obj.from->clone();
	    else
	        this->from = NULL;
	    if(obj.to)
	        this->to = obj.to->clone();
	    else
	        this->to = NULL;
	    if(obj.opinc)
	        this->opinc = obj.opinc->clone();
	    else
	        this->opinc = NULL;
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	}

	/* destructor for FromStatement ast node */
	FromStatementNode::~FromStatementNode() {
	    if(name)
	        delete name;
	    if(from)
	        delete from;
	    if(to)
	        delete to;
	    if(opinc)
	        delete opinc;
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for ForAllStatement ast node */
	void ForAllStatementNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for ForAllStatement ast node */
	ForAllStatementNode::ForAllStatementNode(NameNode * name, StatementBlockNode * statementblock) {
	    this->name = name;
	    this->statementblock = statementblock;
	}

	/* copy constructor for ForAllStatement ast node */
	ForAllStatementNode::ForAllStatementNode(const ForAllStatementNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	}

	/* destructor for ForAllStatement ast node */
	ForAllStatementNode::~ForAllStatementNode() {
	    if(name)
	        delete name;
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for WhileStatement ast node */
	void WhileStatementNode::visitChildren(Visitor* v) {
	    condition->accept(v);
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for WhileStatement ast node */
	WhileStatementNode::WhileStatementNode(ExpressionNode * condition, StatementBlockNode * statementblock) {
	    this->condition = condition;
	    this->statementblock = statementblock;
	}

	/* copy constructor for WhileStatement ast node */
	WhileStatementNode::WhileStatementNode(const WhileStatementNode& obj) {
	    if(obj.condition)
	        this->condition = obj.condition->clone();
	    else
	        this->condition = NULL;
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	}

	/* destructor for WhileStatement ast node */
	WhileStatementNode::~WhileStatementNode() {
	    if(condition)
	        delete condition;
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for IfStatement ast node */
	void IfStatementNode::visitChildren(Visitor* v) {
	    condition->accept(v);
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	    if (this->elseifs) {
	        for(ElseIfStatementNodeList::iterator iter = this->elseifs->begin();
	            iter != this->elseifs->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->elses) {
	        this->elses->accept(v);
	    }

	}

	/* constructor for IfStatement ast node */
	IfStatementNode::IfStatementNode(ExpressionNode * condition, StatementBlockNode * statementblock, ElseIfStatementNodeList * elseifs, ElseStatementNode * elses) {
	    this->condition = condition;
	    this->statementblock = statementblock;
	    this->elseifs = elseifs;
	    this->elses = elses;
	}

	/* copy constructor for IfStatement ast node */
	IfStatementNode::IfStatementNode(const IfStatementNode& obj) {
	    if(obj.condition)
	        this->condition = obj.condition->clone();
	    else
	        this->condition = NULL;
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    // cloning list
	    if (obj.elseifs) {
	        this->elseifs = new ElseIfStatementNodeList();
	        for(ElseIfStatementNodeList::iterator iter = obj.elseifs->begin();
	            iter != obj.elseifs->end(); iter++) {
	            this->elseifs->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->elseifs = NULL;

	    if(obj.elses)
	        this->elses = obj.elses->clone();
	    else
	        this->elses = NULL;
	}

	/* destructor for IfStatement ast node */
	IfStatementNode::~IfStatementNode() {
	    if(condition)
	        delete condition;
	    if(statementblock)
	        delete statementblock;
	    if (elseifs) {
	        for(ElseIfStatementNodeList::iterator iter = this->elseifs->begin();
	            iter != this->elseifs->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(elseifs)
	        delete elseifs;
	    if(elses)
	        delete elses;
	}

	/* visit Children method for ElseIfStatement ast node */
	void ElseIfStatementNode::visitChildren(Visitor* v) {
	    condition->accept(v);
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for ElseIfStatement ast node */
	ElseIfStatementNode::ElseIfStatementNode(ExpressionNode * condition, StatementBlockNode * statementblock) {
	    this->condition = condition;
	    this->statementblock = statementblock;
	}

	/* copy constructor for ElseIfStatement ast node */
	ElseIfStatementNode::ElseIfStatementNode(const ElseIfStatementNode& obj) {
	    if(obj.condition)
	        this->condition = obj.condition->clone();
	    else
	        this->condition = NULL;
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	}

	/* destructor for ElseIfStatement ast node */
	ElseIfStatementNode::~ElseIfStatementNode() {
	    if(condition)
	        delete condition;
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for ElseStatement ast node */
	void ElseStatementNode::visitChildren(Visitor* v) {
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for ElseStatement ast node */
	ElseStatementNode::ElseStatementNode(StatementBlockNode * statementblock) {
	    this->statementblock = statementblock;
	}

	/* copy constructor for ElseStatement ast node */
	ElseStatementNode::ElseStatementNode(const ElseStatementNode& obj) {
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	}

	/* destructor for ElseStatement ast node */
	ElseStatementNode::~ElseStatementNode() {
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for DerivativeBlock ast node */
	void DerivativeBlockNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for DerivativeBlock ast node */
	DerivativeBlockNode::DerivativeBlockNode(NameNode * name, StatementBlockNode * statementblock) {
	    this->name = name;
	    this->statementblock = statementblock;
	    this->token = NULL;
	    this->symtab = NULL;
	}

	/* copy constructor for DerivativeBlock ast node */
	DerivativeBlockNode::DerivativeBlockNode(const DerivativeBlockNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	    this->symtab = NULL;
	}

	/* destructor for DerivativeBlock ast node */
	DerivativeBlockNode::~DerivativeBlockNode() {
	    if(name)
	        delete name;
	    if(statementblock)
	        delete statementblock;
	    if(token)
	        delete token;
	}

	/* visit Children method for LinearBlock ast node */
	void LinearBlockNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->solvefor) {
	        for(NameNodeList::iterator iter = this->solvefor->begin();
	            iter != this->solvefor->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for LinearBlock ast node */
	LinearBlockNode::LinearBlockNode(NameNode * name, NameNodeList * solvefor, StatementBlockNode * statementblock) {
	    this->name = name;
	    this->solvefor = solvefor;
	    this->statementblock = statementblock;
	    this->token = NULL;
	    this->symtab = NULL;
	}

	/* copy constructor for LinearBlock ast node */
	LinearBlockNode::LinearBlockNode(const LinearBlockNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    // cloning list
	    if (obj.solvefor) {
	        this->solvefor = new NameNodeList();
	        for(NameNodeList::iterator iter = obj.solvefor->begin();
	            iter != obj.solvefor->end(); iter++) {
	            this->solvefor->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->solvefor = NULL;

	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	    this->symtab = NULL;
	}

	/* destructor for LinearBlock ast node */
	LinearBlockNode::~LinearBlockNode() {
	    if(name)
	        delete name;
	    if (solvefor) {
	        for(NameNodeList::iterator iter = this->solvefor->begin();
	            iter != this->solvefor->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(solvefor)
	        delete solvefor;
	    if(statementblock)
	        delete statementblock;
	    if(token)
	        delete token;
	}

	/* visit Children method for NonLinearBlock ast node */
	void NonLinearBlockNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->solvefor) {
	        for(NameNodeList::iterator iter = this->solvefor->begin();
	            iter != this->solvefor->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for NonLinearBlock ast node */
	NonLinearBlockNode::NonLinearBlockNode(NameNode * name, NameNodeList * solvefor, StatementBlockNode * statementblock) {
	    this->name = name;
	    this->solvefor = solvefor;
	    this->statementblock = statementblock;
	    this->token = NULL;
	    this->symtab = NULL;
	}

	/* copy constructor for NonLinearBlock ast node */
	NonLinearBlockNode::NonLinearBlockNode(const NonLinearBlockNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    // cloning list
	    if (obj.solvefor) {
	        this->solvefor = new NameNodeList();
	        for(NameNodeList::iterator iter = obj.solvefor->begin();
	            iter != obj.solvefor->end(); iter++) {
	            this->solvefor->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->solvefor = NULL;

	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	    this->symtab = NULL;
	}

	/* destructor for NonLinearBlock ast node */
	NonLinearBlockNode::~NonLinearBlockNode() {
	    if(name)
	        delete name;
	    if (solvefor) {
	        for(NameNodeList::iterator iter = this->solvefor->begin();
	            iter != this->solvefor->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(solvefor)
	        delete solvefor;
	    if(statementblock)
	        delete statementblock;
	    if(token)
	        delete token;
	}

	/* visit Children method for DiscreteBlock ast node */
	void DiscreteBlockNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for DiscreteBlock ast node */
	DiscreteBlockNode::DiscreteBlockNode(NameNode * name, StatementBlockNode * statementblock) {
	    this->name = name;
	    this->statementblock = statementblock;
	    this->token = NULL;
	    this->symtab = NULL;
	}

	/* copy constructor for DiscreteBlock ast node */
	DiscreteBlockNode::DiscreteBlockNode(const DiscreteBlockNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	    this->symtab = NULL;
	}

	/* destructor for DiscreteBlock ast node */
	DiscreteBlockNode::~DiscreteBlockNode() {
	    if(name)
	        delete name;
	    if(statementblock)
	        delete statementblock;
	    if(token)
	        delete token;
	}

	/* visit Children method for PartialBlock ast node */
	void PartialBlockNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for PartialBlock ast node */
	PartialBlockNode::PartialBlockNode(NameNode * name, StatementBlockNode * statementblock) {
	    this->name = name;
	    this->statementblock = statementblock;
	    this->token = NULL;
	    this->symtab = NULL;
	}

	/* copy constructor for PartialBlock ast node */
	PartialBlockNode::PartialBlockNode(const PartialBlockNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	    this->symtab = NULL;
	}

	/* destructor for PartialBlock ast node */
	PartialBlockNode::~PartialBlockNode() {
	    if(name)
	        delete name;
	    if(statementblock)
	        delete statementblock;
	    if(token)
	        delete token;
	}

	/* visit Children method for PartialEquation ast node */
	void PartialEquationNode::visitChildren(Visitor* v) {
	    prime->accept(v);
	    name1->accept(v);
	    name2->accept(v);
	    name3->accept(v);
	}

	/* constructor for PartialEquation ast node */
	PartialEquationNode::PartialEquationNode(PrimeNameNode * prime, NameNode * name1, NameNode * name2, NameNode * name3) {
	    this->prime = prime;
	    this->name1 = name1;
	    this->name2 = name2;
	    this->name3 = name3;
	}

	/* copy constructor for PartialEquation ast node */
	PartialEquationNode::PartialEquationNode(const PartialEquationNode& obj) {
	    if(obj.prime)
	        this->prime = obj.prime->clone();
	    else
	        this->prime = NULL;
	    if(obj.name1)
	        this->name1 = obj.name1->clone();
	    else
	        this->name1 = NULL;
	    if(obj.name2)
	        this->name2 = obj.name2->clone();
	    else
	        this->name2 = NULL;
	    if(obj.name3)
	        this->name3 = obj.name3->clone();
	    else
	        this->name3 = NULL;
	}

	/* destructor for PartialEquation ast node */
	PartialEquationNode::~PartialEquationNode() {
	    if(prime)
	        delete prime;
	    if(name1)
	        delete name1;
	    if(name2)
	        delete name2;
	    if(name3)
	        delete name3;
	}

	/* visit Children method for FirstLastTypeIndex ast node */
	void FirstLastTypeIndexNode::visitChildren(Visitor* v) {
	    /* no children */
	}

	/* constructor for FirstLastTypeIndex ast node */
	FirstLastTypeIndexNode::FirstLastTypeIndexNode(FirstLastType  value) {
	    this->value = value;
	}

	/* copy constructor for FirstLastTypeIndex ast node */
	FirstLastTypeIndexNode::FirstLastTypeIndexNode(const FirstLastTypeIndexNode& obj) {
	    this->value = obj.value;
	}

	/* destructor for FirstLastTypeIndex ast node */
	FirstLastTypeIndexNode::~FirstLastTypeIndexNode() {
	}

	/* visit Children method for PartialBoundary ast node */
	void PartialBoundaryNode::visitChildren(Visitor* v) {
	    if (this->del) {
	        this->del->accept(v);
	    }

	    name->accept(v);
	    if (this->index) {
	        this->index->accept(v);
	    }

	    if (this->expression) {
	        this->expression->accept(v);
	    }

	    if (this->name1) {
	        this->name1->accept(v);
	    }

	    if (this->del2) {
	        this->del2->accept(v);
	    }

	    if (this->name2) {
	        this->name2->accept(v);
	    }

	    if (this->name3) {
	        this->name3->accept(v);
	    }

	}

	/* constructor for PartialBoundary ast node */
	PartialBoundaryNode::PartialBoundaryNode(NameNode * del, IdentifierNode * name, FirstLastTypeIndexNode * index, ExpressionNode * expression, NameNode * name1, NameNode * del2, NameNode * name2, NameNode * name3) {
	    this->del = del;
	    this->name = name;
	    this->index = index;
	    this->expression = expression;
	    this->name1 = name1;
	    this->del2 = del2;
	    this->name2 = name2;
	    this->name3 = name3;
	}

	/* copy constructor for PartialBoundary ast node */
	PartialBoundaryNode::PartialBoundaryNode(const PartialBoundaryNode& obj) {
	    if(obj.del)
	        this->del = obj.del->clone();
	    else
	        this->del = NULL;
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.index)
	        this->index = obj.index->clone();
	    else
	        this->index = NULL;
	    if(obj.expression)
	        this->expression = obj.expression->clone();
	    else
	        this->expression = NULL;
	    if(obj.name1)
	        this->name1 = obj.name1->clone();
	    else
	        this->name1 = NULL;
	    if(obj.del2)
	        this->del2 = obj.del2->clone();
	    else
	        this->del2 = NULL;
	    if(obj.name2)
	        this->name2 = obj.name2->clone();
	    else
	        this->name2 = NULL;
	    if(obj.name3)
	        this->name3 = obj.name3->clone();
	    else
	        this->name3 = NULL;
	}

	/* destructor for PartialBoundary ast node */
	PartialBoundaryNode::~PartialBoundaryNode() {
	    if(del)
	        delete del;
	    if(name)
	        delete name;
	    if(index)
	        delete index;
	    if(expression)
	        delete expression;
	    if(name1)
	        delete name1;
	    if(del2)
	        delete del2;
	    if(name2)
	        delete name2;
	    if(name3)
	        delete name3;
	}

	/* visit Children method for FunctionTableBlock ast node */
	void FunctionTableBlockNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->arguments) {
	        for(ArgumentNodeList::iterator iter = this->arguments->begin();
	            iter != this->arguments->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->unit) {
	        this->unit->accept(v);
	    }

	}

	/* constructor for FunctionTableBlock ast node */
	FunctionTableBlockNode::FunctionTableBlockNode(NameNode * name, ArgumentNodeList * arguments, UnitNode * unit) {
	    this->name = name;
	    this->arguments = arguments;
	    this->unit = unit;
	    this->token = NULL;
	    this->symtab = NULL;
	}

	/* copy constructor for FunctionTableBlock ast node */
	FunctionTableBlockNode::FunctionTableBlockNode(const FunctionTableBlockNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    // cloning list
	    if (obj.arguments) {
	        this->arguments = new ArgumentNodeList();
	        for(ArgumentNodeList::iterator iter = obj.arguments->begin();
	            iter != obj.arguments->end(); iter++) {
	            this->arguments->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->arguments = NULL;

	    if(obj.unit)
	        this->unit = obj.unit->clone();
	    else
	        this->unit = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	    this->symtab = NULL;
	}

	/* destructor for FunctionTableBlock ast node */
	FunctionTableBlockNode::~FunctionTableBlockNode() {
	    if(name)
	        delete name;
	    if (arguments) {
	        for(ArgumentNodeList::iterator iter = this->arguments->begin();
	            iter != this->arguments->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(arguments)
	        delete arguments;
	    if(unit)
	        delete unit;
	    if(token)
	        delete token;
	}

	/* visit Children method for FunctionBlock ast node */
	void FunctionBlockNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->arguments) {
	        for(ArgumentNodeList::iterator iter = this->arguments->begin();
	            iter != this->arguments->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->unit) {
	        this->unit->accept(v);
	    }

	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for FunctionBlock ast node */
	FunctionBlockNode::FunctionBlockNode(NameNode * name, ArgumentNodeList * arguments, UnitNode * unit, StatementBlockNode * statementblock) {
	    this->name = name;
	    this->arguments = arguments;
	    this->unit = unit;
	    this->statementblock = statementblock;
	    this->token = NULL;
	    this->symtab = NULL;
	}

	/* copy constructor for FunctionBlock ast node */
	FunctionBlockNode::FunctionBlockNode(const FunctionBlockNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    // cloning list
	    if (obj.arguments) {
	        this->arguments = new ArgumentNodeList();
	        for(ArgumentNodeList::iterator iter = obj.arguments->begin();
	            iter != obj.arguments->end(); iter++) {
	            this->arguments->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->arguments = NULL;

	    if(obj.unit)
	        this->unit = obj.unit->clone();
	    else
	        this->unit = NULL;
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	    this->symtab = NULL;
	}

	/* destructor for FunctionBlock ast node */
	FunctionBlockNode::~FunctionBlockNode() {
	    if(name)
	        delete name;
	    if (arguments) {
	        for(ArgumentNodeList::iterator iter = this->arguments->begin();
	            iter != this->arguments->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(arguments)
	        delete arguments;
	    if(unit)
	        delete unit;
	    if(statementblock)
	        delete statementblock;
	    if(token)
	        delete token;
	}

	/* visit Children method for ProcedureBlock ast node */
	void ProcedureBlockNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->arguments) {
	        for(ArgumentNodeList::iterator iter = this->arguments->begin();
	            iter != this->arguments->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->unit) {
	        this->unit->accept(v);
	    }

	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for ProcedureBlock ast node */
	ProcedureBlockNode::ProcedureBlockNode(NameNode * name, ArgumentNodeList * arguments, UnitNode * unit, StatementBlockNode * statementblock) {
	    this->name = name;
	    this->arguments = arguments;
	    this->unit = unit;
	    this->statementblock = statementblock;
	    this->token = NULL;
	    this->symtab = NULL;
	}

	/* copy constructor for ProcedureBlock ast node */
	ProcedureBlockNode::ProcedureBlockNode(const ProcedureBlockNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    // cloning list
	    if (obj.arguments) {
	        this->arguments = new ArgumentNodeList();
	        for(ArgumentNodeList::iterator iter = obj.arguments->begin();
	            iter != obj.arguments->end(); iter++) {
	            this->arguments->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->arguments = NULL;

	    if(obj.unit)
	        this->unit = obj.unit->clone();
	    else
	        this->unit = NULL;
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	    this->symtab = NULL;
	}

	/* destructor for ProcedureBlock ast node */
	ProcedureBlockNode::~ProcedureBlockNode() {
	    if(name)
	        delete name;
	    if (arguments) {
	        for(ArgumentNodeList::iterator iter = this->arguments->begin();
	            iter != this->arguments->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(arguments)
	        delete arguments;
	    if(unit)
	        delete unit;
	    if(statementblock)
	        delete statementblock;
	    if(token)
	        delete token;
	}

	/* visit Children method for NetReceiveBlock ast node */
	void NetReceiveBlockNode::visitChildren(Visitor* v) {
	    if (this->arguments) {
	        for(ArgumentNodeList::iterator iter = this->arguments->begin();
	            iter != this->arguments->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for NetReceiveBlock ast node */
	NetReceiveBlockNode::NetReceiveBlockNode(ArgumentNodeList * arguments, StatementBlockNode * statementblock) {
	    this->arguments = arguments;
	    this->statementblock = statementblock;
	    this->symtab = NULL;
	}

	/* copy constructor for NetReceiveBlock ast node */
	NetReceiveBlockNode::NetReceiveBlockNode(const NetReceiveBlockNode& obj) {
	    // cloning list
	    if (obj.arguments) {
	        this->arguments = new ArgumentNodeList();
	        for(ArgumentNodeList::iterator iter = obj.arguments->begin();
	            iter != obj.arguments->end(); iter++) {
	            this->arguments->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->arguments = NULL;

	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    this->symtab = NULL;
	}

	/* destructor for NetReceiveBlock ast node */
	NetReceiveBlockNode::~NetReceiveBlockNode() {
	    if (arguments) {
	        for(ArgumentNodeList::iterator iter = this->arguments->begin();
	            iter != this->arguments->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(arguments)
	        delete arguments;
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for SolveBlock ast node */
	void SolveBlockNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->method) {
	        this->method->accept(v);
	    }

	    if (this->ifsolerr) {
	        this->ifsolerr->accept(v);
	    }

	}

	/* constructor for SolveBlock ast node */
	SolveBlockNode::SolveBlockNode(NameNode * name, NameNode * method, StatementBlockNode * ifsolerr) {
	    this->name = name;
	    this->method = method;
	    this->ifsolerr = ifsolerr;
	    this->symtab = NULL;
	}

	/* copy constructor for SolveBlock ast node */
	SolveBlockNode::SolveBlockNode(const SolveBlockNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.method)
	        this->method = obj.method->clone();
	    else
	        this->method = NULL;
	    if(obj.ifsolerr)
	        this->ifsolerr = obj.ifsolerr->clone();
	    else
	        this->ifsolerr = NULL;
	    this->symtab = NULL;
	}

	/* destructor for SolveBlock ast node */
	SolveBlockNode::~SolveBlockNode() {
	    if(name)
	        delete name;
	    if(method)
	        delete method;
	    if(ifsolerr)
	        delete ifsolerr;
	}

	/* visit Children method for BreakpointBlock ast node */
	void BreakpointBlockNode::visitChildren(Visitor* v) {
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for BreakpointBlock ast node */
	BreakpointBlockNode::BreakpointBlockNode(StatementBlockNode * statementblock) {
	    this->statementblock = statementblock;
	    this->symtab = NULL;
	}

	/* copy constructor for BreakpointBlock ast node */
	BreakpointBlockNode::BreakpointBlockNode(const BreakpointBlockNode& obj) {
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    this->symtab = NULL;
	}

	/* destructor for BreakpointBlock ast node */
	BreakpointBlockNode::~BreakpointBlockNode() {
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for TerminalBlock ast node */
	void TerminalBlockNode::visitChildren(Visitor* v) {
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for TerminalBlock ast node */
	TerminalBlockNode::TerminalBlockNode(StatementBlockNode * statementblock) {
	    this->statementblock = statementblock;
	    this->symtab = NULL;
	}

	/* copy constructor for TerminalBlock ast node */
	TerminalBlockNode::TerminalBlockNode(const TerminalBlockNode& obj) {
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    this->symtab = NULL;
	}

	/* destructor for TerminalBlock ast node */
	TerminalBlockNode::~TerminalBlockNode() {
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for BeforeBlock ast node */
	void BeforeBlockNode::visitChildren(Visitor* v) {
	    block->accept(v);
	}

	/* constructor for BeforeBlock ast node */
	BeforeBlockNode::BeforeBlockNode(BABlockNode * block) {
	    this->block = block;
	    this->symtab = NULL;
	}

	/* copy constructor for BeforeBlock ast node */
	BeforeBlockNode::BeforeBlockNode(const BeforeBlockNode& obj) {
	    if(obj.block)
	        this->block = obj.block->clone();
	    else
	        this->block = NULL;
	    this->symtab = NULL;
	}

	/* destructor for BeforeBlock ast node */
	BeforeBlockNode::~BeforeBlockNode() {
	    if(block)
	        delete block;
	}

	/* visit Children method for AfterBlock ast node */
	void AfterBlockNode::visitChildren(Visitor* v) {
	    block->accept(v);
	}

	/* constructor for AfterBlock ast node */
	AfterBlockNode::AfterBlockNode(BABlockNode * block) {
	    this->block = block;
	    this->symtab = NULL;
	}

	/* copy constructor for AfterBlock ast node */
	AfterBlockNode::AfterBlockNode(const AfterBlockNode& obj) {
	    if(obj.block)
	        this->block = obj.block->clone();
	    else
	        this->block = NULL;
	    this->symtab = NULL;
	}

	/* destructor for AfterBlock ast node */
	AfterBlockNode::~AfterBlockNode() {
	    if(block)
	        delete block;
	}

	/* visit Children method for BABlockType ast node */
	void BABlockTypeNode::visitChildren(Visitor* v) {
	    /* no children */
	}

	/* constructor for BABlockType ast node */
	BABlockTypeNode::BABlockTypeNode(BAType  value) {
	    this->value = value;
	}

	/* copy constructor for BABlockType ast node */
	BABlockTypeNode::BABlockTypeNode(const BABlockTypeNode& obj) {
	    this->value = obj.value;
	}

	/* destructor for BABlockType ast node */
	BABlockTypeNode::~BABlockTypeNode() {
	}

	/* visit Children method for BABlock ast node */
	void BABlockNode::visitChildren(Visitor* v) {
	    type->accept(v);
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for BABlock ast node */
	BABlockNode::BABlockNode(BABlockTypeNode * type, StatementBlockNode * statementblock) {
	    this->type = type;
	    this->statementblock = statementblock;
	    this->symtab = NULL;
	}

	/* copy constructor for BABlock ast node */
	BABlockNode::BABlockNode(const BABlockNode& obj) {
	    if(obj.type)
	        this->type = obj.type->clone();
	    else
	        this->type = NULL;
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    this->symtab = NULL;
	}

	/* destructor for BABlock ast node */
	BABlockNode::~BABlockNode() {
	    if(type)
	        delete type;
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for WatchStatement ast node */
	void WatchStatementNode::visitChildren(Visitor* v) {
	    if (this->statements) {
	        for(WatchNodeList::iterator iter = this->statements->begin();
	            iter != this->statements->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for WatchStatement ast node */
	WatchStatementNode::WatchStatementNode(WatchNodeList * statements) {
	    this->statements = statements;
	}

	/* copy constructor for WatchStatement ast node */
	WatchStatementNode::WatchStatementNode(const WatchStatementNode& obj) {
	    // cloning list
	    if (obj.statements) {
	        this->statements = new WatchNodeList();
	        for(WatchNodeList::iterator iter = obj.statements->begin();
	            iter != obj.statements->end(); iter++) {
	            this->statements->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->statements = NULL;

	}

	/* destructor for WatchStatement ast node */
	WatchStatementNode::~WatchStatementNode() {
	    if (statements) {
	        for(WatchNodeList::iterator iter = this->statements->begin();
	            iter != this->statements->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(statements)
	        delete statements;
	}

	/* visit Children method for Watch ast node */
	void WatchNode::visitChildren(Visitor* v) {
	    expression->accept(v);
	    value->accept(v);
	}

	/* constructor for Watch ast node */
	WatchNode::WatchNode(ExpressionNode * expression, ExpressionNode * value) {
	    this->expression = expression;
	    this->value = value;
	}

	/* copy constructor for Watch ast node */
	WatchNode::WatchNode(const WatchNode& obj) {
	    if(obj.expression)
	        this->expression = obj.expression->clone();
	    else
	        this->expression = NULL;
	    if(obj.value)
	        this->value = obj.value->clone();
	    else
	        this->value = NULL;
	}

	/* destructor for Watch ast node */
	WatchNode::~WatchNode() {
	    if(expression)
	        delete expression;
	    if(value)
	        delete value;
	}

	/* visit Children method for ForNetcon ast node */
	void ForNetconNode::visitChildren(Visitor* v) {
	    if (this->arguments) {
	        for(ArgumentNodeList::iterator iter = this->arguments->begin();
	            iter != this->arguments->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for ForNetcon ast node */
	ForNetconNode::ForNetconNode(ArgumentNodeList * arguments, StatementBlockNode * statementblock) {
	    this->arguments = arguments;
	    this->statementblock = statementblock;
	    this->symtab = NULL;
	}

	/* copy constructor for ForNetcon ast node */
	ForNetconNode::ForNetconNode(const ForNetconNode& obj) {
	    // cloning list
	    if (obj.arguments) {
	        this->arguments = new ArgumentNodeList();
	        for(ArgumentNodeList::iterator iter = obj.arguments->begin();
	            iter != obj.arguments->end(); iter++) {
	            this->arguments->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->arguments = NULL;

	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    this->symtab = NULL;
	}

	/* destructor for ForNetcon ast node */
	ForNetconNode::~ForNetconNode() {
	    if (arguments) {
	        for(ArgumentNodeList::iterator iter = this->arguments->begin();
	            iter != this->arguments->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(arguments)
	        delete arguments;
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for MutexLock ast node */
	void MutexLockNode::visitChildren(Visitor* v) {
	}

	/* destructor for MutexLock ast node */
	MutexLockNode::~MutexLockNode() {
	}

	/* visit Children method for MutexUnlock ast node */
	void MutexUnlockNode::visitChildren(Visitor* v) {
	}

	/* destructor for MutexUnlock ast node */
	MutexUnlockNode::~MutexUnlockNode() {
	}

	/* visit Children method for Reset ast node */
	void ResetNode::visitChildren(Visitor* v) {
	}

	/* destructor for Reset ast node */
	ResetNode::~ResetNode() {
	}

	/* visit Children method for Sens ast node */
	void SensNode::visitChildren(Visitor* v) {
	    if (this->senslist) {
	        for(VarNameNodeList::iterator iter = this->senslist->begin();
	            iter != this->senslist->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for Sens ast node */
	SensNode::SensNode(VarNameNodeList * senslist) {
	    this->senslist = senslist;
	}

	/* copy constructor for Sens ast node */
	SensNode::SensNode(const SensNode& obj) {
	    // cloning list
	    if (obj.senslist) {
	        this->senslist = new VarNameNodeList();
	        for(VarNameNodeList::iterator iter = obj.senslist->begin();
	            iter != obj.senslist->end(); iter++) {
	            this->senslist->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->senslist = NULL;

	}

	/* destructor for Sens ast node */
	SensNode::~SensNode() {
	    if (senslist) {
	        for(VarNameNodeList::iterator iter = this->senslist->begin();
	            iter != this->senslist->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(senslist)
	        delete senslist;
	}

	/* visit Children method for Conserve ast node */
	void ConserveNode::visitChildren(Visitor* v) {
	    react->accept(v);
	    expr->accept(v);
	}

	/* constructor for Conserve ast node */
	ConserveNode::ConserveNode(ExpressionNode * react, ExpressionNode * expr) {
	    this->react = react;
	    this->expr = expr;
	}

	/* copy constructor for Conserve ast node */
	ConserveNode::ConserveNode(const ConserveNode& obj) {
	    if(obj.react)
	        this->react = obj.react->clone();
	    else
	        this->react = NULL;
	    if(obj.expr)
	        this->expr = obj.expr->clone();
	    else
	        this->expr = NULL;
	}

	/* destructor for Conserve ast node */
	ConserveNode::~ConserveNode() {
	    if(react)
	        delete react;
	    if(expr)
	        delete expr;
	}

	/* visit Children method for Compartment ast node */
	void CompartmentNode::visitChildren(Visitor* v) {
	    if (this->name) {
	        this->name->accept(v);
	    }

	    expression->accept(v);
	    if (this->names) {
	        for(NameNodeList::iterator iter = this->names->begin();
	            iter != this->names->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for Compartment ast node */
	CompartmentNode::CompartmentNode(NameNode * name, ExpressionNode * expression, NameNodeList * names) {
	    this->name = name;
	    this->expression = expression;
	    this->names = names;
	}

	/* copy constructor for Compartment ast node */
	CompartmentNode::CompartmentNode(const CompartmentNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.expression)
	        this->expression = obj.expression->clone();
	    else
	        this->expression = NULL;
	    // cloning list
	    if (obj.names) {
	        this->names = new NameNodeList();
	        for(NameNodeList::iterator iter = obj.names->begin();
	            iter != obj.names->end(); iter++) {
	            this->names->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->names = NULL;

	}

	/* destructor for Compartment ast node */
	CompartmentNode::~CompartmentNode() {
	    if(name)
	        delete name;
	    if(expression)
	        delete expression;
	    if (names) {
	        for(NameNodeList::iterator iter = this->names->begin();
	            iter != this->names->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(names)
	        delete names;
	}

	/* visit Children method for LDifuse ast node */
	void LDifuseNode::visitChildren(Visitor* v) {
	    if (this->name) {
	        this->name->accept(v);
	    }

	    expression->accept(v);
	    if (this->names) {
	        for(NameNodeList::iterator iter = this->names->begin();
	            iter != this->names->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for LDifuse ast node */
	LDifuseNode::LDifuseNode(NameNode * name, ExpressionNode * expression, NameNodeList * names) {
	    this->name = name;
	    this->expression = expression;
	    this->names = names;
	}

	/* copy constructor for LDifuse ast node */
	LDifuseNode::LDifuseNode(const LDifuseNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.expression)
	        this->expression = obj.expression->clone();
	    else
	        this->expression = NULL;
	    // cloning list
	    if (obj.names) {
	        this->names = new NameNodeList();
	        for(NameNodeList::iterator iter = obj.names->begin();
	            iter != obj.names->end(); iter++) {
	            this->names->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->names = NULL;

	}

	/* destructor for LDifuse ast node */
	LDifuseNode::~LDifuseNode() {
	    if(name)
	        delete name;
	    if(expression)
	        delete expression;
	    if (names) {
	        for(NameNodeList::iterator iter = this->names->begin();
	            iter != this->names->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(names)
	        delete names;
	}

	/* visit Children method for KineticBlock ast node */
	void KineticBlockNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->solvefor) {
	        for(NameNodeList::iterator iter = this->solvefor->begin();
	            iter != this->solvefor->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for KineticBlock ast node */
	KineticBlockNode::KineticBlockNode(NameNode * name, NameNodeList * solvefor, StatementBlockNode * statementblock) {
	    this->name = name;
	    this->solvefor = solvefor;
	    this->statementblock = statementblock;
	    this->token = NULL;
	    this->symtab = NULL;
	}

	/* copy constructor for KineticBlock ast node */
	KineticBlockNode::KineticBlockNode(const KineticBlockNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    // cloning list
	    if (obj.solvefor) {
	        this->solvefor = new NameNodeList();
	        for(NameNodeList::iterator iter = obj.solvefor->begin();
	            iter != obj.solvefor->end(); iter++) {
	            this->solvefor->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->solvefor = NULL;

	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	    this->symtab = NULL;
	}

	/* destructor for KineticBlock ast node */
	KineticBlockNode::~KineticBlockNode() {
	    if(name)
	        delete name;
	    if (solvefor) {
	        for(NameNodeList::iterator iter = this->solvefor->begin();
	            iter != this->solvefor->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(solvefor)
	        delete solvefor;
	    if(statementblock)
	        delete statementblock;
	    if(token)
	        delete token;
	}

	/* visit Children method for ReactionStatement ast node */
	void ReactionStatementNode::visitChildren(Visitor* v) {
	    react1->accept(v);
	    op->accept(v);
	    if (this->react2) {
	        this->react2->accept(v);
	    }

	    expr1->accept(v);
	    if (this->expr2) {
	        this->expr2->accept(v);
	    }

	}

	/* constructor for ReactionStatement ast node */
	ReactionStatementNode::ReactionStatementNode(ExpressionNode * react1, ReactionOperatorNode * op, ExpressionNode * react2, ExpressionNode * expr1, ExpressionNode * expr2) {
	    this->react1 = react1;
	    this->op = op;
	    this->react2 = react2;
	    this->expr1 = expr1;
	    this->expr2 = expr2;
	}

	/* copy constructor for ReactionStatement ast node */
	ReactionStatementNode::ReactionStatementNode(const ReactionStatementNode& obj) {
	    if(obj.react1)
	        this->react1 = obj.react1->clone();
	    else
	        this->react1 = NULL;
	    if(obj.op)
	        this->op = obj.op->clone();
	    else
	        this->op = NULL;
	    if(obj.react2)
	        this->react2 = obj.react2->clone();
	    else
	        this->react2 = NULL;
	    if(obj.expr1)
	        this->expr1 = obj.expr1->clone();
	    else
	        this->expr1 = NULL;
	    if(obj.expr2)
	        this->expr2 = obj.expr2->clone();
	    else
	        this->expr2 = NULL;
	}

	/* destructor for ReactionStatement ast node */
	ReactionStatementNode::~ReactionStatementNode() {
	    if(react1)
	        delete react1;
	    if(op)
	        delete op;
	    if(react2)
	        delete react2;
	    if(expr1)
	        delete expr1;
	    if(expr2)
	        delete expr2;
	}

	/* visit Children method for ReactVarName ast node */
	void ReactVarNameNode::visitChildren(Visitor* v) {
	    if (this->value) {
	        this->value->accept(v);
	    }

	    name->accept(v);
	}

	/* constructor for ReactVarName ast node */
	ReactVarNameNode::ReactVarNameNode(IntegerNode * value, VarNameNode * name) {
	    this->value = value;
	    this->name = name;
	}

	/* copy constructor for ReactVarName ast node */
	ReactVarNameNode::ReactVarNameNode(const ReactVarNameNode& obj) {
	    if(obj.value)
	        this->value = obj.value->clone();
	    else
	        this->value = NULL;
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for ReactVarName ast node */
	ReactVarNameNode::~ReactVarNameNode() {
	    if(value)
	        delete value;
	    if(name)
	        delete name;
	}

	/* visit Children method for LagStatement ast node */
	void LagStatementNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    byname->accept(v);
	}

	/* constructor for LagStatement ast node */
	LagStatementNode::LagStatementNode(IdentifierNode * name, NameNode * byname) {
	    this->name = name;
	    this->byname = byname;
	}

	/* copy constructor for LagStatement ast node */
	LagStatementNode::LagStatementNode(const LagStatementNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.byname)
	        this->byname = obj.byname->clone();
	    else
	        this->byname = NULL;
	}

	/* destructor for LagStatement ast node */
	LagStatementNode::~LagStatementNode() {
	    if(name)
	        delete name;
	    if(byname)
	        delete byname;
	}

	/* visit Children method for QueueStatement ast node */
	void QueueStatementNode::visitChildren(Visitor* v) {
	    qype->accept(v);
	    name->accept(v);
	}

	/* constructor for QueueStatement ast node */
	QueueStatementNode::QueueStatementNode(QueueExpressionTypeNode * qype, IdentifierNode * name) {
	    this->qype = qype;
	    this->name = name;
	}

	/* copy constructor for QueueStatement ast node */
	QueueStatementNode::QueueStatementNode(const QueueStatementNode& obj) {
	    if(obj.qype)
	        this->qype = obj.qype->clone();
	    else
	        this->qype = NULL;
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for QueueStatement ast node */
	QueueStatementNode::~QueueStatementNode() {
	    if(qype)
	        delete qype;
	    if(name)
	        delete name;
	}

	/* visit Children method for QueueExpressionType ast node */
	void QueueExpressionTypeNode::visitChildren(Visitor* v) {
	    /* no children */
	}

	/* constructor for QueueExpressionType ast node */
	QueueExpressionTypeNode::QueueExpressionTypeNode(QueueType  value) {
	    this->value = value;
	}

	/* copy constructor for QueueExpressionType ast node */
	QueueExpressionTypeNode::QueueExpressionTypeNode(const QueueExpressionTypeNode& obj) {
	    this->value = obj.value;
	}

	/* destructor for QueueExpressionType ast node */
	QueueExpressionTypeNode::~QueueExpressionTypeNode() {
	}

	/* visit Children method for MatchBlock ast node */
	void MatchBlockNode::visitChildren(Visitor* v) {
	    if (this->matchs) {
	        for(MatchNodeList::iterator iter = this->matchs->begin();
	            iter != this->matchs->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for MatchBlock ast node */
	MatchBlockNode::MatchBlockNode(MatchNodeList * matchs) {
	    this->matchs = matchs;
	    this->symtab = NULL;
	}

	/* copy constructor for MatchBlock ast node */
	MatchBlockNode::MatchBlockNode(const MatchBlockNode& obj) {
	    // cloning list
	    if (obj.matchs) {
	        this->matchs = new MatchNodeList();
	        for(MatchNodeList::iterator iter = obj.matchs->begin();
	            iter != obj.matchs->end(); iter++) {
	            this->matchs->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->matchs = NULL;

	    this->symtab = NULL;
	}

	/* destructor for MatchBlock ast node */
	MatchBlockNode::~MatchBlockNode() {
	    if (matchs) {
	        for(MatchNodeList::iterator iter = this->matchs->begin();
	            iter != this->matchs->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(matchs)
	        delete matchs;
	}

	/* visit Children method for Match ast node */
	void MatchNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->expression) {
	        this->expression->accept(v);
	    }

	}

	/* constructor for Match ast node */
	MatchNode::MatchNode(IdentifierNode * name, ExpressionNode * expression) {
	    this->name = name;
	    this->expression = expression;
	}

	/* copy constructor for Match ast node */
	MatchNode::MatchNode(const MatchNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.expression)
	        this->expression = obj.expression->clone();
	    else
	        this->expression = NULL;
	}

	/* destructor for Match ast node */
	MatchNode::~MatchNode() {
	    if(name)
	        delete name;
	    if(expression)
	        delete expression;
	}

	/* visit Children method for UnitBlock ast node */
	void UnitBlockNode::visitChildren(Visitor* v) {
	    if (this->definitions) {
	        for(ExpressionNodeList::iterator iter = this->definitions->begin();
	            iter != this->definitions->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for UnitBlock ast node */
	UnitBlockNode::UnitBlockNode(ExpressionNodeList * definitions) {
	    this->definitions = definitions;
	    this->symtab = NULL;
	}

	/* copy constructor for UnitBlock ast node */
	UnitBlockNode::UnitBlockNode(const UnitBlockNode& obj) {
	    // cloning list
	    if (obj.definitions) {
	        this->definitions = new ExpressionNodeList();
	        for(ExpressionNodeList::iterator iter = obj.definitions->begin();
	            iter != obj.definitions->end(); iter++) {
	            this->definitions->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->definitions = NULL;

	    this->symtab = NULL;
	}

	/* destructor for UnitBlock ast node */
	UnitBlockNode::~UnitBlockNode() {
	    if (definitions) {
	        for(ExpressionNodeList::iterator iter = this->definitions->begin();
	            iter != this->definitions->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(definitions)
	        delete definitions;
	}

	/* visit Children method for UnitDef ast node */
	void UnitDefNode::visitChildren(Visitor* v) {
	    unit1->accept(v);
	    unit2->accept(v);
	}

	/* constructor for UnitDef ast node */
	UnitDefNode::UnitDefNode(UnitNode * unit1, UnitNode * unit2) {
	    this->unit1 = unit1;
	    this->unit2 = unit2;
	}

	/* copy constructor for UnitDef ast node */
	UnitDefNode::UnitDefNode(const UnitDefNode& obj) {
	    if(obj.unit1)
	        this->unit1 = obj.unit1->clone();
	    else
	        this->unit1 = NULL;
	    if(obj.unit2)
	        this->unit2 = obj.unit2->clone();
	    else
	        this->unit2 = NULL;
	}

	/* destructor for UnitDef ast node */
	UnitDefNode::~UnitDefNode() {
	    if(unit1)
	        delete unit1;
	    if(unit2)
	        delete unit2;
	}

	/* visit Children method for Factordef ast node */
	void FactordefNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->value) {
	        this->value->accept(v);
	    }

	    unit1->accept(v);
	    if (this->gt) {
	        this->gt->accept(v);
	    }

	    if (this->unit2) {
	        this->unit2->accept(v);
	    }

	}

	/* constructor for Factordef ast node */
	FactordefNode::FactordefNode(NameNode * name, DoubleNode * value, UnitNode * unit1, BooleanNode * gt, UnitNode * unit2) {
	    this->name = name;
	    this->value = value;
	    this->unit1 = unit1;
	    this->gt = gt;
	    this->unit2 = unit2;
	    this->token = NULL;
	}

	/* copy constructor for Factordef ast node */
	FactordefNode::FactordefNode(const FactordefNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.value)
	        this->value = obj.value->clone();
	    else
	        this->value = NULL;
	    if(obj.unit1)
	        this->unit1 = obj.unit1->clone();
	    else
	        this->unit1 = NULL;
	    if(obj.gt)
	        this->gt = obj.gt->clone();
	    else
	        this->gt = NULL;
	    if(obj.unit2)
	        this->unit2 = obj.unit2->clone();
	    else
	        this->unit2 = NULL;
	    if(obj.token)
	        this->token = obj.token->clone();
	    else
	        this->token = NULL;
	}

	/* destructor for Factordef ast node */
	FactordefNode::~FactordefNode() {
	    if(name)
	        delete name;
	    if(value)
	        delete value;
	    if(unit1)
	        delete unit1;
	    if(gt)
	        delete gt;
	    if(unit2)
	        delete unit2;
	    if(token)
	        delete token;
	}

	/* visit Children method for ConstantStatement ast node */
	void ConstantStatementNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    value->accept(v);
	    if (this->unit) {
	        this->unit->accept(v);
	    }

	}

	/* constructor for ConstantStatement ast node */
	ConstantStatementNode::ConstantStatementNode(NameNode * name, NumberNode * value, UnitNode * unit) {
	    this->name = name;
	    this->value = value;
	    this->unit = unit;
	}

	/* copy constructor for ConstantStatement ast node */
	ConstantStatementNode::ConstantStatementNode(const ConstantStatementNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    if(obj.value)
	        this->value = obj.value->clone();
	    else
	        this->value = NULL;
	    if(obj.unit)
	        this->unit = obj.unit->clone();
	    else
	        this->unit = NULL;
	}

	/* destructor for ConstantStatement ast node */
	ConstantStatementNode::~ConstantStatementNode() {
	    if(name)
	        delete name;
	    if(value)
	        delete value;
	    if(unit)
	        delete unit;
	}

	/* visit Children method for ConstantBlock ast node */
	void ConstantBlockNode::visitChildren(Visitor* v) {
	    if (this->statements) {
	        for(ConstantStatementNodeList::iterator iter = this->statements->begin();
	            iter != this->statements->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for ConstantBlock ast node */
	ConstantBlockNode::ConstantBlockNode(ConstantStatementNodeList * statements) {
	    this->statements = statements;
	    this->symtab = NULL;
	}

	/* copy constructor for ConstantBlock ast node */
	ConstantBlockNode::ConstantBlockNode(const ConstantBlockNode& obj) {
	    // cloning list
	    if (obj.statements) {
	        this->statements = new ConstantStatementNodeList();
	        for(ConstantStatementNodeList::iterator iter = obj.statements->begin();
	            iter != obj.statements->end(); iter++) {
	            this->statements->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->statements = NULL;

	    this->symtab = NULL;
	}

	/* destructor for ConstantBlock ast node */
	ConstantBlockNode::~ConstantBlockNode() {
	    if (statements) {
	        for(ConstantStatementNodeList::iterator iter = this->statements->begin();
	            iter != this->statements->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(statements)
	        delete statements;
	}

	/* visit Children method for TableStatement ast node */
	void TableStatementNode::visitChildren(Visitor* v) {
	    if (this->tablst) {
	        for(NameNodeList::iterator iter = this->tablst->begin();
	            iter != this->tablst->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->dependlst) {
	        for(NameNodeList::iterator iter = this->dependlst->begin();
	            iter != this->dependlst->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    from->accept(v);
	    to->accept(v);
	    with->accept(v);
	}

	/* constructor for TableStatement ast node */
	TableStatementNode::TableStatementNode(NameNodeList * tablst, NameNodeList * dependlst, ExpressionNode * from, ExpressionNode * to, IntegerNode * with) {
	    this->tablst = tablst;
	    this->dependlst = dependlst;
	    this->from = from;
	    this->to = to;
	    this->with = with;
	}

	/* copy constructor for TableStatement ast node */
	TableStatementNode::TableStatementNode(const TableStatementNode& obj) {
	    // cloning list
	    if (obj.tablst) {
	        this->tablst = new NameNodeList();
	        for(NameNodeList::iterator iter = obj.tablst->begin();
	            iter != obj.tablst->end(); iter++) {
	            this->tablst->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->tablst = NULL;

	    // cloning list
	    if (obj.dependlst) {
	        this->dependlst = new NameNodeList();
	        for(NameNodeList::iterator iter = obj.dependlst->begin();
	            iter != obj.dependlst->end(); iter++) {
	            this->dependlst->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->dependlst = NULL;

	    if(obj.from)
	        this->from = obj.from->clone();
	    else
	        this->from = NULL;
	    if(obj.to)
	        this->to = obj.to->clone();
	    else
	        this->to = NULL;
	    if(obj.with)
	        this->with = obj.with->clone();
	    else
	        this->with = NULL;
	}

	/* destructor for TableStatement ast node */
	TableStatementNode::~TableStatementNode() {
	    if (tablst) {
	        for(NameNodeList::iterator iter = this->tablst->begin();
	            iter != this->tablst->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(tablst)
	        delete tablst;
	    if (dependlst) {
	        for(NameNodeList::iterator iter = this->dependlst->begin();
	            iter != this->dependlst->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(dependlst)
	        delete dependlst;
	    if(from)
	        delete from;
	    if(to)
	        delete to;
	    if(with)
	        delete with;
	}

	/* visit Children method for NeuronBlock ast node */
	void NeuronBlockNode::visitChildren(Visitor* v) {
	    if (this->statementblock) {
	        this->statementblock->accept(v);
	    }

	}

	/* constructor for NeuronBlock ast node */
	NeuronBlockNode::NeuronBlockNode(StatementBlockNode * statementblock) {
	    this->statementblock = statementblock;
	    this->symtab = NULL;
	}

	/* copy constructor for NeuronBlock ast node */
	NeuronBlockNode::NeuronBlockNode(const NeuronBlockNode& obj) {
	    if(obj.statementblock)
	        this->statementblock = obj.statementblock->clone();
	    else
	        this->statementblock = NULL;
	    this->symtab = NULL;
	}

	/* destructor for NeuronBlock ast node */
	NeuronBlockNode::~NeuronBlockNode() {
	    if(statementblock)
	        delete statementblock;
	}

	/* visit Children method for ReadIonVar ast node */
	void ReadIonVarNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for ReadIonVar ast node */
	ReadIonVarNode::ReadIonVarNode(NameNode * name) {
	    this->name = name;
	}

	/* copy constructor for ReadIonVar ast node */
	ReadIonVarNode::ReadIonVarNode(const ReadIonVarNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for ReadIonVar ast node */
	ReadIonVarNode::~ReadIonVarNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for WriteIonVar ast node */
	void WriteIonVarNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for WriteIonVar ast node */
	WriteIonVarNode::WriteIonVarNode(NameNode * name) {
	    this->name = name;
	}

	/* copy constructor for WriteIonVar ast node */
	WriteIonVarNode::WriteIonVarNode(const WriteIonVarNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for WriteIonVar ast node */
	WriteIonVarNode::~WriteIonVarNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for NonspeCurVar ast node */
	void NonspeCurVarNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for NonspeCurVar ast node */
	NonspeCurVarNode::NonspeCurVarNode(NameNode * name) {
	    this->name = name;
	}

	/* copy constructor for NonspeCurVar ast node */
	NonspeCurVarNode::NonspeCurVarNode(const NonspeCurVarNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for NonspeCurVar ast node */
	NonspeCurVarNode::~NonspeCurVarNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for ElectrodeCurVar ast node */
	void ElectrodeCurVarNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for ElectrodeCurVar ast node */
	ElectrodeCurVarNode::ElectrodeCurVarNode(NameNode * name) {
	    this->name = name;
	}

	/* copy constructor for ElectrodeCurVar ast node */
	ElectrodeCurVarNode::ElectrodeCurVarNode(const ElectrodeCurVarNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for ElectrodeCurVar ast node */
	ElectrodeCurVarNode::~ElectrodeCurVarNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for SectionVar ast node */
	void SectionVarNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for SectionVar ast node */
	SectionVarNode::SectionVarNode(NameNode * name) {
	    this->name = name;
	}

	/* copy constructor for SectionVar ast node */
	SectionVarNode::SectionVarNode(const SectionVarNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for SectionVar ast node */
	SectionVarNode::~SectionVarNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for RangeVar ast node */
	void RangeVarNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for RangeVar ast node */
	RangeVarNode::RangeVarNode(NameNode * name) {
	    this->name = name;
	}

	/* copy constructor for RangeVar ast node */
	RangeVarNode::RangeVarNode(const RangeVarNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for RangeVar ast node */
	RangeVarNode::~RangeVarNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for GlobalVar ast node */
	void GlobalVarNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for GlobalVar ast node */
	GlobalVarNode::GlobalVarNode(NameNode * name) {
	    this->name = name;
	}

	/* copy constructor for GlobalVar ast node */
	GlobalVarNode::GlobalVarNode(const GlobalVarNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for GlobalVar ast node */
	GlobalVarNode::~GlobalVarNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for PointerVar ast node */
	void PointerVarNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for PointerVar ast node */
	PointerVarNode::PointerVarNode(NameNode * name) {
	    this->name = name;
	}

	/* copy constructor for PointerVar ast node */
	PointerVarNode::PointerVarNode(const PointerVarNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for PointerVar ast node */
	PointerVarNode::~PointerVarNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for BbcorePointerVar ast node */
	void BbcorePointerVarNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for BbcorePointerVar ast node */
	BbcorePointerVarNode::BbcorePointerVarNode(NameNode * name) {
	    this->name = name;
	}

	/* copy constructor for BbcorePointerVar ast node */
	BbcorePointerVarNode::BbcorePointerVarNode(const BbcorePointerVarNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for BbcorePointerVar ast node */
	BbcorePointerVarNode::~BbcorePointerVarNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for ExternVar ast node */
	void ExternVarNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for ExternVar ast node */
	ExternVarNode::ExternVarNode(NameNode * name) {
	    this->name = name;
	}

	/* copy constructor for ExternVar ast node */
	ExternVarNode::ExternVarNode(const ExternVarNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for ExternVar ast node */
	ExternVarNode::~ExternVarNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for ThreadsafeVar ast node */
	void ThreadsafeVarNode::visitChildren(Visitor* v) {
	    name->accept(v);
	}

	/* constructor for ThreadsafeVar ast node */
	ThreadsafeVarNode::ThreadsafeVarNode(NameNode * name) {
	    this->name = name;
	}

	/* copy constructor for ThreadsafeVar ast node */
	ThreadsafeVarNode::ThreadsafeVarNode(const ThreadsafeVarNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for ThreadsafeVar ast node */
	ThreadsafeVarNode::~ThreadsafeVarNode() {
	    if(name)
	        delete name;
	}

	/* visit Children method for NrnSuffix ast node */
	void NrnSuffixNode::visitChildren(Visitor* v) {
	    type->accept(v);
	    name->accept(v);
	}

	/* constructor for NrnSuffix ast node */
	NrnSuffixNode::NrnSuffixNode(NameNode * type, NameNode * name) {
	    this->type = type;
	    this->name = name;
	}

	/* copy constructor for NrnSuffix ast node */
	NrnSuffixNode::NrnSuffixNode(const NrnSuffixNode& obj) {
	    if(obj.type)
	        this->type = obj.type->clone();
	    else
	        this->type = NULL;
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	}

	/* destructor for NrnSuffix ast node */
	NrnSuffixNode::~NrnSuffixNode() {
	    if(type)
	        delete type;
	    if(name)
	        delete name;
	}

	/* visit Children method for NrnUseion ast node */
	void NrnUseionNode::visitChildren(Visitor* v) {
	    name->accept(v);
	    if (this->readlist) {
	        for(ReadIonVarNodeList::iterator iter = this->readlist->begin();
	            iter != this->readlist->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->writelist) {
	        for(WriteIonVarNodeList::iterator iter = this->writelist->begin();
	            iter != this->writelist->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	    if (this->valence) {
	        this->valence->accept(v);
	    }

	}

	/* constructor for NrnUseion ast node */
	NrnUseionNode::NrnUseionNode(NameNode * name, ReadIonVarNodeList * readlist, WriteIonVarNodeList * writelist, ValenceNode * valence) {
	    this->name = name;
	    this->readlist = readlist;
	    this->writelist = writelist;
	    this->valence = valence;
	}

	/* copy constructor for NrnUseion ast node */
	NrnUseionNode::NrnUseionNode(const NrnUseionNode& obj) {
	    if(obj.name)
	        this->name = obj.name->clone();
	    else
	        this->name = NULL;
	    // cloning list
	    if (obj.readlist) {
	        this->readlist = new ReadIonVarNodeList();
	        for(ReadIonVarNodeList::iterator iter = obj.readlist->begin();
	            iter != obj.readlist->end(); iter++) {
	            this->readlist->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->readlist = NULL;

	    // cloning list
	    if (obj.writelist) {
	        this->writelist = new WriteIonVarNodeList();
	        for(WriteIonVarNodeList::iterator iter = obj.writelist->begin();
	            iter != obj.writelist->end(); iter++) {
	            this->writelist->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->writelist = NULL;

	    if(obj.valence)
	        this->valence = obj.valence->clone();
	    else
	        this->valence = NULL;
	}

	/* destructor for NrnUseion ast node */
	NrnUseionNode::~NrnUseionNode() {
	    if(name)
	        delete name;
	    if (readlist) {
	        for(ReadIonVarNodeList::iterator iter = this->readlist->begin();
	            iter != this->readlist->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(readlist)
	        delete readlist;
	    if (writelist) {
	        for(WriteIonVarNodeList::iterator iter = this->writelist->begin();
	            iter != this->writelist->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(writelist)
	        delete writelist;
	    if(valence)
	        delete valence;
	}

	/* visit Children method for NrnNonspecific ast node */
	void NrnNonspecificNode::visitChildren(Visitor* v) {
	    if (this->currents) {
	        for(NonspeCurVarNodeList::iterator iter = this->currents->begin();
	            iter != this->currents->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for NrnNonspecific ast node */
	NrnNonspecificNode::NrnNonspecificNode(NonspeCurVarNodeList * currents) {
	    this->currents = currents;
	}

	/* copy constructor for NrnNonspecific ast node */
	NrnNonspecificNode::NrnNonspecificNode(const NrnNonspecificNode& obj) {
	    // cloning list
	    if (obj.currents) {
	        this->currents = new NonspeCurVarNodeList();
	        for(NonspeCurVarNodeList::iterator iter = obj.currents->begin();
	            iter != obj.currents->end(); iter++) {
	            this->currents->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->currents = NULL;

	}

	/* destructor for NrnNonspecific ast node */
	NrnNonspecificNode::~NrnNonspecificNode() {
	    if (currents) {
	        for(NonspeCurVarNodeList::iterator iter = this->currents->begin();
	            iter != this->currents->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(currents)
	        delete currents;
	}

	/* visit Children method for NrnElctrodeCurrent ast node */
	void NrnElctrodeCurrentNode::visitChildren(Visitor* v) {
	    if (this->ecurrents) {
	        for(ElectrodeCurVarNodeList::iterator iter = this->ecurrents->begin();
	            iter != this->ecurrents->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for NrnElctrodeCurrent ast node */
	NrnElctrodeCurrentNode::NrnElctrodeCurrentNode(ElectrodeCurVarNodeList * ecurrents) {
	    this->ecurrents = ecurrents;
	}

	/* copy constructor for NrnElctrodeCurrent ast node */
	NrnElctrodeCurrentNode::NrnElctrodeCurrentNode(const NrnElctrodeCurrentNode& obj) {
	    // cloning list
	    if (obj.ecurrents) {
	        this->ecurrents = new ElectrodeCurVarNodeList();
	        for(ElectrodeCurVarNodeList::iterator iter = obj.ecurrents->begin();
	            iter != obj.ecurrents->end(); iter++) {
	            this->ecurrents->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->ecurrents = NULL;

	}

	/* destructor for NrnElctrodeCurrent ast node */
	NrnElctrodeCurrentNode::~NrnElctrodeCurrentNode() {
	    if (ecurrents) {
	        for(ElectrodeCurVarNodeList::iterator iter = this->ecurrents->begin();
	            iter != this->ecurrents->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(ecurrents)
	        delete ecurrents;
	}

	/* visit Children method for NrnSection ast node */
	void NrnSectionNode::visitChildren(Visitor* v) {
	    if (this->sections) {
	        for(SectionVarNodeList::iterator iter = this->sections->begin();
	            iter != this->sections->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for NrnSection ast node */
	NrnSectionNode::NrnSectionNode(SectionVarNodeList * sections) {
	    this->sections = sections;
	}

	/* copy constructor for NrnSection ast node */
	NrnSectionNode::NrnSectionNode(const NrnSectionNode& obj) {
	    // cloning list
	    if (obj.sections) {
	        this->sections = new SectionVarNodeList();
	        for(SectionVarNodeList::iterator iter = obj.sections->begin();
	            iter != obj.sections->end(); iter++) {
	            this->sections->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->sections = NULL;

	}

	/* destructor for NrnSection ast node */
	NrnSectionNode::~NrnSectionNode() {
	    if (sections) {
	        for(SectionVarNodeList::iterator iter = this->sections->begin();
	            iter != this->sections->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(sections)
	        delete sections;
	}

	/* visit Children method for NrnRange ast node */
	void NrnRangeNode::visitChildren(Visitor* v) {
	    if (this->range_vars) {
	        for(RangeVarNodeList::iterator iter = this->range_vars->begin();
	            iter != this->range_vars->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for NrnRange ast node */
	NrnRangeNode::NrnRangeNode(RangeVarNodeList * range_vars) {
	    this->range_vars = range_vars;
	}

	/* copy constructor for NrnRange ast node */
	NrnRangeNode::NrnRangeNode(const NrnRangeNode& obj) {
	    // cloning list
	    if (obj.range_vars) {
	        this->range_vars = new RangeVarNodeList();
	        for(RangeVarNodeList::iterator iter = obj.range_vars->begin();
	            iter != obj.range_vars->end(); iter++) {
	            this->range_vars->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->range_vars = NULL;

	}

	/* destructor for NrnRange ast node */
	NrnRangeNode::~NrnRangeNode() {
	    if (range_vars) {
	        for(RangeVarNodeList::iterator iter = this->range_vars->begin();
	            iter != this->range_vars->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(range_vars)
	        delete range_vars;
	}

	/* visit Children method for NrnGlobal ast node */
	void NrnGlobalNode::visitChildren(Visitor* v) {
	    if (this->global_vars) {
	        for(GlobalVarNodeList::iterator iter = this->global_vars->begin();
	            iter != this->global_vars->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for NrnGlobal ast node */
	NrnGlobalNode::NrnGlobalNode(GlobalVarNodeList * global_vars) {
	    this->global_vars = global_vars;
	}

	/* copy constructor for NrnGlobal ast node */
	NrnGlobalNode::NrnGlobalNode(const NrnGlobalNode& obj) {
	    // cloning list
	    if (obj.global_vars) {
	        this->global_vars = new GlobalVarNodeList();
	        for(GlobalVarNodeList::iterator iter = obj.global_vars->begin();
	            iter != obj.global_vars->end(); iter++) {
	            this->global_vars->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->global_vars = NULL;

	}

	/* destructor for NrnGlobal ast node */
	NrnGlobalNode::~NrnGlobalNode() {
	    if (global_vars) {
	        for(GlobalVarNodeList::iterator iter = this->global_vars->begin();
	            iter != this->global_vars->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(global_vars)
	        delete global_vars;
	}

	/* visit Children method for NrnPointer ast node */
	void NrnPointerNode::visitChildren(Visitor* v) {
	    if (this->pointers) {
	        for(PointerVarNodeList::iterator iter = this->pointers->begin();
	            iter != this->pointers->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for NrnPointer ast node */
	NrnPointerNode::NrnPointerNode(PointerVarNodeList * pointers) {
	    this->pointers = pointers;
	}

	/* copy constructor for NrnPointer ast node */
	NrnPointerNode::NrnPointerNode(const NrnPointerNode& obj) {
	    // cloning list
	    if (obj.pointers) {
	        this->pointers = new PointerVarNodeList();
	        for(PointerVarNodeList::iterator iter = obj.pointers->begin();
	            iter != obj.pointers->end(); iter++) {
	            this->pointers->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->pointers = NULL;

	}

	/* destructor for NrnPointer ast node */
	NrnPointerNode::~NrnPointerNode() {
	    if (pointers) {
	        for(PointerVarNodeList::iterator iter = this->pointers->begin();
	            iter != this->pointers->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(pointers)
	        delete pointers;
	}

	/* visit Children method for NrnBbcorePtr ast node */
	void NrnBbcorePtrNode::visitChildren(Visitor* v) {
	    if (this->bbcore_pointers) {
	        for(BbcorePointerVarNodeList::iterator iter = this->bbcore_pointers->begin();
	            iter != this->bbcore_pointers->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for NrnBbcorePtr ast node */
	NrnBbcorePtrNode::NrnBbcorePtrNode(BbcorePointerVarNodeList * bbcore_pointers) {
	    this->bbcore_pointers = bbcore_pointers;
	}

	/* copy constructor for NrnBbcorePtr ast node */
	NrnBbcorePtrNode::NrnBbcorePtrNode(const NrnBbcorePtrNode& obj) {
	    // cloning list
	    if (obj.bbcore_pointers) {
	        this->bbcore_pointers = new BbcorePointerVarNodeList();
	        for(BbcorePointerVarNodeList::iterator iter = obj.bbcore_pointers->begin();
	            iter != obj.bbcore_pointers->end(); iter++) {
	            this->bbcore_pointers->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->bbcore_pointers = NULL;

	}

	/* destructor for NrnBbcorePtr ast node */
	NrnBbcorePtrNode::~NrnBbcorePtrNode() {
	    if (bbcore_pointers) {
	        for(BbcorePointerVarNodeList::iterator iter = this->bbcore_pointers->begin();
	            iter != this->bbcore_pointers->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(bbcore_pointers)
	        delete bbcore_pointers;
	}

	/* visit Children method for NrnExternal ast node */
	void NrnExternalNode::visitChildren(Visitor* v) {
	    if (this->externals) {
	        for(ExternVarNodeList::iterator iter = this->externals->begin();
	            iter != this->externals->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for NrnExternal ast node */
	NrnExternalNode::NrnExternalNode(ExternVarNodeList * externals) {
	    this->externals = externals;
	}

	/* copy constructor for NrnExternal ast node */
	NrnExternalNode::NrnExternalNode(const NrnExternalNode& obj) {
	    // cloning list
	    if (obj.externals) {
	        this->externals = new ExternVarNodeList();
	        for(ExternVarNodeList::iterator iter = obj.externals->begin();
	            iter != obj.externals->end(); iter++) {
	            this->externals->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->externals = NULL;

	}

	/* destructor for NrnExternal ast node */
	NrnExternalNode::~NrnExternalNode() {
	    if (externals) {
	        for(ExternVarNodeList::iterator iter = this->externals->begin();
	            iter != this->externals->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(externals)
	        delete externals;
	}

	/* visit Children method for NrnThreadSafe ast node */
	void NrnThreadSafeNode::visitChildren(Visitor* v) {
	    if (this->threadsafe) {
	        for(ThreadsafeVarNodeList::iterator iter = this->threadsafe->begin();
	            iter != this->threadsafe->end(); iter++) {
	            (*iter)->accept(v);
	        }
	    }

	}

	/* constructor for NrnThreadSafe ast node */
	NrnThreadSafeNode::NrnThreadSafeNode(ThreadsafeVarNodeList * threadsafe) {
	    this->threadsafe = threadsafe;
	}

	/* copy constructor for NrnThreadSafe ast node */
	NrnThreadSafeNode::NrnThreadSafeNode(const NrnThreadSafeNode& obj) {
	    // cloning list
	    if (obj.threadsafe) {
	        this->threadsafe = new ThreadsafeVarNodeList();
	        for(ThreadsafeVarNodeList::iterator iter = obj.threadsafe->begin();
	            iter != obj.threadsafe->end(); iter++) {
	            this->threadsafe->push_back((*iter)->clone());
	        }
	    }
	    else
	        this->threadsafe = NULL;

	}

	/* destructor for NrnThreadSafe ast node */
	NrnThreadSafeNode::~NrnThreadSafeNode() {
	    if (threadsafe) {
	        for(ThreadsafeVarNodeList::iterator iter = this->threadsafe->begin();
	            iter != this->threadsafe->end(); iter++) {
	            delete (*iter);
	        }
	    }

	    if(threadsafe)
	        delete threadsafe;
	}

	/* visit Children method for Verbatim ast node */
	void VerbatimNode::visitChildren(Visitor* v) {
	    statement->accept(v);
	}

	/* constructor for Verbatim ast node */
	VerbatimNode::VerbatimNode(StringNode * statement) {
	    this->statement = statement;
	}

	/* copy constructor for Verbatim ast node */
	VerbatimNode::VerbatimNode(const VerbatimNode& obj) {
	    if(obj.statement)
	        this->statement = obj.statement->clone();
	    else
	        this->statement = NULL;
	}

	/* destructor for Verbatim ast node */
	VerbatimNode::~VerbatimNode() {
	    if(statement)
	        delete statement;
	}

	/* visit Children method for Comment ast node */
	void CommentNode::visitChildren(Visitor* v) {
	    comment->accept(v);
	}

	/* constructor for Comment ast node */
	CommentNode::CommentNode(StringNode * comment) {
	    this->comment = comment;
	}

	/* copy constructor for Comment ast node */
	CommentNode::CommentNode(const CommentNode& obj) {
	    if(obj.comment)
	        this->comment = obj.comment->clone();
	    else
	        this->comment = NULL;
	}

	/* destructor for Comment ast node */
	CommentNode::~CommentNode() {
	    if(comment)
	        delete comment;
	}

	/* visit Children method for Valence ast node */
	void ValenceNode::visitChildren(Visitor* v) {
	    type->accept(v);
	    value->accept(v);
	}

	/* constructor for Valence ast node */
	ValenceNode::ValenceNode(NameNode * type, DoubleNode * value) {
	    this->type = type;
	    this->value = value;
	}

	/* copy constructor for Valence ast node */
	ValenceNode::ValenceNode(const ValenceNode& obj) {
	    if(obj.type)
	        this->type = obj.type->clone();
	    else
	        this->type = NULL;
	    if(obj.value)
	        this->value = obj.value->clone();
	    else
	        this->value = NULL;
	}

	/* destructor for Valence ast node */
	ValenceNode::~ValenceNode() {
	    if(type)
	        delete type;
	    if(value)
	        delete value;
	}

}    //namespace ast
