#include "ast/ast.hpp"

namespace ast {
    void Statement::visitChildren(Visitor* v) {}
    void Expression::visitChildren(Visitor* v) {}
    void Block::visitChildren(Visitor* v) {}
    void Identifier::visitChildren(Visitor* v) {}
    void Number::visitChildren(Visitor* v) {}
    void String::visitChildren(Visitor* v) {}
    /* constructor for String ast node */
    String::String(std::string value)
    :    value(value)
    {
    }

    /* copy constructor for String ast node */
    String::String(const String& obj) 
    {
        this->value = obj.value;
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for Integer ast node */
    void Integer::visitChildren(Visitor* v) {
        if (this->macroname) {
            this->macroname->accept(v);
        }
    }

    /* constructor for Integer ast node */
    Integer::Integer(int value, Name* macroname)
    :    value(value)
    {
        this->macroname = std::shared_ptr<Name>(macroname);
    }

    /* copy constructor for Integer ast node */
    Integer::Integer(const Integer& obj) 
    {
        this->value = obj.value;
        if(obj.macroname)
            this->macroname = std::shared_ptr<Name>(obj.macroname->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    void Float::visitChildren(Visitor* v) {}
    /* constructor for Float ast node */
    Float::Float(float value)
    :    value(value)
    {
    }

    /* copy constructor for Float ast node */
    Float::Float(const Float& obj) 
    {
        this->value = obj.value;
    }

    void Double::visitChildren(Visitor* v) {}
    /* constructor for Double ast node */
    Double::Double(double value)
    :    value(value)
    {
    }

    /* copy constructor for Double ast node */
    Double::Double(const Double& obj) 
    {
        this->value = obj.value;
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    void Boolean::visitChildren(Visitor* v) {}
    /* constructor for Boolean ast node */
    Boolean::Boolean(int value)
    :    value(value)
    {
    }

    /* copy constructor for Boolean ast node */
    Boolean::Boolean(const Boolean& obj) 
    {
        this->value = obj.value;
    }

    /* visit method for Name ast node */
    void Name::visitChildren(Visitor* v) {
        value->accept(v);
    }

    /* constructor for Name ast node */
    Name::Name(String* value)
    {
        this->value = std::shared_ptr<String>(value);
    }

    /* copy constructor for Name ast node */
    Name::Name(const Name& obj) 
    {
        if(obj.value)
            this->value = std::shared_ptr<String>(obj.value->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for PrimeName ast node */
    void PrimeName::visitChildren(Visitor* v) {
        value->accept(v);
        order->accept(v);
    }

    /* constructor for PrimeName ast node */
    PrimeName::PrimeName(String* value, Integer* order)
    {
        this->value = std::shared_ptr<String>(value);
        this->order = std::shared_ptr<Integer>(order);
    }

    /* copy constructor for PrimeName ast node */
    PrimeName::PrimeName(const PrimeName& obj) 
    {
        if(obj.value)
            this->value = std::shared_ptr<String>(obj.value->clone());
        if(obj.order)
            this->order = std::shared_ptr<Integer>(obj.order->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for VarName ast node */
    void VarName::visitChildren(Visitor* v) {
        name->accept(v);
        if (this->at_index) {
            this->at_index->accept(v);
        }
    }

    /* constructor for VarName ast node */
    VarName::VarName(Identifier* name, Integer* at_index)
    {
        this->name = std::shared_ptr<Identifier>(name);
        this->at_index = std::shared_ptr<Integer>(at_index);
    }

    /* copy constructor for VarName ast node */
    VarName::VarName(const VarName& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Identifier>(obj.name->clone());
        if(obj.at_index)
            this->at_index = std::shared_ptr<Integer>(obj.at_index->clone());
    }

    /* visit method for IndexedName ast node */
    void IndexedName::visitChildren(Visitor* v) {
        name->accept(v);
        index->accept(v);
    }

    /* constructor for IndexedName ast node */
    IndexedName::IndexedName(Identifier* name, Expression* index)
    {
        this->name = std::shared_ptr<Identifier>(name);
        this->index = std::shared_ptr<Expression>(index);
    }

    /* copy constructor for IndexedName ast node */
    IndexedName::IndexedName(const IndexedName& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Identifier>(obj.name->clone());
        if(obj.index)
            this->index = std::shared_ptr<Expression>(obj.index->clone());
    }

    /* visit method for Argument ast node */
    void Argument::visitChildren(Visitor* v) {
        name->accept(v);
        if (this->unit) {
            this->unit->accept(v);
        }
    }

    /* constructor for Argument ast node */
    Argument::Argument(Identifier* name, Unit* unit)
    {
        this->name = std::shared_ptr<Identifier>(name);
        this->unit = std::shared_ptr<Unit>(unit);
    }

    /* copy constructor for Argument ast node */
    Argument::Argument(const Argument& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Identifier>(obj.name->clone());
        if(obj.unit)
            this->unit = std::shared_ptr<Unit>(obj.unit->clone());
    }

    /* visit method for ReactVarName ast node */
    void ReactVarName::visitChildren(Visitor* v) {
        if (this->value) {
            this->value->accept(v);
        }
        name->accept(v);
    }

    /* constructor for ReactVarName ast node */
    ReactVarName::ReactVarName(Integer* value, VarName* name)
    {
        this->value = std::shared_ptr<Integer>(value);
        this->name = std::shared_ptr<VarName>(name);
    }

    /* copy constructor for ReactVarName ast node */
    ReactVarName::ReactVarName(const ReactVarName& obj) 
    {
        if(obj.value)
            this->value = std::shared_ptr<Integer>(obj.value->clone());
        if(obj.name)
            this->name = std::shared_ptr<VarName>(obj.name->clone());
    }

    /* visit method for ReadIonVar ast node */
    void ReadIonVar::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for ReadIonVar ast node */
    ReadIonVar::ReadIonVar(Name* name)
    {
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for ReadIonVar ast node */
    ReadIonVar::ReadIonVar(const ReadIonVar& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
    }

    /* visit method for WriteIonVar ast node */
    void WriteIonVar::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for WriteIonVar ast node */
    WriteIonVar::WriteIonVar(Name* name)
    {
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for WriteIonVar ast node */
    WriteIonVar::WriteIonVar(const WriteIonVar& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
    }

    /* visit method for NonspeCurVar ast node */
    void NonspeCurVar::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for NonspeCurVar ast node */
    NonspeCurVar::NonspeCurVar(Name* name)
    {
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for NonspeCurVar ast node */
    NonspeCurVar::NonspeCurVar(const NonspeCurVar& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
    }

    /* visit method for ElectrodeCurVar ast node */
    void ElectrodeCurVar::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for ElectrodeCurVar ast node */
    ElectrodeCurVar::ElectrodeCurVar(Name* name)
    {
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for ElectrodeCurVar ast node */
    ElectrodeCurVar::ElectrodeCurVar(const ElectrodeCurVar& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
    }

    /* visit method for SectionVar ast node */
    void SectionVar::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for SectionVar ast node */
    SectionVar::SectionVar(Name* name)
    {
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for SectionVar ast node */
    SectionVar::SectionVar(const SectionVar& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
    }

    /* visit method for RangeVar ast node */
    void RangeVar::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for RangeVar ast node */
    RangeVar::RangeVar(Name* name)
    {
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for RangeVar ast node */
    RangeVar::RangeVar(const RangeVar& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
    }

    /* visit method for GlobalVar ast node */
    void GlobalVar::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for GlobalVar ast node */
    GlobalVar::GlobalVar(Name* name)
    {
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for GlobalVar ast node */
    GlobalVar::GlobalVar(const GlobalVar& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
    }

    /* visit method for PointerVar ast node */
    void PointerVar::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for PointerVar ast node */
    PointerVar::PointerVar(Name* name)
    {
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for PointerVar ast node */
    PointerVar::PointerVar(const PointerVar& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
    }

    /* visit method for BbcorePointerVar ast node */
    void BbcorePointerVar::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for BbcorePointerVar ast node */
    BbcorePointerVar::BbcorePointerVar(Name* name)
    {
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for BbcorePointerVar ast node */
    BbcorePointerVar::BbcorePointerVar(const BbcorePointerVar& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
    }

    /* visit method for ExternVar ast node */
    void ExternVar::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for ExternVar ast node */
    ExternVar::ExternVar(Name* name)
    {
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for ExternVar ast node */
    ExternVar::ExternVar(const ExternVar& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
    }

    /* visit method for ThreadsafeVar ast node */
    void ThreadsafeVar::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for ThreadsafeVar ast node */
    ThreadsafeVar::ThreadsafeVar(Name* name)
    {
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for ThreadsafeVar ast node */
    ThreadsafeVar::ThreadsafeVar(const ThreadsafeVar& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
    }

    /* visit method for ParamBlock ast node */
    void ParamBlock::visitChildren(Visitor* v) {
        for(auto& item : this->statements) {
                item->accept(v);
        }
    }

    /* constructor for ParamBlock ast node */
    ParamBlock::ParamBlock(ParamAssignList statements)
    :    statements(statements)
    {
    }

    /* copy constructor for ParamBlock ast node */
    ParamBlock::ParamBlock(const ParamBlock& obj) 
    {
        for(auto& item : obj.statements) {
            this->statements.push_back(std::shared_ptr< ParamAssign>(item->clone()));
        }
    }

    /* visit method for StepBlock ast node */
    void StepBlock::visitChildren(Visitor* v) {
        for(auto& item : this->statements) {
                item->accept(v);
        }
    }

    /* constructor for StepBlock ast node */
    StepBlock::StepBlock(SteppedList statements)
    :    statements(statements)
    {
    }

    /* copy constructor for StepBlock ast node */
    StepBlock::StepBlock(const StepBlock& obj) 
    {
        for(auto& item : obj.statements) {
            this->statements.push_back(std::shared_ptr< Stepped>(item->clone()));
        }
    }

    /* visit method for IndependentBlock ast node */
    void IndependentBlock::visitChildren(Visitor* v) {
        for(auto& item : this->definitions) {
                item->accept(v);
        }
    }

    /* constructor for IndependentBlock ast node */
    IndependentBlock::IndependentBlock(IndependentDefList definitions)
    :    definitions(definitions)
    {
    }

    /* copy constructor for IndependentBlock ast node */
    IndependentBlock::IndependentBlock(const IndependentBlock& obj) 
    {
        for(auto& item : obj.definitions) {
            this->definitions.push_back(std::shared_ptr< IndependentDef>(item->clone()));
        }
    }

    /* visit method for DependentBlock ast node */
    void DependentBlock::visitChildren(Visitor* v) {
        for(auto& item : this->definitions) {
                item->accept(v);
        }
    }

    /* constructor for DependentBlock ast node */
    DependentBlock::DependentBlock(DependentDefList definitions)
    :    definitions(definitions)
    {
    }

    /* copy constructor for DependentBlock ast node */
    DependentBlock::DependentBlock(const DependentBlock& obj) 
    {
        for(auto& item : obj.definitions) {
            this->definitions.push_back(std::shared_ptr< DependentDef>(item->clone()));
        }
    }

    /* visit method for StateBlock ast node */
    void StateBlock::visitChildren(Visitor* v) {
        for(auto& item : this->definitions) {
                item->accept(v);
        }
    }

    /* constructor for StateBlock ast node */
    StateBlock::StateBlock(DependentDefList definitions)
    :    definitions(definitions)
    {
    }

    /* copy constructor for StateBlock ast node */
    StateBlock::StateBlock(const StateBlock& obj) 
    {
        for(auto& item : obj.definitions) {
            this->definitions.push_back(std::shared_ptr< DependentDef>(item->clone()));
        }
    }

    /* visit method for PlotBlock ast node */
    void PlotBlock::visitChildren(Visitor* v) {
        plot->accept(v);
    }

    /* constructor for PlotBlock ast node */
    PlotBlock::PlotBlock(PlotDeclaration* plot)
    {
        this->plot = std::shared_ptr<PlotDeclaration>(plot);
    }

    /* copy constructor for PlotBlock ast node */
    PlotBlock::PlotBlock(const PlotBlock& obj) 
    {
        if(obj.plot)
            this->plot = std::shared_ptr<PlotDeclaration>(obj.plot->clone());
    }

    /* visit method for InitialBlock ast node */
    void InitialBlock::visitChildren(Visitor* v) {
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for InitialBlock ast node */
    InitialBlock::InitialBlock(StatementBlock* statementblock)
    {
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for InitialBlock ast node */
    InitialBlock::InitialBlock(const InitialBlock& obj) 
    {
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for ConstructorBlock ast node */
    void ConstructorBlock::visitChildren(Visitor* v) {
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for ConstructorBlock ast node */
    ConstructorBlock::ConstructorBlock(StatementBlock* statementblock)
    {
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for ConstructorBlock ast node */
    ConstructorBlock::ConstructorBlock(const ConstructorBlock& obj) 
    {
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for DestructorBlock ast node */
    void DestructorBlock::visitChildren(Visitor* v) {
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for DestructorBlock ast node */
    DestructorBlock::DestructorBlock(StatementBlock* statementblock)
    {
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for DestructorBlock ast node */
    DestructorBlock::DestructorBlock(const DestructorBlock& obj) 
    {
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for StatementBlock ast node */
    void StatementBlock::visitChildren(Visitor* v) {
        for(auto& item : this->statements) {
                item->accept(v);
        }
    }

    /* constructor for StatementBlock ast node */
    StatementBlock::StatementBlock(StatementList statements)
    :    statements(statements)
    {
    }

    /* copy constructor for StatementBlock ast node */
    StatementBlock::StatementBlock(const StatementBlock& obj) 
    {
        for(auto& item : obj.statements) {
            this->statements.push_back(std::shared_ptr< Statement>(item->clone()));
        }
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for DerivativeBlock ast node */
    void DerivativeBlock::visitChildren(Visitor* v) {
        name->accept(v);
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for DerivativeBlock ast node */
    DerivativeBlock::DerivativeBlock(Name* name, StatementBlock* statementblock)
    {
        this->name = std::shared_ptr<Name>(name);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for DerivativeBlock ast node */
    DerivativeBlock::DerivativeBlock(const DerivativeBlock& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for LinearBlock ast node */
    void LinearBlock::visitChildren(Visitor* v) {
        name->accept(v);
        for(auto& item : this->solvefor) {
                item->accept(v);
        }
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for LinearBlock ast node */
    LinearBlock::LinearBlock(Name* name, NameList solvefor, StatementBlock* statementblock)
    :    solvefor(solvefor)
    {
        this->name = std::shared_ptr<Name>(name);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for LinearBlock ast node */
    LinearBlock::LinearBlock(const LinearBlock& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        for(auto& item : obj.solvefor) {
            this->solvefor.push_back(std::shared_ptr< Name>(item->clone()));
        }
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for NonLinearBlock ast node */
    void NonLinearBlock::visitChildren(Visitor* v) {
        name->accept(v);
        for(auto& item : this->solvefor) {
                item->accept(v);
        }
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for NonLinearBlock ast node */
    NonLinearBlock::NonLinearBlock(Name* name, NameList solvefor, StatementBlock* statementblock)
    :    solvefor(solvefor)
    {
        this->name = std::shared_ptr<Name>(name);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for NonLinearBlock ast node */
    NonLinearBlock::NonLinearBlock(const NonLinearBlock& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        for(auto& item : obj.solvefor) {
            this->solvefor.push_back(std::shared_ptr< Name>(item->clone()));
        }
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for DiscreteBlock ast node */
    void DiscreteBlock::visitChildren(Visitor* v) {
        name->accept(v);
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for DiscreteBlock ast node */
    DiscreteBlock::DiscreteBlock(Name* name, StatementBlock* statementblock)
    {
        this->name = std::shared_ptr<Name>(name);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for DiscreteBlock ast node */
    DiscreteBlock::DiscreteBlock(const DiscreteBlock& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for PartialBlock ast node */
    void PartialBlock::visitChildren(Visitor* v) {
        name->accept(v);
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for PartialBlock ast node */
    PartialBlock::PartialBlock(Name* name, StatementBlock* statementblock)
    {
        this->name = std::shared_ptr<Name>(name);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for PartialBlock ast node */
    PartialBlock::PartialBlock(const PartialBlock& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for FunctionTableBlock ast node */
    void FunctionTableBlock::visitChildren(Visitor* v) {
        name->accept(v);
        for(auto& item : this->arguments) {
                item->accept(v);
        }
        if (this->unit) {
            this->unit->accept(v);
        }
    }

    /* constructor for FunctionTableBlock ast node */
    FunctionTableBlock::FunctionTableBlock(Name* name, ArgumentList arguments, Unit* unit)
    :    arguments(arguments)
    {
        this->name = std::shared_ptr<Name>(name);
        this->unit = std::shared_ptr<Unit>(unit);
    }

    /* copy constructor for FunctionTableBlock ast node */
    FunctionTableBlock::FunctionTableBlock(const FunctionTableBlock& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        for(auto& item : obj.arguments) {
            this->arguments.push_back(std::shared_ptr< Argument>(item->clone()));
        }
        if(obj.unit)
            this->unit = std::shared_ptr<Unit>(obj.unit->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for FunctionBlock ast node */
    void FunctionBlock::visitChildren(Visitor* v) {
        name->accept(v);
        for(auto& item : this->arguments) {
                item->accept(v);
        }
        if (this->unit) {
            this->unit->accept(v);
        }
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for FunctionBlock ast node */
    FunctionBlock::FunctionBlock(Name* name, ArgumentList arguments, Unit* unit, StatementBlock* statementblock)
    :    arguments(arguments)
    {
        this->name = std::shared_ptr<Name>(name);
        this->unit = std::shared_ptr<Unit>(unit);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for FunctionBlock ast node */
    FunctionBlock::FunctionBlock(const FunctionBlock& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        for(auto& item : obj.arguments) {
            this->arguments.push_back(std::shared_ptr< Argument>(item->clone()));
        }
        if(obj.unit)
            this->unit = std::shared_ptr<Unit>(obj.unit->clone());
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for ProcedureBlock ast node */
    void ProcedureBlock::visitChildren(Visitor* v) {
        name->accept(v);
        for(auto& item : this->arguments) {
                item->accept(v);
        }
        if (this->unit) {
            this->unit->accept(v);
        }
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for ProcedureBlock ast node */
    ProcedureBlock::ProcedureBlock(Name* name, ArgumentList arguments, Unit* unit, StatementBlock* statementblock)
    :    arguments(arguments)
    {
        this->name = std::shared_ptr<Name>(name);
        this->unit = std::shared_ptr<Unit>(unit);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for ProcedureBlock ast node */
    ProcedureBlock::ProcedureBlock(const ProcedureBlock& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        for(auto& item : obj.arguments) {
            this->arguments.push_back(std::shared_ptr< Argument>(item->clone()));
        }
        if(obj.unit)
            this->unit = std::shared_ptr<Unit>(obj.unit->clone());
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for NetReceiveBlock ast node */
    void NetReceiveBlock::visitChildren(Visitor* v) {
        for(auto& item : this->arguments) {
                item->accept(v);
        }
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for NetReceiveBlock ast node */
    NetReceiveBlock::NetReceiveBlock(ArgumentList arguments, StatementBlock* statementblock)
    :    arguments(arguments)
    {
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for NetReceiveBlock ast node */
    NetReceiveBlock::NetReceiveBlock(const NetReceiveBlock& obj) 
    {
        for(auto& item : obj.arguments) {
            this->arguments.push_back(std::shared_ptr< Argument>(item->clone()));
        }
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for SolveBlock ast node */
    void SolveBlock::visitChildren(Visitor* v) {
        name->accept(v);
        if (this->method) {
            this->method->accept(v);
        }
        if (this->ifsolerr) {
            this->ifsolerr->accept(v);
        }
    }

    /* constructor for SolveBlock ast node */
    SolveBlock::SolveBlock(Name* name, Name* method, StatementBlock* ifsolerr)
    {
        this->name = std::shared_ptr<Name>(name);
        this->method = std::shared_ptr<Name>(method);
        this->ifsolerr = std::shared_ptr<StatementBlock>(ifsolerr);
    }

    /* copy constructor for SolveBlock ast node */
    SolveBlock::SolveBlock(const SolveBlock& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        if(obj.method)
            this->method = std::shared_ptr<Name>(obj.method->clone());
        if(obj.ifsolerr)
            this->ifsolerr = std::shared_ptr<StatementBlock>(obj.ifsolerr->clone());
    }

    /* visit method for BreakpointBlock ast node */
    void BreakpointBlock::visitChildren(Visitor* v) {
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for BreakpointBlock ast node */
    BreakpointBlock::BreakpointBlock(StatementBlock* statementblock)
    {
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for BreakpointBlock ast node */
    BreakpointBlock::BreakpointBlock(const BreakpointBlock& obj) 
    {
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for TerminalBlock ast node */
    void TerminalBlock::visitChildren(Visitor* v) {
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for TerminalBlock ast node */
    TerminalBlock::TerminalBlock(StatementBlock* statementblock)
    {
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for TerminalBlock ast node */
    TerminalBlock::TerminalBlock(const TerminalBlock& obj) 
    {
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for BeforeBlock ast node */
    void BeforeBlock::visitChildren(Visitor* v) {
        block->accept(v);
    }

    /* constructor for BeforeBlock ast node */
    BeforeBlock::BeforeBlock(BABlock* block)
    {
        this->block = std::shared_ptr<BABlock>(block);
    }

    /* copy constructor for BeforeBlock ast node */
    BeforeBlock::BeforeBlock(const BeforeBlock& obj) 
    {
        if(obj.block)
            this->block = std::shared_ptr<BABlock>(obj.block->clone());
    }

    /* visit method for AfterBlock ast node */
    void AfterBlock::visitChildren(Visitor* v) {
        block->accept(v);
    }

    /* constructor for AfterBlock ast node */
    AfterBlock::AfterBlock(BABlock* block)
    {
        this->block = std::shared_ptr<BABlock>(block);
    }

    /* copy constructor for AfterBlock ast node */
    AfterBlock::AfterBlock(const AfterBlock& obj) 
    {
        if(obj.block)
            this->block = std::shared_ptr<BABlock>(obj.block->clone());
    }

    /* visit method for BABlock ast node */
    void BABlock::visitChildren(Visitor* v) {
        type->accept(v);
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for BABlock ast node */
    BABlock::BABlock(BABlockType* type, StatementBlock* statementblock)
    {
        this->type = std::shared_ptr<BABlockType>(type);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for BABlock ast node */
    BABlock::BABlock(const BABlock& obj) 
    {
        if(obj.type)
            this->type = std::shared_ptr<BABlockType>(obj.type->clone());
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for ForNetcon ast node */
    void ForNetcon::visitChildren(Visitor* v) {
        for(auto& item : this->arguments) {
                item->accept(v);
        }
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for ForNetcon ast node */
    ForNetcon::ForNetcon(ArgumentList arguments, StatementBlock* statementblock)
    :    arguments(arguments)
    {
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for ForNetcon ast node */
    ForNetcon::ForNetcon(const ForNetcon& obj) 
    {
        for(auto& item : obj.arguments) {
            this->arguments.push_back(std::shared_ptr< Argument>(item->clone()));
        }
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for KineticBlock ast node */
    void KineticBlock::visitChildren(Visitor* v) {
        name->accept(v);
        for(auto& item : this->solvefor) {
                item->accept(v);
        }
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for KineticBlock ast node */
    KineticBlock::KineticBlock(Name* name, NameList solvefor, StatementBlock* statementblock)
    :    solvefor(solvefor)
    {
        this->name = std::shared_ptr<Name>(name);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for KineticBlock ast node */
    KineticBlock::KineticBlock(const KineticBlock& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        for(auto& item : obj.solvefor) {
            this->solvefor.push_back(std::shared_ptr< Name>(item->clone()));
        }
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for MatchBlock ast node */
    void MatchBlock::visitChildren(Visitor* v) {
        for(auto& item : this->matchs) {
                item->accept(v);
        }
    }

    /* constructor for MatchBlock ast node */
    MatchBlock::MatchBlock(MatchList matchs)
    :    matchs(matchs)
    {
    }

    /* copy constructor for MatchBlock ast node */
    MatchBlock::MatchBlock(const MatchBlock& obj) 
    {
        for(auto& item : obj.matchs) {
            this->matchs.push_back(std::shared_ptr< Match>(item->clone()));
        }
    }

    /* visit method for UnitBlock ast node */
    void UnitBlock::visitChildren(Visitor* v) {
        for(auto& item : this->definitions) {
                item->accept(v);
        }
    }

    /* constructor for UnitBlock ast node */
    UnitBlock::UnitBlock(ExpressionList definitions)
    :    definitions(definitions)
    {
    }

    /* copy constructor for UnitBlock ast node */
    UnitBlock::UnitBlock(const UnitBlock& obj) 
    {
        for(auto& item : obj.definitions) {
            this->definitions.push_back(std::shared_ptr< Expression>(item->clone()));
        }
    }

    /* visit method for ConstantBlock ast node */
    void ConstantBlock::visitChildren(Visitor* v) {
        for(auto& item : this->statements) {
                item->accept(v);
        }
    }

    /* constructor for ConstantBlock ast node */
    ConstantBlock::ConstantBlock(ConstantStatementList statements)
    :    statements(statements)
    {
    }

    /* copy constructor for ConstantBlock ast node */
    ConstantBlock::ConstantBlock(const ConstantBlock& obj) 
    {
        for(auto& item : obj.statements) {
            this->statements.push_back(std::shared_ptr< ConstantStatement>(item->clone()));
        }
    }

    /* visit method for NeuronBlock ast node */
    void NeuronBlock::visitChildren(Visitor* v) {
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for NeuronBlock ast node */
    NeuronBlock::NeuronBlock(StatementBlock* statementblock)
    {
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for NeuronBlock ast node */
    NeuronBlock::NeuronBlock(const NeuronBlock& obj) 
    {
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for Unit ast node */
    void Unit::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for Unit ast node */
    Unit::Unit(String* name)
    {
        this->name = std::shared_ptr<String>(name);
    }

    /* copy constructor for Unit ast node */
    Unit::Unit(const Unit& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<String>(obj.name->clone());
    }

    /* visit method for DoubleUnit ast node */
    void DoubleUnit::visitChildren(Visitor* v) {
        values->accept(v);
        if (this->unit) {
            this->unit->accept(v);
        }
    }

    /* constructor for DoubleUnit ast node */
    DoubleUnit::DoubleUnit(Double* values, Unit* unit)
    {
        this->values = std::shared_ptr<Double>(values);
        this->unit = std::shared_ptr<Unit>(unit);
    }

    /* copy constructor for DoubleUnit ast node */
    DoubleUnit::DoubleUnit(const DoubleUnit& obj) 
    {
        if(obj.values)
            this->values = std::shared_ptr<Double>(obj.values->clone());
        if(obj.unit)
            this->unit = std::shared_ptr<Unit>(obj.unit->clone());
    }

    /* visit method for LocalVariable ast node */
    void LocalVariable::visitChildren(Visitor* v) {
        name->accept(v);
    }

    /* constructor for LocalVariable ast node */
    LocalVariable::LocalVariable(Identifier* name)
    {
        this->name = std::shared_ptr<Identifier>(name);
    }

    /* copy constructor for LocalVariable ast node */
    LocalVariable::LocalVariable(const LocalVariable& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Identifier>(obj.name->clone());
    }

    /* visit method for Limits ast node */
    void Limits::visitChildren(Visitor* v) {
        min->accept(v);
        max->accept(v);
    }

    /* constructor for Limits ast node */
    Limits::Limits(Double* min, Double* max)
    {
        this->min = std::shared_ptr<Double>(min);
        this->max = std::shared_ptr<Double>(max);
    }

    /* copy constructor for Limits ast node */
    Limits::Limits(const Limits& obj) 
    {
        if(obj.min)
            this->min = std::shared_ptr<Double>(obj.min->clone());
        if(obj.max)
            this->max = std::shared_ptr<Double>(obj.max->clone());
    }

    /* visit method for NumberRange ast node */
    void NumberRange::visitChildren(Visitor* v) {
        min->accept(v);
        max->accept(v);
    }

    /* constructor for NumberRange ast node */
    NumberRange::NumberRange(Number* min, Number* max)
    {
        this->min = std::shared_ptr<Number>(min);
        this->max = std::shared_ptr<Number>(max);
    }

    /* copy constructor for NumberRange ast node */
    NumberRange::NumberRange(const NumberRange& obj) 
    {
        if(obj.min)
            this->min = std::shared_ptr<Number>(obj.min->clone());
        if(obj.max)
            this->max = std::shared_ptr<Number>(obj.max->clone());
    }

    /* visit method for PlotVariable ast node */
    void PlotVariable::visitChildren(Visitor* v) {
        name->accept(v);
        if (this->index) {
            this->index->accept(v);
        }
    }

    /* constructor for PlotVariable ast node */
    PlotVariable::PlotVariable(Identifier* name, Integer* index)
    {
        this->name = std::shared_ptr<Identifier>(name);
        this->index = std::shared_ptr<Integer>(index);
    }

    /* copy constructor for PlotVariable ast node */
    PlotVariable::PlotVariable(const PlotVariable& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Identifier>(obj.name->clone());
        if(obj.index)
            this->index = std::shared_ptr<Integer>(obj.index->clone());
    }

    void BinaryOperator::visitChildren(Visitor* v) {}
    /* constructor for BinaryOperator ast node */
    BinaryOperator::BinaryOperator(BinaryOp value)
    :    value(value)
    {
    }

    /* copy constructor for BinaryOperator ast node */
    BinaryOperator::BinaryOperator(const BinaryOperator& obj) 
    {
        this->value = obj.value;
    }

    void UnaryOperator::visitChildren(Visitor* v) {}
    /* constructor for UnaryOperator ast node */
    UnaryOperator::UnaryOperator(UnaryOp value)
    :    value(value)
    {
    }

    /* copy constructor for UnaryOperator ast node */
    UnaryOperator::UnaryOperator(const UnaryOperator& obj) 
    {
        this->value = obj.value;
    }

    void ReactionOperator::visitChildren(Visitor* v) {}
    /* constructor for ReactionOperator ast node */
    ReactionOperator::ReactionOperator(ReactionOp value)
    :    value(value)
    {
    }

    /* copy constructor for ReactionOperator ast node */
    ReactionOperator::ReactionOperator(const ReactionOperator& obj) 
    {
        this->value = obj.value;
    }

    /* visit method for BinaryExpression ast node */
    void BinaryExpression::visitChildren(Visitor* v) {
        lhs->accept(v);
        op.accept(v);
        rhs->accept(v);
    }

    /* constructor for BinaryExpression ast node */
    BinaryExpression::BinaryExpression(Expression* lhs, BinaryOperator op, Expression* rhs)
    :    op(op)
    {
        this->lhs = std::shared_ptr<Expression>(lhs);
        this->rhs = std::shared_ptr<Expression>(rhs);
    }

    /* copy constructor for BinaryExpression ast node */
    BinaryExpression::BinaryExpression(const BinaryExpression& obj) 
    {
        if(obj.lhs)
            this->lhs = std::shared_ptr<Expression>(obj.lhs->clone());
        this->op = obj.op;
        if(obj.rhs)
            this->rhs = std::shared_ptr<Expression>(obj.rhs->clone());
    }

    /* visit method for UnaryExpression ast node */
    void UnaryExpression::visitChildren(Visitor* v) {
        op.accept(v);
        expression->accept(v);
    }

    /* constructor for UnaryExpression ast node */
    UnaryExpression::UnaryExpression(UnaryOperator op, Expression* expression)
    :    op(op)
    {
        this->expression = std::shared_ptr<Expression>(expression);
    }

    /* copy constructor for UnaryExpression ast node */
    UnaryExpression::UnaryExpression(const UnaryExpression& obj) 
    {
        this->op = obj.op;
        if(obj.expression)
            this->expression = std::shared_ptr<Expression>(obj.expression->clone());
    }

    /* visit method for NonLinEuation ast node */
    void NonLinEuation::visitChildren(Visitor* v) {
        lhs->accept(v);
        rhs->accept(v);
    }

    /* constructor for NonLinEuation ast node */
    NonLinEuation::NonLinEuation(Expression* lhs, Expression* rhs)
    {
        this->lhs = std::shared_ptr<Expression>(lhs);
        this->rhs = std::shared_ptr<Expression>(rhs);
    }

    /* copy constructor for NonLinEuation ast node */
    NonLinEuation::NonLinEuation(const NonLinEuation& obj) 
    {
        if(obj.lhs)
            this->lhs = std::shared_ptr<Expression>(obj.lhs->clone());
        if(obj.rhs)
            this->rhs = std::shared_ptr<Expression>(obj.rhs->clone());
    }

    /* visit method for LinEquation ast node */
    void LinEquation::visitChildren(Visitor* v) {
        leftlinexpr->accept(v);
        linexpr->accept(v);
    }

    /* constructor for LinEquation ast node */
    LinEquation::LinEquation(Expression* leftlinexpr, Expression* linexpr)
    {
        this->leftlinexpr = std::shared_ptr<Expression>(leftlinexpr);
        this->linexpr = std::shared_ptr<Expression>(linexpr);
    }

    /* copy constructor for LinEquation ast node */
    LinEquation::LinEquation(const LinEquation& obj) 
    {
        if(obj.leftlinexpr)
            this->leftlinexpr = std::shared_ptr<Expression>(obj.leftlinexpr->clone());
        if(obj.linexpr)
            this->linexpr = std::shared_ptr<Expression>(obj.linexpr->clone());
    }

    /* visit method for FunctionCall ast node */
    void FunctionCall::visitChildren(Visitor* v) {
        name->accept(v);
        for(auto& item : this->arguments) {
                item->accept(v);
        }
    }

    /* constructor for FunctionCall ast node */
    FunctionCall::FunctionCall(Name* name, ExpressionList arguments)
    :    arguments(arguments)
    {
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for FunctionCall ast node */
    FunctionCall::FunctionCall(const FunctionCall& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        for(auto& item : obj.arguments) {
            this->arguments.push_back(std::shared_ptr< Expression>(item->clone()));
        }
    }

    void FirstLastTypeIndex::visitChildren(Visitor* v) {}
    /* constructor for FirstLastTypeIndex ast node */
    FirstLastTypeIndex::FirstLastTypeIndex(FirstLastType value)
    :    value(value)
    {
    }

    /* copy constructor for FirstLastTypeIndex ast node */
    FirstLastTypeIndex::FirstLastTypeIndex(const FirstLastTypeIndex& obj) 
    {
        this->value = obj.value;
    }

    /* visit method for Watch ast node */
    void Watch::visitChildren(Visitor* v) {
        expression->accept(v);
        value->accept(v);
    }

    /* constructor for Watch ast node */
    Watch::Watch(Expression* expression, Expression* value)
    {
        this->expression = std::shared_ptr<Expression>(expression);
        this->value = std::shared_ptr<Expression>(value);
    }

    /* copy constructor for Watch ast node */
    Watch::Watch(const Watch& obj) 
    {
        if(obj.expression)
            this->expression = std::shared_ptr<Expression>(obj.expression->clone());
        if(obj.value)
            this->value = std::shared_ptr<Expression>(obj.value->clone());
    }

    void QueueExpressionType::visitChildren(Visitor* v) {}
    /* constructor for QueueExpressionType ast node */
    QueueExpressionType::QueueExpressionType(QueueType value)
    :    value(value)
    {
    }

    /* copy constructor for QueueExpressionType ast node */
    QueueExpressionType::QueueExpressionType(const QueueExpressionType& obj) 
    {
        this->value = obj.value;
    }

    /* visit method for Match ast node */
    void Match::visitChildren(Visitor* v) {
        name->accept(v);
        if (this->expression) {
            this->expression->accept(v);
        }
    }

    /* constructor for Match ast node */
    Match::Match(Identifier* name, Expression* expression)
    {
        this->name = std::shared_ptr<Identifier>(name);
        this->expression = std::shared_ptr<Expression>(expression);
    }

    /* copy constructor for Match ast node */
    Match::Match(const Match& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Identifier>(obj.name->clone());
        if(obj.expression)
            this->expression = std::shared_ptr<Expression>(obj.expression->clone());
    }

    void BABlockType::visitChildren(Visitor* v) {}
    /* constructor for BABlockType ast node */
    BABlockType::BABlockType(BAType value)
    :    value(value)
    {
    }

    /* copy constructor for BABlockType ast node */
    BABlockType::BABlockType(const BABlockType& obj) 
    {
        this->value = obj.value;
    }

    /* visit method for UnitDef ast node */
    void UnitDef::visitChildren(Visitor* v) {
        unit1->accept(v);
        unit2->accept(v);
    }

    /* constructor for UnitDef ast node */
    UnitDef::UnitDef(Unit* unit1, Unit* unit2)
    {
        this->unit1 = std::shared_ptr<Unit>(unit1);
        this->unit2 = std::shared_ptr<Unit>(unit2);
    }

    /* copy constructor for UnitDef ast node */
    UnitDef::UnitDef(const UnitDef& obj) 
    {
        if(obj.unit1)
            this->unit1 = std::shared_ptr<Unit>(obj.unit1->clone());
        if(obj.unit2)
            this->unit2 = std::shared_ptr<Unit>(obj.unit2->clone());
    }

    /* visit method for FactorDef ast node */
    void FactorDef::visitChildren(Visitor* v) {
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

    /* constructor for FactorDef ast node */
    FactorDef::FactorDef(Name* name, Double* value, Unit* unit1, Boolean* gt, Unit* unit2)
    {
        this->name = std::shared_ptr<Name>(name);
        this->value = std::shared_ptr<Double>(value);
        this->unit1 = std::shared_ptr<Unit>(unit1);
        this->gt = std::shared_ptr<Boolean>(gt);
        this->unit2 = std::shared_ptr<Unit>(unit2);
    }

    /* copy constructor for FactorDef ast node */
    FactorDef::FactorDef(const FactorDef& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        if(obj.value)
            this->value = std::shared_ptr<Double>(obj.value->clone());
        if(obj.unit1)
            this->unit1 = std::shared_ptr<Unit>(obj.unit1->clone());
        if(obj.gt)
            this->gt = std::shared_ptr<Boolean>(obj.gt->clone());
        if(obj.unit2)
            this->unit2 = std::shared_ptr<Unit>(obj.unit2->clone());
        if(obj.token)
            this->token = std::shared_ptr<ModToken>(obj.token->clone());
    }

    /* visit method for Valence ast node */
    void Valence::visitChildren(Visitor* v) {
        type->accept(v);
        value->accept(v);
    }

    /* constructor for Valence ast node */
    Valence::Valence(Name* type, Double* value)
    {
        this->type = std::shared_ptr<Name>(type);
        this->value = std::shared_ptr<Double>(value);
    }

    /* copy constructor for Valence ast node */
    Valence::Valence(const Valence& obj) 
    {
        if(obj.type)
            this->type = std::shared_ptr<Name>(obj.type->clone());
        if(obj.value)
            this->value = std::shared_ptr<Double>(obj.value->clone());
    }

    void UnitState::visitChildren(Visitor* v) {}
    /* constructor for UnitState ast node */
    UnitState::UnitState(UnitStateType value)
    :    value(value)
    {
    }

    /* copy constructor for UnitState ast node */
    UnitState::UnitState(const UnitState& obj) 
    {
        this->value = obj.value;
    }

    /* visit method for LocalListStatement ast node */
    void LocalListStatement::visitChildren(Visitor* v) {
        for(auto& item : this->variables) {
                item->accept(v);
        }
    }

    /* constructor for LocalListStatement ast node */
    LocalListStatement::LocalListStatement(LocalVariableList variables)
    :    variables(variables)
    {
    }

    /* copy constructor for LocalListStatement ast node */
    LocalListStatement::LocalListStatement(const LocalListStatement& obj) 
    {
        for(auto& item : obj.variables) {
            this->variables.push_back(std::shared_ptr< LocalVariable>(item->clone()));
        }
    }

    /* visit method for Model ast node */
    void Model::visitChildren(Visitor* v) {
        title->accept(v);
    }

    /* constructor for Model ast node */
    Model::Model(String* title)
    {
        this->title = std::shared_ptr<String>(title);
    }

    /* copy constructor for Model ast node */
    Model::Model(const Model& obj) 
    {
        if(obj.title)
            this->title = std::shared_ptr<String>(obj.title->clone());
    }

    /* visit method for Define ast node */
    void Define::visitChildren(Visitor* v) {
        name->accept(v);
        value->accept(v);
    }

    /* constructor for Define ast node */
    Define::Define(Name* name, Integer* value)
    {
        this->name = std::shared_ptr<Name>(name);
        this->value = std::shared_ptr<Integer>(value);
    }

    /* copy constructor for Define ast node */
    Define::Define(const Define& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        if(obj.value)
            this->value = std::shared_ptr<Integer>(obj.value->clone());
    }

    /* visit method for Include ast node */
    void Include::visitChildren(Visitor* v) {
        filename->accept(v);
    }

    /* constructor for Include ast node */
    Include::Include(String* filename)
    {
        this->filename = std::shared_ptr<String>(filename);
    }

    /* copy constructor for Include ast node */
    Include::Include(const Include& obj) 
    {
        if(obj.filename)
            this->filename = std::shared_ptr<String>(obj.filename->clone());
    }

    /* visit method for ParamAssign ast node */
    void ParamAssign::visitChildren(Visitor* v) {
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
    ParamAssign::ParamAssign(Identifier* name, Number* value, Unit* unit, Limits* limit)
    {
        this->name = std::shared_ptr<Identifier>(name);
        this->value = std::shared_ptr<Number>(value);
        this->unit = std::shared_ptr<Unit>(unit);
        this->limit = std::shared_ptr<Limits>(limit);
    }

    /* copy constructor for ParamAssign ast node */
    ParamAssign::ParamAssign(const ParamAssign& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Identifier>(obj.name->clone());
        if(obj.value)
            this->value = std::shared_ptr<Number>(obj.value->clone());
        if(obj.unit)
            this->unit = std::shared_ptr<Unit>(obj.unit->clone());
        if(obj.limit)
            this->limit = std::shared_ptr<Limits>(obj.limit->clone());
    }

    /* visit method for Stepped ast node */
    void Stepped::visitChildren(Visitor* v) {
        name->accept(v);
        for(auto& item : this->values) {
                item->accept(v);
        }
        unit->accept(v);
    }

    /* constructor for Stepped ast node */
    Stepped::Stepped(Name* name, NumberList values, Unit* unit)
    :    values(values)
    {
        this->name = std::shared_ptr<Name>(name);
        this->unit = std::shared_ptr<Unit>(unit);
    }

    /* copy constructor for Stepped ast node */
    Stepped::Stepped(const Stepped& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        for(auto& item : obj.values) {
            this->values.push_back(std::shared_ptr< Number>(item->clone()));
        }
        if(obj.unit)
            this->unit = std::shared_ptr<Unit>(obj.unit->clone());
    }

    /* visit method for IndependentDef ast node */
    void IndependentDef::visitChildren(Visitor* v) {
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
    IndependentDef::IndependentDef(Boolean* sweep, Name* name, Number* from, Number* to, Integer* with, Number* opstart, Unit* unit)
    {
        this->sweep = std::shared_ptr<Boolean>(sweep);
        this->name = std::shared_ptr<Name>(name);
        this->from = std::shared_ptr<Number>(from);
        this->to = std::shared_ptr<Number>(to);
        this->with = std::shared_ptr<Integer>(with);
        this->opstart = std::shared_ptr<Number>(opstart);
        this->unit = std::shared_ptr<Unit>(unit);
    }

    /* copy constructor for IndependentDef ast node */
    IndependentDef::IndependentDef(const IndependentDef& obj) 
    {
        if(obj.sweep)
            this->sweep = std::shared_ptr<Boolean>(obj.sweep->clone());
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        if(obj.from)
            this->from = std::shared_ptr<Number>(obj.from->clone());
        if(obj.to)
            this->to = std::shared_ptr<Number>(obj.to->clone());
        if(obj.with)
            this->with = std::shared_ptr<Integer>(obj.with->clone());
        if(obj.opstart)
            this->opstart = std::shared_ptr<Number>(obj.opstart->clone());
        if(obj.unit)
            this->unit = std::shared_ptr<Unit>(obj.unit->clone());
    }

    /* visit method for DependentDef ast node */
    void DependentDef::visitChildren(Visitor* v) {
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
    DependentDef::DependentDef(Identifier* name, Integer* index, Number* from, Number* to, Number* opstart, Unit* unit, Double* abstol)
    {
        this->name = std::shared_ptr<Identifier>(name);
        this->index = std::shared_ptr<Integer>(index);
        this->from = std::shared_ptr<Number>(from);
        this->to = std::shared_ptr<Number>(to);
        this->opstart = std::shared_ptr<Number>(opstart);
        this->unit = std::shared_ptr<Unit>(unit);
        this->abstol = std::shared_ptr<Double>(abstol);
    }

    /* copy constructor for DependentDef ast node */
    DependentDef::DependentDef(const DependentDef& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Identifier>(obj.name->clone());
        if(obj.index)
            this->index = std::shared_ptr<Integer>(obj.index->clone());
        if(obj.from)
            this->from = std::shared_ptr<Number>(obj.from->clone());
        if(obj.to)
            this->to = std::shared_ptr<Number>(obj.to->clone());
        if(obj.opstart)
            this->opstart = std::shared_ptr<Number>(obj.opstart->clone());
        if(obj.unit)
            this->unit = std::shared_ptr<Unit>(obj.unit->clone());
        if(obj.abstol)
            this->abstol = std::shared_ptr<Double>(obj.abstol->clone());
    }

    /* visit method for PlotDeclaration ast node */
    void PlotDeclaration::visitChildren(Visitor* v) {
        for(auto& item : this->pvlist) {
                item->accept(v);
        }
        name->accept(v);
    }

    /* constructor for PlotDeclaration ast node */
    PlotDeclaration::PlotDeclaration(PlotVariableList pvlist, PlotVariable* name)
    :    pvlist(pvlist)
    {
        this->name = std::shared_ptr<PlotVariable>(name);
    }

    /* copy constructor for PlotDeclaration ast node */
    PlotDeclaration::PlotDeclaration(const PlotDeclaration& obj) 
    {
        for(auto& item : obj.pvlist) {
            this->pvlist.push_back(std::shared_ptr< PlotVariable>(item->clone()));
        }
        if(obj.name)
            this->name = std::shared_ptr<PlotVariable>(obj.name->clone());
    }

    /* visit method for ConductanceHint ast node */
    void ConductanceHint::visitChildren(Visitor* v) {
        conductance->accept(v);
        if (this->ion) {
            this->ion->accept(v);
        }
    }

    /* constructor for ConductanceHint ast node */
    ConductanceHint::ConductanceHint(Name* conductance, Name* ion)
    {
        this->conductance = std::shared_ptr<Name>(conductance);
        this->ion = std::shared_ptr<Name>(ion);
    }

    /* copy constructor for ConductanceHint ast node */
    ConductanceHint::ConductanceHint(const ConductanceHint& obj) 
    {
        if(obj.conductance)
            this->conductance = std::shared_ptr<Name>(obj.conductance->clone());
        if(obj.ion)
            this->ion = std::shared_ptr<Name>(obj.ion->clone());
    }

    /* visit method for ExpressionStatement ast node */
    void ExpressionStatement::visitChildren(Visitor* v) {
        expression->accept(v);
    }

    /* constructor for ExpressionStatement ast node */
    ExpressionStatement::ExpressionStatement(Expression* expression)
    {
        this->expression = std::shared_ptr<Expression>(expression);
    }

    /* copy constructor for ExpressionStatement ast node */
    ExpressionStatement::ExpressionStatement(const ExpressionStatement& obj) 
    {
        if(obj.expression)
            this->expression = std::shared_ptr<Expression>(obj.expression->clone());
    }

    /* visit method for ProtectStatement ast node */
    void ProtectStatement::visitChildren(Visitor* v) {
        expression->accept(v);
    }

    /* constructor for ProtectStatement ast node */
    ProtectStatement::ProtectStatement(Expression* expression)
    {
        this->expression = std::shared_ptr<Expression>(expression);
    }

    /* copy constructor for ProtectStatement ast node */
    ProtectStatement::ProtectStatement(const ProtectStatement& obj) 
    {
        if(obj.expression)
            this->expression = std::shared_ptr<Expression>(obj.expression->clone());
    }

    /* visit method for FromStatement ast node */
    void FromStatement::visitChildren(Visitor* v) {
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
    FromStatement::FromStatement(Name* name, Expression* from, Expression* to, Expression* opinc, StatementBlock* statementblock)
    {
        this->name = std::shared_ptr<Name>(name);
        this->from = std::shared_ptr<Expression>(from);
        this->to = std::shared_ptr<Expression>(to);
        this->opinc = std::shared_ptr<Expression>(opinc);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for FromStatement ast node */
    FromStatement::FromStatement(const FromStatement& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        if(obj.from)
            this->from = std::shared_ptr<Expression>(obj.from->clone());
        if(obj.to)
            this->to = std::shared_ptr<Expression>(obj.to->clone());
        if(obj.opinc)
            this->opinc = std::shared_ptr<Expression>(obj.opinc->clone());
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for ForAllStatement ast node */
    void ForAllStatement::visitChildren(Visitor* v) {
        name->accept(v);
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for ForAllStatement ast node */
    ForAllStatement::ForAllStatement(Name* name, StatementBlock* statementblock)
    {
        this->name = std::shared_ptr<Name>(name);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for ForAllStatement ast node */
    ForAllStatement::ForAllStatement(const ForAllStatement& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for WhileStatement ast node */
    void WhileStatement::visitChildren(Visitor* v) {
        condition->accept(v);
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for WhileStatement ast node */
    WhileStatement::WhileStatement(Expression* condition, StatementBlock* statementblock)
    {
        this->condition = std::shared_ptr<Expression>(condition);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for WhileStatement ast node */
    WhileStatement::WhileStatement(const WhileStatement& obj) 
    {
        if(obj.condition)
            this->condition = std::shared_ptr<Expression>(obj.condition->clone());
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for IfStatement ast node */
    void IfStatement::visitChildren(Visitor* v) {
        condition->accept(v);
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
        for(auto& item : this->elseifs) {
                item->accept(v);
        }
        if (this->elses) {
            this->elses->accept(v);
        }
    }

    /* constructor for IfStatement ast node */
    IfStatement::IfStatement(Expression* condition, StatementBlock* statementblock, ElseIfStatementList elseifs, ElseStatement* elses)
    :    elseifs(elseifs)
    {
        this->condition = std::shared_ptr<Expression>(condition);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
        this->elses = std::shared_ptr<ElseStatement>(elses);
    }

    /* copy constructor for IfStatement ast node */
    IfStatement::IfStatement(const IfStatement& obj) 
    {
        if(obj.condition)
            this->condition = std::shared_ptr<Expression>(obj.condition->clone());
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
        for(auto& item : obj.elseifs) {
            this->elseifs.push_back(std::shared_ptr< ElseIfStatement>(item->clone()));
        }
        if(obj.elses)
            this->elses = std::shared_ptr<ElseStatement>(obj.elses->clone());
    }

    /* visit method for ElseIfStatement ast node */
    void ElseIfStatement::visitChildren(Visitor* v) {
        condition->accept(v);
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for ElseIfStatement ast node */
    ElseIfStatement::ElseIfStatement(Expression* condition, StatementBlock* statementblock)
    {
        this->condition = std::shared_ptr<Expression>(condition);
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for ElseIfStatement ast node */
    ElseIfStatement::ElseIfStatement(const ElseIfStatement& obj) 
    {
        if(obj.condition)
            this->condition = std::shared_ptr<Expression>(obj.condition->clone());
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for ElseStatement ast node */
    void ElseStatement::visitChildren(Visitor* v) {
        if (this->statementblock) {
            this->statementblock->accept(v);
        }
    }

    /* constructor for ElseStatement ast node */
    ElseStatement::ElseStatement(StatementBlock* statementblock)
    {
        this->statementblock = std::shared_ptr<StatementBlock>(statementblock);
    }

    /* copy constructor for ElseStatement ast node */
    ElseStatement::ElseStatement(const ElseStatement& obj) 
    {
        if(obj.statementblock)
            this->statementblock = std::shared_ptr<StatementBlock>(obj.statementblock->clone());
    }

    /* visit method for PartialEquation ast node */
    void PartialEquation::visitChildren(Visitor* v) {
        prime->accept(v);
        name1->accept(v);
        name2->accept(v);
        name3->accept(v);
    }

    /* constructor for PartialEquation ast node */
    PartialEquation::PartialEquation(PrimeName* prime, Name* name1, Name* name2, Name* name3)
    {
        this->prime = std::shared_ptr<PrimeName>(prime);
        this->name1 = std::shared_ptr<Name>(name1);
        this->name2 = std::shared_ptr<Name>(name2);
        this->name3 = std::shared_ptr<Name>(name3);
    }

    /* copy constructor for PartialEquation ast node */
    PartialEquation::PartialEquation(const PartialEquation& obj) 
    {
        if(obj.prime)
            this->prime = std::shared_ptr<PrimeName>(obj.prime->clone());
        if(obj.name1)
            this->name1 = std::shared_ptr<Name>(obj.name1->clone());
        if(obj.name2)
            this->name2 = std::shared_ptr<Name>(obj.name2->clone());
        if(obj.name3)
            this->name3 = std::shared_ptr<Name>(obj.name3->clone());
    }

    /* visit method for PartialBoundary ast node */
    void PartialBoundary::visitChildren(Visitor* v) {
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
    PartialBoundary::PartialBoundary(Name* del, Identifier* name, FirstLastTypeIndex* index, Expression* expression, Name* name1, Name* del2, Name* name2, Name* name3)
    {
        this->del = std::shared_ptr<Name>(del);
        this->name = std::shared_ptr<Identifier>(name);
        this->index = std::shared_ptr<FirstLastTypeIndex>(index);
        this->expression = std::shared_ptr<Expression>(expression);
        this->name1 = std::shared_ptr<Name>(name1);
        this->del2 = std::shared_ptr<Name>(del2);
        this->name2 = std::shared_ptr<Name>(name2);
        this->name3 = std::shared_ptr<Name>(name3);
    }

    /* copy constructor for PartialBoundary ast node */
    PartialBoundary::PartialBoundary(const PartialBoundary& obj) 
    {
        if(obj.del)
            this->del = std::shared_ptr<Name>(obj.del->clone());
        if(obj.name)
            this->name = std::shared_ptr<Identifier>(obj.name->clone());
        if(obj.index)
            this->index = std::shared_ptr<FirstLastTypeIndex>(obj.index->clone());
        if(obj.expression)
            this->expression = std::shared_ptr<Expression>(obj.expression->clone());
        if(obj.name1)
            this->name1 = std::shared_ptr<Name>(obj.name1->clone());
        if(obj.del2)
            this->del2 = std::shared_ptr<Name>(obj.del2->clone());
        if(obj.name2)
            this->name2 = std::shared_ptr<Name>(obj.name2->clone());
        if(obj.name3)
            this->name3 = std::shared_ptr<Name>(obj.name3->clone());
    }

    /* visit method for WatchStatement ast node */
    void WatchStatement::visitChildren(Visitor* v) {
        for(auto& item : this->statements) {
                item->accept(v);
        }
    }

    /* constructor for WatchStatement ast node */
    WatchStatement::WatchStatement(WatchList statements)
    :    statements(statements)
    {
    }

    /* copy constructor for WatchStatement ast node */
    WatchStatement::WatchStatement(const WatchStatement& obj) 
    {
        for(auto& item : obj.statements) {
            this->statements.push_back(std::shared_ptr< Watch>(item->clone()));
        }
    }

    void MutexLock::visitChildren(Visitor* v) {}
    void MutexUnlock::visitChildren(Visitor* v) {}
    void Reset::visitChildren(Visitor* v) {}
    /* visit method for Sens ast node */
    void Sens::visitChildren(Visitor* v) {
        for(auto& item : this->senslist) {
                item->accept(v);
        }
    }

    /* constructor for Sens ast node */
    Sens::Sens(VarNameList senslist)
    :    senslist(senslist)
    {
    }

    /* copy constructor for Sens ast node */
    Sens::Sens(const Sens& obj) 
    {
        for(auto& item : obj.senslist) {
            this->senslist.push_back(std::shared_ptr< VarName>(item->clone()));
        }
    }

    /* visit method for Conserve ast node */
    void Conserve::visitChildren(Visitor* v) {
        react->accept(v);
        expr->accept(v);
    }

    /* constructor for Conserve ast node */
    Conserve::Conserve(Expression* react, Expression* expr)
    {
        this->react = std::shared_ptr<Expression>(react);
        this->expr = std::shared_ptr<Expression>(expr);
    }

    /* copy constructor for Conserve ast node */
    Conserve::Conserve(const Conserve& obj) 
    {
        if(obj.react)
            this->react = std::shared_ptr<Expression>(obj.react->clone());
        if(obj.expr)
            this->expr = std::shared_ptr<Expression>(obj.expr->clone());
    }

    /* visit method for Compartment ast node */
    void Compartment::visitChildren(Visitor* v) {
        if (this->name) {
            this->name->accept(v);
        }
        expression->accept(v);
        for(auto& item : this->names) {
                item->accept(v);
        }
    }

    /* constructor for Compartment ast node */
    Compartment::Compartment(Name* name, Expression* expression, NameList names)
    :    names(names)
    {
        this->name = std::shared_ptr<Name>(name);
        this->expression = std::shared_ptr<Expression>(expression);
    }

    /* copy constructor for Compartment ast node */
    Compartment::Compartment(const Compartment& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        if(obj.expression)
            this->expression = std::shared_ptr<Expression>(obj.expression->clone());
        for(auto& item : obj.names) {
            this->names.push_back(std::shared_ptr< Name>(item->clone()));
        }
    }

    /* visit method for LDifuse ast node */
    void LDifuse::visitChildren(Visitor* v) {
        if (this->name) {
            this->name->accept(v);
        }
        expression->accept(v);
        for(auto& item : this->names) {
                item->accept(v);
        }
    }

    /* constructor for LDifuse ast node */
    LDifuse::LDifuse(Name* name, Expression* expression, NameList names)
    :    names(names)
    {
        this->name = std::shared_ptr<Name>(name);
        this->expression = std::shared_ptr<Expression>(expression);
    }

    /* copy constructor for LDifuse ast node */
    LDifuse::LDifuse(const LDifuse& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        if(obj.expression)
            this->expression = std::shared_ptr<Expression>(obj.expression->clone());
        for(auto& item : obj.names) {
            this->names.push_back(std::shared_ptr< Name>(item->clone()));
        }
    }

    /* visit method for ReactionStatement ast node */
    void ReactionStatement::visitChildren(Visitor* v) {
        react1->accept(v);
        op.accept(v);
        if (this->react2) {
            this->react2->accept(v);
        }
        expr1->accept(v);
        if (this->expr2) {
            this->expr2->accept(v);
        }
    }

    /* constructor for ReactionStatement ast node */
    ReactionStatement::ReactionStatement(Expression* react1, ReactionOperator op, Expression* react2, Expression* expr1, Expression* expr2)
    :    op(op)
    {
        this->react1 = std::shared_ptr<Expression>(react1);
        this->react2 = std::shared_ptr<Expression>(react2);
        this->expr1 = std::shared_ptr<Expression>(expr1);
        this->expr2 = std::shared_ptr<Expression>(expr2);
    }

    /* copy constructor for ReactionStatement ast node */
    ReactionStatement::ReactionStatement(const ReactionStatement& obj) 
    {
        if(obj.react1)
            this->react1 = std::shared_ptr<Expression>(obj.react1->clone());
        this->op = obj.op;
        if(obj.react2)
            this->react2 = std::shared_ptr<Expression>(obj.react2->clone());
        if(obj.expr1)
            this->expr1 = std::shared_ptr<Expression>(obj.expr1->clone());
        if(obj.expr2)
            this->expr2 = std::shared_ptr<Expression>(obj.expr2->clone());
    }

    /* visit method for LagStatement ast node */
    void LagStatement::visitChildren(Visitor* v) {
        name->accept(v);
        byname->accept(v);
    }

    /* constructor for LagStatement ast node */
    LagStatement::LagStatement(Identifier* name, Name* byname)
    {
        this->name = std::shared_ptr<Identifier>(name);
        this->byname = std::shared_ptr<Name>(byname);
    }

    /* copy constructor for LagStatement ast node */
    LagStatement::LagStatement(const LagStatement& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Identifier>(obj.name->clone());
        if(obj.byname)
            this->byname = std::shared_ptr<Name>(obj.byname->clone());
    }

    /* visit method for QueueStatement ast node */
    void QueueStatement::visitChildren(Visitor* v) {
        qype->accept(v);
        name->accept(v);
    }

    /* constructor for QueueStatement ast node */
    QueueStatement::QueueStatement(QueueExpressionType* qype, Identifier* name)
    {
        this->qype = std::shared_ptr<QueueExpressionType>(qype);
        this->name = std::shared_ptr<Identifier>(name);
    }

    /* copy constructor for QueueStatement ast node */
    QueueStatement::QueueStatement(const QueueStatement& obj) 
    {
        if(obj.qype)
            this->qype = std::shared_ptr<QueueExpressionType>(obj.qype->clone());
        if(obj.name)
            this->name = std::shared_ptr<Identifier>(obj.name->clone());
    }

    /* visit method for ConstantStatement ast node */
    void ConstantStatement::visitChildren(Visitor* v) {
        name->accept(v);
        value->accept(v);
        if (this->unit) {
            this->unit->accept(v);
        }
    }

    /* constructor for ConstantStatement ast node */
    ConstantStatement::ConstantStatement(Name* name, Number* value, Unit* unit)
    {
        this->name = std::shared_ptr<Name>(name);
        this->value = std::shared_ptr<Number>(value);
        this->unit = std::shared_ptr<Unit>(unit);
    }

    /* copy constructor for ConstantStatement ast node */
    ConstantStatement::ConstantStatement(const ConstantStatement& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        if(obj.value)
            this->value = std::shared_ptr<Number>(obj.value->clone());
        if(obj.unit)
            this->unit = std::shared_ptr<Unit>(obj.unit->clone());
    }

    /* visit method for TableStatement ast node */
    void TableStatement::visitChildren(Visitor* v) {
        for(auto& item : this->tablst) {
                item->accept(v);
        }
        for(auto& item : this->dependlst) {
                item->accept(v);
        }
        from->accept(v);
        to->accept(v);
        with->accept(v);
    }

    /* constructor for TableStatement ast node */
    TableStatement::TableStatement(NameList tablst, NameList dependlst, Expression* from, Expression* to, Integer* with)
    :    tablst(tablst),
dependlst(dependlst)
    {
        this->from = std::shared_ptr<Expression>(from);
        this->to = std::shared_ptr<Expression>(to);
        this->with = std::shared_ptr<Integer>(with);
    }

    /* copy constructor for TableStatement ast node */
    TableStatement::TableStatement(const TableStatement& obj) 
    {
        for(auto& item : obj.tablst) {
            this->tablst.push_back(std::shared_ptr< Name>(item->clone()));
        }
        for(auto& item : obj.dependlst) {
            this->dependlst.push_back(std::shared_ptr< Name>(item->clone()));
        }
        if(obj.from)
            this->from = std::shared_ptr<Expression>(obj.from->clone());
        if(obj.to)
            this->to = std::shared_ptr<Expression>(obj.to->clone());
        if(obj.with)
            this->with = std::shared_ptr<Integer>(obj.with->clone());
    }

    /* visit method for NrnSuffix ast node */
    void NrnSuffix::visitChildren(Visitor* v) {
        type->accept(v);
        name->accept(v);
    }

    /* constructor for NrnSuffix ast node */
    NrnSuffix::NrnSuffix(Name* type, Name* name)
    {
        this->type = std::shared_ptr<Name>(type);
        this->name = std::shared_ptr<Name>(name);
    }

    /* copy constructor for NrnSuffix ast node */
    NrnSuffix::NrnSuffix(const NrnSuffix& obj) 
    {
        if(obj.type)
            this->type = std::shared_ptr<Name>(obj.type->clone());
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
    }

    /* visit method for NrnUseion ast node */
    void NrnUseion::visitChildren(Visitor* v) {
        name->accept(v);
        for(auto& item : this->readlist) {
                item->accept(v);
        }
        for(auto& item : this->writelist) {
                item->accept(v);
        }
        if (this->valence) {
            this->valence->accept(v);
        }
    }

    /* constructor for NrnUseion ast node */
    NrnUseion::NrnUseion(Name* name, ReadIonVarList readlist, WriteIonVarList writelist, Valence* valence)
    :    readlist(readlist),
writelist(writelist)
    {
        this->name = std::shared_ptr<Name>(name);
        this->valence = std::shared_ptr<Valence>(valence);
    }

    /* copy constructor for NrnUseion ast node */
    NrnUseion::NrnUseion(const NrnUseion& obj) 
    {
        if(obj.name)
            this->name = std::shared_ptr<Name>(obj.name->clone());
        for(auto& item : obj.readlist) {
            this->readlist.push_back(std::shared_ptr< ReadIonVar>(item->clone()));
        }
        for(auto& item : obj.writelist) {
            this->writelist.push_back(std::shared_ptr< WriteIonVar>(item->clone()));
        }
        if(obj.valence)
            this->valence = std::shared_ptr<Valence>(obj.valence->clone());
    }

    /* visit method for NrnNonspecific ast node */
    void NrnNonspecific::visitChildren(Visitor* v) {
        for(auto& item : this->currents) {
                item->accept(v);
        }
    }

    /* constructor for NrnNonspecific ast node */
    NrnNonspecific::NrnNonspecific(NonspeCurVarList currents)
    :    currents(currents)
    {
    }

    /* copy constructor for NrnNonspecific ast node */
    NrnNonspecific::NrnNonspecific(const NrnNonspecific& obj) 
    {
        for(auto& item : obj.currents) {
            this->currents.push_back(std::shared_ptr< NonspeCurVar>(item->clone()));
        }
    }

    /* visit method for NrnElctrodeCurrent ast node */
    void NrnElctrodeCurrent::visitChildren(Visitor* v) {
        for(auto& item : this->ecurrents) {
                item->accept(v);
        }
    }

    /* constructor for NrnElctrodeCurrent ast node */
    NrnElctrodeCurrent::NrnElctrodeCurrent(ElectrodeCurVarList ecurrents)
    :    ecurrents(ecurrents)
    {
    }

    /* copy constructor for NrnElctrodeCurrent ast node */
    NrnElctrodeCurrent::NrnElctrodeCurrent(const NrnElctrodeCurrent& obj) 
    {
        for(auto& item : obj.ecurrents) {
            this->ecurrents.push_back(std::shared_ptr< ElectrodeCurVar>(item->clone()));
        }
    }

    /* visit method for NrnSection ast node */
    void NrnSection::visitChildren(Visitor* v) {
        for(auto& item : this->sections) {
                item->accept(v);
        }
    }

    /* constructor for NrnSection ast node */
    NrnSection::NrnSection(SectionVarList sections)
    :    sections(sections)
    {
    }

    /* copy constructor for NrnSection ast node */
    NrnSection::NrnSection(const NrnSection& obj) 
    {
        for(auto& item : obj.sections) {
            this->sections.push_back(std::shared_ptr< SectionVar>(item->clone()));
        }
    }

    /* visit method for NrnRange ast node */
    void NrnRange::visitChildren(Visitor* v) {
        for(auto& item : this->range_vars) {
                item->accept(v);
        }
    }

    /* constructor for NrnRange ast node */
    NrnRange::NrnRange(RangeVarList range_vars)
    :    range_vars(range_vars)
    {
    }

    /* copy constructor for NrnRange ast node */
    NrnRange::NrnRange(const NrnRange& obj) 
    {
        for(auto& item : obj.range_vars) {
            this->range_vars.push_back(std::shared_ptr< RangeVar>(item->clone()));
        }
    }

    /* visit method for NrnGlobal ast node */
    void NrnGlobal::visitChildren(Visitor* v) {
        for(auto& item : this->global_vars) {
                item->accept(v);
        }
    }

    /* constructor for NrnGlobal ast node */
    NrnGlobal::NrnGlobal(GlobalVarList global_vars)
    :    global_vars(global_vars)
    {
    }

    /* copy constructor for NrnGlobal ast node */
    NrnGlobal::NrnGlobal(const NrnGlobal& obj) 
    {
        for(auto& item : obj.global_vars) {
            this->global_vars.push_back(std::shared_ptr< GlobalVar>(item->clone()));
        }
    }

    /* visit method for NrnPointer ast node */
    void NrnPointer::visitChildren(Visitor* v) {
        for(auto& item : this->pointers) {
                item->accept(v);
        }
    }

    /* constructor for NrnPointer ast node */
    NrnPointer::NrnPointer(PointerVarList pointers)
    :    pointers(pointers)
    {
    }

    /* copy constructor for NrnPointer ast node */
    NrnPointer::NrnPointer(const NrnPointer& obj) 
    {
        for(auto& item : obj.pointers) {
            this->pointers.push_back(std::shared_ptr< PointerVar>(item->clone()));
        }
    }

    /* visit method for NrnBbcorePtr ast node */
    void NrnBbcorePtr::visitChildren(Visitor* v) {
        for(auto& item : this->bbcore_pointers) {
                item->accept(v);
        }
    }

    /* constructor for NrnBbcorePtr ast node */
    NrnBbcorePtr::NrnBbcorePtr(BbcorePointerVarList bbcore_pointers)
    :    bbcore_pointers(bbcore_pointers)
    {
    }

    /* copy constructor for NrnBbcorePtr ast node */
    NrnBbcorePtr::NrnBbcorePtr(const NrnBbcorePtr& obj) 
    {
        for(auto& item : obj.bbcore_pointers) {
            this->bbcore_pointers.push_back(std::shared_ptr< BbcorePointerVar>(item->clone()));
        }
    }

    /* visit method for NrnExternal ast node */
    void NrnExternal::visitChildren(Visitor* v) {
        for(auto& item : this->externals) {
                item->accept(v);
        }
    }

    /* constructor for NrnExternal ast node */
    NrnExternal::NrnExternal(ExternVarList externals)
    :    externals(externals)
    {
    }

    /* copy constructor for NrnExternal ast node */
    NrnExternal::NrnExternal(const NrnExternal& obj) 
    {
        for(auto& item : obj.externals) {
            this->externals.push_back(std::shared_ptr< ExternVar>(item->clone()));
        }
    }

    /* visit method for NrnThreadSafe ast node */
    void NrnThreadSafe::visitChildren(Visitor* v) {
        for(auto& item : this->threadsafe) {
                item->accept(v);
        }
    }

    /* constructor for NrnThreadSafe ast node */
    NrnThreadSafe::NrnThreadSafe(ThreadsafeVarList threadsafe)
    :    threadsafe(threadsafe)
    {
    }

    /* copy constructor for NrnThreadSafe ast node */
    NrnThreadSafe::NrnThreadSafe(const NrnThreadSafe& obj) 
    {
        for(auto& item : obj.threadsafe) {
            this->threadsafe.push_back(std::shared_ptr< ThreadsafeVar>(item->clone()));
        }
    }

    /* visit method for Verbatim ast node */
    void Verbatim::visitChildren(Visitor* v) {
        statement->accept(v);
    }

    /* constructor for Verbatim ast node */
    Verbatim::Verbatim(String* statement)
    {
        this->statement = std::shared_ptr<String>(statement);
    }

    /* copy constructor for Verbatim ast node */
    Verbatim::Verbatim(const Verbatim& obj) 
    {
        if(obj.statement)
            this->statement = std::shared_ptr<String>(obj.statement->clone());
    }

    /* visit method for Comment ast node */
    void Comment::visitChildren(Visitor* v) {
        comment->accept(v);
    }

    /* constructor for Comment ast node */
    Comment::Comment(String* comment)
    {
        this->comment = std::shared_ptr<String>(comment);
    }

    /* copy constructor for Comment ast node */
    Comment::Comment(const Comment& obj) 
    {
        if(obj.comment)
            this->comment = std::shared_ptr<String>(obj.comment->clone());
    }

    /* visit method for Program ast node */
    void Program::visitChildren(Visitor* v) {
        for(auto& item : this->statements) {
                item->accept(v);
        }
        for(auto& item : this->blocks) {
                item->accept(v);
        }
    }

    /* constructor for Program ast node */
    Program::Program(StatementList statements, BlockList blocks)
    :    statements(statements),
blocks(blocks)
    {
    }

    /* copy constructor for Program ast node */
    Program::Program(const Program& obj) 
    {
        for(auto& item : obj.statements) {
            this->statements.push_back(std::shared_ptr< Statement>(item->clone()));
        }
        for(auto& item : obj.blocks) {
            this->blocks.push_back(std::shared_ptr< Block>(item->clone()));
        }
    }

} // namespace ast
