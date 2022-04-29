/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#pragma once

/**
 * \dir
 * \brief Auto generated AST Implementations
 *
 * \file
 * \brief Auto generated AST classes declaration
 */

#include <string>
#include <vector>
#include <unordered_set>

#include "ast/ast_decl.hpp"
#include "ast/ast_common.hpp"
#include "lexer/modtoken.hpp"
#include "utils/common_utils.hpp"
#include "visitors/visitor.hpp"

namespace nmodl {
namespace ast {

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
  private:
  /**
   * \brief Generic pointer to the parent
   *
   * Children types can be known at compile time. Conversely, many parents
   * can have the same children type. Thus, this is just a pointer to
   * the base class. The pointer to the parent cannot have ownership
   * (circular ownership problem). weak_ptr you say? Yes, however weak_ptr
   * can be instantiated from shared_ptr (not this). Whys is this a problem?
   * In bison things must be passed around as raw pointers (because it uses
   * unions etc.) and there are cases where the shared_ptr to the parent
   * was not yet created while the child is added (throwing a bad_weak_ptr
   * exception).
   *
   * i.e. in bison the lines:
   *
   * ast::WatchStatement* a = new ast::WatchStatement();
   * yylhs.value.as< ast::WatchStatement* > ()->add_watch(a);
   *
   * would throw a bad_weak_ptr exception because when you call add_watch
   * the shared_ptr_from_this to "a" is not yet created.
   */
  Ast* parent = nullptr;

  public:

  /// \name Ctor & dtor
  /// \{

  Ast() = default;

  virtual ~Ast() = default;

  /// \}


  /// \name Pure Virtual Functions
  /// \{

  /**
   * \brief Return type (ast::AstNodeType) of AST node
   *
   * Every node in the ast has a type defined in ast::AstNodeType.
   * This type is can be used to check/compare node types.
   */
  virtual AstNodeType get_node_type() const = 0;

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
  virtual std::string get_node_type_name() const = 0;

  /**
   * \brief Return NMODL statement of ast node as std::string
   *
   * Every node is related to a special statement in the NMODL. This
   * statement can be returned as a std::string for printing to
   * text/json form.
   *
   * \return name of the statement as a string
   *
   * \sa Ast::get_nmodl_name
   */
  virtual std::string get_nmodl_name() const {
    throw std::runtime_error("get_nmodl_name not implemented");
  }

  /**
   * \brief Accept (or visit) the AST node using current visitor
   *
   * Instead of visiting children of AST node, like Ast::visit_children,
   * accept allows to visit the current node itself using the concrete
   * visitor provided.
   *
   * \param v Concrete visitor that will be used to recursively visit children
   *
   * \note Below is an example of `accept` method implementation which shows how
   *       visitor method corresponding to ast::IndexedName node is called allowing
   *       to visit the node itself in the visitor.
   *
   * \code{.cpp}
   *   void IndexedName::accept(visitor::Visitor& v) override {
   *       v.visit_indexed_name(this);
   *   }
   * \endcode
   *
   */
  virtual void accept(visitor::Visitor& v) = 0;

  /**
   * \brief Accept (or visit) the AST node using a given visitor
   * \param v constant visitor visiting the AST node
   * \ref accept(visitor::Visitor&);
   */
  virtual void accept(visitor::ConstVisitor& v) const = 0;

  /**
   * \brief Visit children i.e. member of AST node using provided visitor
   *
   * Different nodes in the AST have different members (i.e. children). This method
   * recursively visits children using provided concrete visitor.
   *
   * \param v Concrete visitor that will be used to recursively visit node
   *
   * \note Below is an example of `visit_children` method implementation which shows
   *       ast::IndexedName node children are visited instead of node itself.
   *
   * \code{.cpp}
   * void IndexedName::visit_children(visitor::Visitor& v) {
   *    name->accept(v);
   *    length->accept(v);
   * }
   * \endcode
   */
  virtual void visit_children(visitor::Visitor& v) = 0;

  /**
   * \copydoc visit_children(visitor::Visitor& v)
   */
  virtual void visit_children(visitor::ConstVisitor& v) const = 0;

  /**
   * \brief Create a copy of the current node
   *
   * Recursively make a new copy/clone of the current node including
   * all members and return a pointer to the node. This is used for
   * passes like nmodl::visitor::InlineVisitor where nodes are cloned in the
   * ast.
   *
   * \return pointer to the clone/copy of the current node
   */
  virtual Ast* clone() const;

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
   * \return name of the node as std::string
   *
   * \sa Ast::get_node_type_name
   */
  virtual std::string get_node_name() const;

  /**
   * \brief Return associated token for the AST node
   *
   * Not all ast nodes have token information. For example, nmodl::visitor::NeuronSolveVisitor
   * can insert new nodes in the ast as a solution of ODEs. In this case, we return
   * nullptr to store in the nmodl::symtab::SymbolTable.
   *
   * \return pointer to token if exist otherwise nullptr
   */
  virtual const ModToken* get_token() const;

  /**
   * \brief Return associated symbol table for the AST node
   *
   * Certain ast nodes (e.g. inherited from ast::Block) have associated
   * symbol table. These nodes have nmodl::symtab::SymbolTable as member
   * and it can be accessed using this method.
   *
   * \return pointer to the symbol table
   *
   * \sa nmodl::symtab::SymbolTable nmodl::visitor::SymtabVisitor
   */
  virtual symtab::SymbolTable* get_symbol_table() const;

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
   * \return pointer to the statement block if exist
   *
   * \sa ast::StatementBlock
   */
  virtual const std::shared_ptr<StatementBlock>& get_statement_block() const;

  /**
   * \brief Set symbol table for the AST node
   *
   * Top level, block scoped nodes store symbol table in the ast node.
   * nmodl::visitor::SymtabVisitor then used this method to setup symbol table
   * for every node in the ast.
   *
   * \sa nmodl::visitor::SymtabVisitor
   */
  virtual void set_symbol_table(symtab::SymbolTable* symtab);

  /**
   * \brief Set name for the AST node
   *
   * Some ast nodes have a member marked designated as node name (e.g. nodes
   * derived from ast::Identifier). This method is used to set new name for those
   * nodes. This useful for passes like nmodl::visitor::RenameVisitor.
   *
   * \sa Ast::get_node_type_name Ast::get_node_name
   */
  virtual void set_name(const std::string& name);

  /**
   * \brief Negate the value of AST node
   *
   * Parser parse `-x` in two parts : `x` and then `-`. Once second token
   * `-` is encountered, the corresponding value of ast node needs to be
   * multiplied by `-1` for ast::Number node types or apply `!` operator
   for the nodes of type ast::Boolean.
   */
  virtual void negate();
  /// \}

  /// get std::shared_ptr from `this` pointer of the AST node
  virtual std::shared_ptr<Ast> get_shared_ptr();

  /// get std::shared_ptr from `this` pointer of the AST node
  virtual std::shared_ptr<const Ast> get_shared_ptr() const;

  /**
   *\brief Check if the ast node is an instance of ast::Ast
   * \return true if object of type ast::Ast
   */
  virtual bool is_ast() const noexcept;

  {% for node in nodes %}
  /**
   * \brief Check if the ast node is an instance of ast::{{ node.class_name }}
   * \return true if object of type ast::OntologyStatement
   */
  virtual bool is_{{ node.class_name | snake_case }} () const noexcept;

  {% endfor %}

  /**
   *\brief Parent getter
   *
   * returning a raw pointer may create less problems that the
   * shared_from_this from the parent.
   *
   * Check \ref Ast::parent for more information
   */
  virtual Ast* get_parent() const;

  /**
   *\brief Parent setter
   *
   * Usually, the parent parent pointer cannot be set in the constructor
   * because children are generally build BEFORE the parent. Conversely,
   * we set children parents directly in the parent constructor using
   * set_parent_in_children()
   *
   *  Check \ref Ast::parent for more information
   */
  virtual void set_parent(Ast* p);
};

}  // namespace ast
}  // namespace nmodl
