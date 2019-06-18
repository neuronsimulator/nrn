/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \dir
 * \brief AST Implementation
 *
 * \file
 * \brief Implementation of AST base class and it's properties
 */

#include <memory>
#include <string>

#include "ast/ast_decl.hpp"
#include "lexer/modtoken.hpp"
#include "symtab/symbol_table.hpp"
#include "visitors/visitor.hpp"

namespace nmodl {

/// Abstract Syntax Tree (AST) related implementations
namespace ast {

/**
 * @defgroup ast AST Implementation
 * @brief All AST related implementation details
 *
 * @defgroup ast_prop AST Properties
 * @ingroup ast
 * @brief Properties and types used with of AST classes
 * @{
 */

/**
 * \brief enum Type for binary operators in NMODL
 *
 * NMODL support different binary operators and this
 * type is used to store their value in the AST.
 */
typedef enum {
    BOP_ADDITION,        ///< \+
    BOP_SUBTRACTION,     ///< --
    BOP_MULTIPLICATION,  ///< \c *
    BOP_DIVISION,        ///< \/
    BOP_POWER,           ///< ^
    BOP_AND,             ///< &&
    BOP_OR,              ///< ||
    BOP_GREATER,         ///< >
    BOP_LESS,            ///< <
    BOP_GREATER_EQUAL,   ///< >=
    BOP_LESS_EQUAL,      ///< <=
    BOP_ASSIGN,          ///< =
    BOP_NOT_EQUAL,       ///< !=
    BOP_EXACT_EQUAL      ///< ==
} BinaryOp;

/**
 * \brief string representation of ast::BinaryOp
 *
 * When AST is converted back to NMODL or C code, ast::BinaryOpNames
 * is used to lookup the corresponding symbol for the operator.
 */
static const std::string BinaryOpNames[] =
    {"+", "-", "*", "/", "^", "&&", "||", ">", "<", ">=", "<=", "=", "!=", "=="};

/// enum type for unary operators
typedef enum { UOP_NOT, UOP_NEGATION } UnaryOp;

/// string representation of ast::UnaryOp
static const std::string UnaryOpNames[] = {"!", "-"};

/// enum type for partial equation types
typedef enum { PEQ_FIRST, PEQ_LAST } FirstLastType;

/// string representation of ast::FirstLastType
static const std::string FirstLastTypeNames[] = {"FIRST", "LAST"};

/// enum type for queue types
typedef enum { PUT_QUEUE, GET_QUEUE } QueueType;

/// string representation of ast::QueueType
static const std::string QueueTypeNames[] = {"PUTQ", "GETQ"};

/// enum type to distinguish BEFORE or AFTER blocks
typedef enum { BATYPE_BREAKPOINT, BATYPE_SOLVE, BATYPE_INITIAL, BATYPE_STEP } BAType;

/// string representation of ast::BAType
static const std::string BATypeNames[] = {"BREAKPOINT", "SOLVE", "INITIAL", "STEP"};

/// enum type used for UNIT_ON or UNIT_OFF state
typedef enum { UNIT_ON, UNIT_OFF } UnitStateType;

/// string representation of ast::UnitStateType
static const std::string UnitStateTypeNames[] = {"UNITSON", "UNITSOFF"};

/// enum type used for Reaction statement
typedef enum { LTMINUSGT, LTLT, MINUSGT } ReactionOp;

/// string representation of ast::ReactionOp
static const std::string ReactionOpNames[] = {"<->", "<<", "->"};

/** @} */  // end of ast_prop


/**
 * \page ast_design Design of Abstract Syntax Tree (AST)
 *
 * This page describes the AST design aspects.
 *
 * \tableofcontents
 *
 * \section sec_1 AST Class Hierarchy
 * This section describes the AST design
 *
 * \section sec_2 Block Scoped Nodes
 * This section describes block scoped nodes.
 *
 * \section sec_3 Symbol Table Nodes
 * This section describes nodes with symbol table.
 */

/**
 * @defgroup ast_class AST Classes
 * @ingroup ast
 * @brief Classes for implementing Abstract Syntax Tree (AST)
 * @{
 */

/**
 * \brief Base class for all Abstract Syntax Tree node types
 *
 * Every node in the Abstract Syntax Tree is inherited from base class
 * ast::Ast. This class provides base properties and pure virtual
 * functions that must be implemented by base classes. We inherit from
 * std::enable_shared_from_this to get a `shared_ptr` from `this` pointer.
 *
 * \todo With the ast::Node as another top level node, this can be removed
 *       in the future.
 */
struct Ast: public std::enable_shared_from_this<Ast> {
    /// \name Ctor & dtor
    /// \{

    Ast() = default;

    virtual ~Ast() {}

    /// \}


    /// \name Pure Virtual Functions
    /// \{

    /**
     * \brief Return type (ast::AstNodeType) of AST node
     *
     * Every node in the ast has a type defined in ast::AstNodeType.
     * This type is can be used to check/compare node types.
     */
    virtual AstNodeType get_node_type() = 0;

    /**
     * \brief Return type (ast::AstNodeType) of ast node as std::string
     *
     * Every node in the ast has a type defined in ast::AstNodeType.
     * This type name can be returned as a std::string for printing
     * ast to text/json form.
     *
     * @return name of the node type as a string
     *
     * \sa Ast::get_node_name
     */
    virtual std::string get_node_type_name() = 0;

    /**
     * \brief Return NMODL statement of ast node as std::string
     *
     * Every node is related to a special statement in the NMODL. This
     * statement can be returned as a std::string for printing to
     * text/json form.
     *
     * @return name of the statement as a string
     *
     * \sa Ast::get_nmodl_name
     */
    virtual std::string get_nmodl_name() {
        throw std::runtime_error("get_nmodl_name not implemented");
    }

    /**
     * \brief Accept (or visit) the AST node using current visitor
     *
     * Instead of visiting children of AST node, like Ast::visit_children,
     * accept allows to visit the current node itself using the concrete
     * visitor provided.
     *
     * @param v Concrete visitor that will be used to recursively visit children
     *
     * \note Below is an example of `accept` method implementation which shows how
     *       visitor method corresponding to ast::IndexedName node is called allowing
     *       to visit the node itself in the visitor.
     *
     * \code{.cpp}
     *   void IndexedName::accept(visitor::Visitor* v) override {
     *       v->visit_indexed_name(this);
     *   }
     * \endcode
     *
     */
    virtual void accept(visitor::Visitor* v) = 0;

    /**
     * \brief Visit children i.e. member of AST node using provided visitor
     *
     * Different nodes in the AST have different members (i.e. children). This method
     * recursively visits children using provided concrete visitor.
     *
     * @param v Concrete visitor that will be used to recursively visit node
     *
     * \note Below is an example of `visit_children` method implementation which shows
     *       ast::IndexedName node children are visited instead of node itself.
     *
     * \code{.cpp}
     * void IndexedName::visit_children(visitor::Visitor* v) {
     *    name->accept(v);
     *    length->accept(v);
     * }
     * \endcode
     */
    virtual void visit_children(visitor::Visitor* v) = 0;

    /**
     * \brief Create a copy of the current node
     *
     * Recursively make a new copy/clone of the current node including
     * all members and return a pointer to the node. This is used for
     * passes like nmodl::visitor::InlineVisitor where nodes are cloned in the
     * ast.
     *
     * @return pointer to the clone/copy of the current node
     */
    virtual Ast* clone() {
        throw std::logic_error("clone not implemented");
    }

    /// \}


    /// \name Not implemented
    /// \{

    /**
     * \brief Return name of of the node
     *
     * Some ast nodes have a member marked designated as node name. For example,
     * in case of ast::FunctionCall, name of the function is returned as a node
     * name. Note that this is different from ast node type name and not every
     * ast node has name.
     *
     * @return name of the node as std::string
     *
     * \sa Ast::get_node_type_name
     */
    virtual std::string get_node_name() {
        throw std::logic_error("get_node_name() not implemented");
    }

    /**
     * \brief Return associated token for the AST node
     *
     * Not all ast nodes have token information. For example, nmodl::visitor::NeuronSolveVisitor
     * can insert new nodes in the ast as a solution of ODEs. In this case, we return
     * nullptr to store in the nmodl::symtab::SymbolTable.
     *
     * @return pointer to token if exist otherwise nullptr
     */
    virtual ModToken* get_token() {
        return nullptr;
    }

    /**
     * \brief Return associated symbol table for the AST node
     *
     * Certain ast nodes (e.g. inherited from ast::Block) have associated
     * symbol table. These nodes have nmodl::symtab::SymbolTable as member
     * and it can be accessed using this method.
     *
     * @return pointer to the symbol table
     *
     * \sa nmodl::symtab::SymbolTable nmodl::visitor::SymtabVisitor
     */
    virtual symtab::SymbolTable* get_symbol_table() {
        throw std::runtime_error("get_symbol_table not implemented");
    }

    /**
     * \brief Return associated statement block for the AST node
     *
     * Top level block nodes encloses all statements in the ast::StatementBlock.
     * For example, ast::BreakpointBlock has all statements in curly brace (`{ }`)
     * stored in ast::StatementBlock :
     *
     * \code
     *     BREAKPOINT {
     *         SOLVE states METHOD cnexp
     *         gNaTs2_t = gNaTs2_tbar*m*m*m*h
     *         ina = gNaTs2_t*(v-ena)
     *     }
     * \endcode
     *
     * This method return enclosing statement block.
     *
     * @return pointer to the statement block if exist
     *
     * \sa ast::StatementBlock
     */
    virtual std::shared_ptr<StatementBlock> get_statement_block() {
        throw std::runtime_error("get_statement_block not implemented");
    }

    /**
     * \brief Set symbol table for the AST node
     *
     * Top level, block scoped nodes store symbol table in the ast node.
     * nmodl::visitor::SymtabVisitor then used this method to setup symbol table
     * for every node in the ast.
     *
     * \sa nmodl::visitor::SymtabVisitor
     */
    virtual void set_symbol_table(symtab::SymbolTable* /*symtab*/) {
        throw std::runtime_error("set_symbol_table not implemented");
    }

    /**
     * \brief Set name for the AST node
     *
     * Some ast nodes have a member marked designated as node name (e.g. nodes
     * derived from ast::Identifier). This method is used to set new name for those
     * nodes. This useful for passes like nmodl::visitor::RenameVisitor.
     *
     * \sa Ast::get_node_type_name Ast::get_node_name
     */
    virtual void set_name(std::string /*name*/) {
        throw std::runtime_error("set_name not implemented");
    }

    /**
     * \brief Negate the value of AST node
     *
     * Parser parse `-x` in two parts : `x` and then `-`. Once second token
     * `-` is encountered, the corresponding value of ast node needs to be
     * multiplied by `-1` for ast::Number node types or apply `!` operator
     for the nodes of type ast::Boolean.
     */
    virtual void negate() {
        throw std::runtime_error("negate not implemented");
    }
    /// \}

    /// get std::shared_ptr from `this` pointer of the AST node
    virtual std::shared_ptr<Ast> get_shared_ptr() {
        return std::static_pointer_cast<Ast>(shared_from_this());
    }

    /**
     *\brief Check if the ast node is an instance of ast::Ast
     * @return true if object of type ast::Ast
     */
    virtual bool is_ast() {
        return true;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Node
     * @return true if object of type ast::Node
     */
    virtual bool is_node() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Statement
     * @return true if object of type ast::Statement
     */
    virtual bool is_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Expression
     * @return true if object of type ast::Expression
     */
    virtual bool is_expression() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Block
     * @return true if object of type ast::Block
     */
    virtual bool is_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Identifier
     * @return true if object of type ast::Identifier
     */
    virtual bool is_identifier() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Number
     * @return true if object of type ast::Number
     */
    virtual bool is_number() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::String
     * @return true if object of type ast::String
     */
    virtual bool is_string() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Integer
     * @return true if object of type ast::Integer
     */
    virtual bool is_integer() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Float
     * @return true if object of type ast::Float
     */
    virtual bool is_float() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Double
     * @return true if object of type ast::Double
     */
    virtual bool is_double() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Boolean
     * @return true if object of type ast::Boolean
     */
    virtual bool is_boolean() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Name
     * @return true if object of type ast::Name
     */
    virtual bool is_name() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::PrimeName
     * @return true if object of type ast::PrimeName
     */
    virtual bool is_prime_name() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::VarName
     * @return true if object of type ast::VarName
     */
    virtual bool is_var_name() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::IndexedName
     * @return true if object of type ast::IndexedName
     */
    virtual bool is_indexed_name() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Argument
     * @return true if object of type ast::Argument
     */
    virtual bool is_argument() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ReactVarName
     * @return true if object of type ast::ReactVarName
     */
    virtual bool is_react_var_name() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ReadIonVar
     * @return true if object of type ast::ReadIonVar
     */
    virtual bool is_read_ion_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::WriteIonVar
     * @return true if object of type ast::WriteIonVar
     */
    virtual bool is_write_ion_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::NonspecificCurVar
     * @return true if object of type ast::NonspecificCurVar
     */
    virtual bool is_nonspecific_cur_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ElectrodeCurVar
     * @return true if object of type ast::ElectrodeCurVar
     */
    virtual bool is_electrode_cur_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::SectionVar
     * @return true if object of type ast::SectionVar
     */
    virtual bool is_section_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::RangeVar
     * @return true if object of type ast::RangeVar
     */
    virtual bool is_range_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::GlobalVar
     * @return true if object of type ast::GlobalVar
     */
    virtual bool is_global_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::PointerVar
     * @return true if object of type ast::PointerVar
     */
    virtual bool is_pointer_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::BbcorePointerVar
     * @return true if object of type ast::BbcorePointerVar
     */
    virtual bool is_bbcore_pointer_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ExternVar
     * @return true if object of type ast::ExternVar
     */
    virtual bool is_extern_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ThreadsafeVar
     * @return true if object of type ast::ThreadsafeVar
     */
    virtual bool is_threadsafe_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ParamBlock
     * @return true if object of type ast::ParamBlock
     */
    virtual bool is_param_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::StepBlock
     * @return true if object of type ast::StepBlock
     */
    virtual bool is_step_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::IndependentBlock
     * @return true if object of type ast::IndependentBlock
     */
    virtual bool is_independent_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::AssignedBlock
     * @return true if object of type ast::AssignedBlock
     */
    virtual bool is_assigned_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::StateBlock
     * @return true if object of type ast::StateBlock
     */
    virtual bool is_state_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::PlotBlock
     * @return true if object of type ast::PlotBlock
     */
    virtual bool is_plot_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::InitialBlock
     * @return true if object of type ast::InitialBlock
     */
    virtual bool is_initial_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ConstructorBlock
     * @return true if object of type ast::ConstructorBlock
     */
    virtual bool is_constructor_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::DestructorBlock
     * @return true if object of type ast::DestructorBlock
     */
    virtual bool is_destructor_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::StatementBlock
     * @return true if object of type ast::StatementBlock
     */
    virtual bool is_statement_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::DerivativeBlock
     * @return true if object of type ast::DerivativeBlock
     */
    virtual bool is_derivative_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::LinearBlock
     * @return true if object of type ast::LinearBlock
     */
    virtual bool is_linear_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::NonLinearBlock
     * @return true if object of type ast::NonLinearBlock
     */
    virtual bool is_non_linear_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::DiscreteBlock
     * @return true if object of type ast::DiscreteBlock
     */
    virtual bool is_discrete_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::PartialBlock
     * @return true if object of type ast::PartialBlock
     */
    virtual bool is_partial_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::FunctionTableBlock
     * @return true if object of type ast::FunctionTableBlock
     */
    virtual bool is_function_table_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::FunctionBlock
     * @return true if object of type ast::FunctionBlock
     */
    virtual bool is_function_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ProcedureBlock
     * @return true if object of type ast::ProcedureBlock
     */
    virtual bool is_procedure_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::NetReceiveBlock
     * @return true if object of type ast::NetReceiveBlock
     */
    virtual bool is_net_receive_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::SolveBlock
     * @return true if object of type ast::SolveBlock
     */
    virtual bool is_solve_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::BreakpointBlock
     * @return true if object of type ast::BreakpointBlock
     */
    virtual bool is_breakpoint_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::TerminalBlock
     * @return true if object of type ast::TerminalBlock
     */
    virtual bool is_terminal_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::BeforeBlock
     * @return true if object of type ast::BeforeBlock
     */
    virtual bool is_before_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::AfterBlock
     * @return true if object of type ast::AfterBlock
     */
    virtual bool is_after_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::BABlock
     * @return true if object of type ast::BABlock
     */
    virtual bool is_ba_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ForNetcon
     * @return true if object of type ast::ForNetcon
     */
    virtual bool is_for_netcon() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::KineticBlock
     * @return true if object of type ast::KineticBlock
     */
    virtual bool is_kinetic_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::MatchBlock
     * @return true if object of type ast::MatchBlock
     */
    virtual bool is_match_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::UnitBlock
     * @return true if object of type ast::UnitBlock
     */
    virtual bool is_unit_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ConstantBlock
     * @return true if object of type ast::ConstantBlock
     */
    virtual bool is_constant_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::NeuronBlock
     * @return true if object of type ast::NeuronBlock
     */
    virtual bool is_neuron_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Unit
     * @return true if object of type ast::Unit
     */
    virtual bool is_unit() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::DoubleUnit
     * @return true if object of type ast::DoubleUnit
     */
    virtual bool is_double_unit() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::LocalVar
     * @return true if object of type ast::LocalVar
     */
    virtual bool is_local_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Limits
     * @return true if object of type ast::Limits
     */
    virtual bool is_limits() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::NumberRange
     * @return true if object of type ast::NumberRange
     */
    virtual bool is_number_range() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::PlotVar
     * @return true if object of type ast::PlotVar
     */
    virtual bool is_plot_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ConstantVar
     * @return true if object of type ast::ConstantVar
     */
    virtual bool is_constant_var() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::BinaryOperator
     * @return true if object of type ast::BinaryOperator
     */
    virtual bool is_binary_operator() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::UnaryOperator
     * @return true if object of type ast::UnaryOperator
     */
    virtual bool is_unary_operator() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ReactionOperator
     * @return true if object of type ast::ReactionOperator
     */
    virtual bool is_reaction_operator() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ParenExpression
     * @return true if object of type ast::ParenExpression
     */
    virtual bool is_paren_expression() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::BinaryExpression
     * @return true if object of type ast::BinaryExpression
     */
    virtual bool is_binary_expression() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::DiffEqExpression
     * @return true if object of type ast::DiffEqExpression
     */
    virtual bool is_diff_eq_expression() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::UnaryExpression
     * @return true if object of type ast::UnaryExpression
     */
    virtual bool is_unary_expression() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::NonLinEquation
     * @return true if object of type ast::NonLinEquation
     */
    virtual bool is_non_lin_equation() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::LinEquation
     * @return true if object of type ast::LinEquation
     */
    virtual bool is_lin_equation() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::FunctionCall
     * @return true if object of type ast::FunctionCall
     */
    virtual bool is_function_call() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::FirstLastTypeIndex
     * @return true if object of type ast::FirstLastTypeIndex
     */
    virtual bool is_first_last_type_index() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Watch
     * @return true if object of type ast::Watch
     */
    virtual bool is_watch() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::QueueExpressionType
     * @return true if object of type ast::QueueExpressionType
     */
    virtual bool is_queue_expression_type() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Match
     * @return true if object of type ast::Match
     */
    virtual bool is_match() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::BABlockType
     * @return true if object of type ast::BABlockType
     */
    virtual bool is_ba_block_type() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::UnitDef
     * @return true if object of type ast::UnitDef
     */
    virtual bool is_unit_def() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::FactorDef
     * @return true if object of type ast::FactorDef
     */
    virtual bool is_factor_def() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Valence
     * @return true if object of type ast::Valence
     */
    virtual bool is_valence() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::UnitState
     * @return true if object of type ast::UnitState
     */
    virtual bool is_unit_state() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::LocalListStatement
     * @return true if object of type ast::LocalListStatement
     */
    virtual bool is_local_list_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Model
     * @return true if object of type ast::Model
     */
    virtual bool is_model() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Define
     * @return true if object of type ast::Define
     */
    virtual bool is_define() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Include
     * @return true if object of type ast::Include
     */
    virtual bool is_include() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ParamAssign
     * @return true if object of type ast::ParamAssign
     */
    virtual bool is_param_assign() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Stepped
     * @return true if object of type ast::Stepped
     */
    virtual bool is_stepped() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::IndependentDefinition
     * @return true if object of type ast::IndependentDefinition
     */
    virtual bool is_independent_definition() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::AssignedDefinition
     * @return true if object of type ast::AssignedDefinition
     */
    virtual bool is_assigned_definition() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::PlotDeclaration
     * @return true if object of type ast::PlotDeclaration
     */
    virtual bool is_plot_declaration() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ConductanceHint
     * @return true if object of type ast::ConductanceHint
     */
    virtual bool is_conductance_hint() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ExpressionStatement
     * @return true if object of type ast::ExpressionStatement
     */
    virtual bool is_expression_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ProtectStatement
     * @return true if object of type ast::ProtectStatement
     */
    virtual bool is_protect_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::FromStatement
     * @return true if object of type ast::FromStatement
     */
    virtual bool is_from_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ForAllStatement
     * @return true if object of type ast::ForAllStatement
     */
    virtual bool is_for_all_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::WhileStatement
     * @return true if object of type ast::WhileStatement
     */
    virtual bool is_while_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::IfStatement
     * @return true if object of type ast::IfStatement
     */
    virtual bool is_if_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ElseIfStatement
     * @return true if object of type ast::ElseIfStatement
     */
    virtual bool is_else_if_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ElseStatement
     * @return true if object of type ast::ElseStatement
     */
    virtual bool is_else_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::PartialEquation
     * @return true if object of type ast::PartialEquation
     */
    virtual bool is_partial_equation() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Ast
     * @return true if object of type ast::Ast
     */
    virtual bool is_partial_boundary() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::PartialBoundary
     * @return true if object of type ast::PartialBoundary
     */
    virtual bool is_watch_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::MutexLock
     * @return true if object of type ast::MutexLock
     */
    virtual bool is_mutex_lock() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::MutexUnlock
     * @return true if object of type ast::MutexUnlock
     */
    virtual bool is_mutex_unlock() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Reset
     * @return true if object of type ast::Reset
     */
    virtual bool is_reset() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Sens
     * @return true if object of type ast::Sens
     */
    virtual bool is_sens() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Conserve
     * @return true if object of type ast::Conserve
     */
    virtual bool is_conserve() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Compartment
     * @return true if object of type ast::Compartment
     */
    virtual bool is_compartment() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::LonDifuse
     * @return true if object of type ast::LonDifuse
     */
    virtual bool is_lon_difuse() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ReactionStatement
     * @return true if object of type ast::ReactionStatement
     */
    virtual bool is_reaction_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::LagStatement
     * @return true if object of type ast::LagStatement
     */
    virtual bool is_lag_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::QueueStatement
     * @return true if object of type ast::QueueStatement
     */
    virtual bool is_queue_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ConstantStatement
     * @return true if object of type ast::ConstantStatement
     */
    virtual bool is_constant_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::TableStatement
     * @return true if object of type ast::TableStatement
     */
    virtual bool is_table_statement() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Suffix
     * @return true if object of type ast::Suffix
     */
    virtual bool is_suffix() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Useion
     * @return true if object of type ast::Useion
     */
    virtual bool is_useion() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Nonspecific
     * @return true if object of type ast::Nonspecific
     */
    virtual bool is_nonspecific() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ElctrodeCurrent
     * @return true if object of type ast::ElctrodeCurrent
     */
    virtual bool is_elctrode_current() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Section
     * @return true if object of type ast::Section
     */
    virtual bool is_section() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Range
     * @return true if object of type ast::Range
     */
    virtual bool is_range() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Global
     * @return true if object of type ast::Global
     */
    virtual bool is_global() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Pointer
     * @return true if object of type ast::Pointer
     */
    virtual bool is_pointer() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::BbcorePointer
     * @return true if object of type ast::BbcorePointer
     */
    virtual bool is_bbcore_pointer() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::External
     * @return true if object of type ast::External
     */
    virtual bool is_external() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::ThreadSafe
     * @return true if object of type ast::ThreadSafe
     */
    virtual bool is_thread_safe() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Verbatim
     * @return true if object of type ast::Verbatim
     */
    virtual bool is_verbatim() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::LineComment
     * @return true if object of type ast::LineComment
     */
    virtual bool is_line_comment() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::BlockComment
     * @return true if object of type ast::BlockComment
     */
    virtual bool is_block_comment() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::Program
     * @return true if object of type ast::Program
     */
    virtual bool is_program() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::NrnStateBlock
     * @return true if object of type ast::NrnStateBlock
     */
    virtual bool is_nrn_state_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::EigenNewtonSolverBlock
     * @return true if object of type ast::EigenNewtonSolverBlock
     */
    virtual bool is_eigen_newton_solver_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::EigenLinearSolverBlock
     * @return true if object of type ast::EigenLinearSolverBlock
     */
    virtual bool is_eigen_linear_solver_block() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::WrappedExpression
     * @return true if object of type ast::WrappedExpression
     */
    virtual bool is_wrapped_expression() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::DerivimplicitCallback
     * @return true if object of type ast::DerivimplicitCallback
     */
    virtual bool is_derivimplicit_callback() {
        return false;
    }

    /**
     *\brief Check if the ast node is an instance of ast::SolutionExpression
     * @return true if object of type ast::SolutionExpression
     */
    virtual bool is_solution_expression() {
        return false;
    }
};

/** @} */  // end of ast_class

}  // namespace ast
}  // namespace nmodl
