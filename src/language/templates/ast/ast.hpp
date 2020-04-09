/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
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

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ast/ast_decl.hpp"
#include "ast/ast_common.hpp"
#include "lexer/modtoken.hpp"
#include "utils/common_utils.hpp"
#include "visitors/visitor.hpp"


{# add virtual qualifier if node is an abstract class #}
{% macro virtual(node) -%}
    {% if node.is_abstract %} virtual {% endif %}
{% endmacro %}




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
   * @return name of the statement as a string
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
   * @param v Concrete visitor that will be used to recursively visit children
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
   * void IndexedName::visit_children(visitor::Visitor& v) {
   *    name->accept(v);
   *    length->accept(v);
   * }
   * \endcode
   */
  virtual void visit_children(visitor::Visitor& v) = 0;

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
   * @return name of the node as std::string
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
   * @return pointer to token if exist otherwise nullptr
   */
  virtual const ModToken* get_token() const;

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
  virtual symtab::SymbolTable* get_symbol_table();

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
   * @return true if object of type ast::Ast
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
   * \ref Check Ast::parent for more information
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
   * \ref Check Ast::parent for more information
   */
  virtual void set_parent(Ast* p);

  /**
   *\brief Set this object as parent for all the children
   *
   * This should be called in every object (with children) constructor
   * to set the parents.
   */
  virtual void set_parent_in_children();
};

/** @} */  // end of ast_class

    /**
     * @addtogroup ast_class
     * @ingroup ast
     * @{
     */

    {% for node in nodes %}
    /**
     * \brief {{ node.brief }}
     *
     * {{- node.get_description() -}}
     */
    class {{ node.class_name }} : public {{ node.base_class }} {
    {% if node.private_members() %}
      private:
        {% for member in node.private_members() %}
        {{ '/// ' + member[3] }}
        {% if member[2] is none %}
        {{ member[0] }} {{ member[1] }};
        {% else %}
        {{ member[0] }} {{ member[1] }} = {{ member[2] }};
        {% endif %}
        {% endfor %}

    {% endif %}
      public:
        {% for member in node.public_members() %}
        {{ '/// ' + member[3] }}
        {% if member[2] is none %}
        {{ member[0] }} {{ member[1] }};
        {% else %}
        {{ member[0] }} {{ member[1] }} = {{ member[2] }};
        {% endif %}
        {% endfor %}

        /// \name Ctor & dtor
        /// \{

        {% if node.children %}
        {{ node.ctor_declaration() }}
        {% if node.has_ptr_children() %}
            {{ node.ctor_shrptr_declaration() }}
        {% endif %}
        {{ node.class_name }}(const {{ node.class_name }}& obj);
        {% endif %}

        {% if node.requires_default_constructor %}
        {{ node.class_name}}() = default;
        {% endif %}

        virtual ~{{ node.class_name }}() {}

        /// \}

        /// \name Not implemented
        /// \{

        {% if node.is_base_block_node %}
        virtual const ArgumentVector& get_parameters() const {
            throw std::runtime_error("get_parameters not implemented");
        }
        {% endif %}

        {% if node.is_number_node %}
        {{ virtual(node) }}double to_double() {
            throw std::runtime_error("to_double not implemented");
        }
        {% endif %}

        /// \}

        /**
         *\brief Check if the ast node is an instance of ast::{{ node.class_name }}
         * @return true as object is of type ast::{{ node.class_name }}
         */
        bool is_{{ node.class_name | snake_case }} () const noexcept override {
            return true;
        }

        /**
         * \brief Return a copy of the current node
         *
         * Recursively make a new copy/clone of the current node including
         * all members and return a pointer to the node. This is used for
         * passes like nmodl::visitor::InlineVisitor where nodes are cloned in the
         * ast.
         *
         * @return pointer to the clone/copy of the current node
         */
        {{ virtual(node) }} {{ node.class_name }}* clone() const override {
            return new {{ node.class_name }}(*this);
        }


        /// \name Getters
        /// \{

        /**
         * \brief Return type (ast::AstNodeType) of ast node
         *
         * Every node in the ast has a type defined in ast::AstNodeType and this
         * function is used to retrieve the same.
         *
         * \return ast node type i.e. ast::AstNodeType::{{ node.ast_enum_name }}
         *
         * \sa Ast::get_node_type_name
         */
        {{ virtual(node) }} AstNodeType get_node_type() const noexcept override {
            return AstNodeType::{{ node.ast_enum_name }};
        }

        /**
         * \brief Return type (ast::AstNodeType) of ast node as std::string
         *
         * Every node in the ast has a type defined in ast::AstNodeType.
         * This type name can be returned as a std::string for printing
         * node to text/json form.
         *
         * @return name of the node type as a string i.e. "{{ node.class_name }}"
         *
         * \sa Ast::get_node_name
         */
        {{ virtual(node) }} std::string get_node_type_name() const noexcept override {
            return "{{ node.class_name }}";
        }

        {% if node.nmodl_name %}
        /**
        * \brief Return NMODL statement of ast node as std::string
        *
        * Every node is related to a special statement in the NMODL. This
        * statement can be returned as a std::string for printing to
        * text/json form.
        *
        * @return name of the statement as a string i.e. "{{ node.nmodl_name }}"
        *
        * \sa Ast::get_nmodl_name
        */
        {{ virtual(node) }} std::string get_nmodl_name() const noexcept override {
            return "{{ node.nmodl_name }}";
        }
        {% endif %}

        /**
         * \brief Get std::shared_ptr from `this` pointer of the current ast node
         */
        {{ virtual(node) }} std::shared_ptr<Ast> get_shared_ptr() override {
            return std::static_pointer_cast<{{ node.class_name }}>(shared_from_this());
        }

        /**
         * \brief Get std::shared_ptr from `this` pointer of the current ast node
         */
        {{ virtual(node) }} std::shared_ptr<const Ast> get_shared_ptr() const override {
            return std::static_pointer_cast<const {{ node.class_name }}>(shared_from_this());
        }

        {% if node.has_token %}
        /**
         * \brief Return associated token for the current ast node
         *
         * Not all ast nodes have token information. For example, nmodl::visitor::NeuronSolveVisitor
         * can insert new nodes in the ast as a solution of ODEs. In this case, we return
         * nullptr to store in the nmodl::symtab::SymbolTable.
         *
         * @return pointer to token if exist otherwise nullptr
         */
        const {{ virtual(node) }}ModToken* get_token() const override {
            return token.get();
        }
        {% endif %}

        {% if node.is_symtab_needed %}
        /**
         * \brief Return associated symbol table for the current ast node
         *
         * Only certain ast nodes (e.g. inherited from ast::Block) have associated
         * symbol table. These nodes have nmodl::symtab::SymbolTable as member
         * and it can be accessed using this method.
         *
         * @return pointer to the symbol table
         *
         * \sa nmodl::symtab::SymbolTable nmodl::visitor::SymtabVisitor
         */
        symtab::SymbolTable* get_symbol_table() override {
            return symtab;
        }

        {% endif %}

        {% if node.is_program_node %}
        /**
         * \brief Return global symbol table for the mod file
         */
        symtab::ModelSymbolTable* get_model_symbol_table() {
            return &model_symtab;
        }
        {% endif %}

        {# doxygen for these methods is handled by nodes.py #}
        {% for child in node.children %}
        {{ child.get_add_methods() }}

        {{ child.get_node_name_method() }}

        {{ child.get_getter_method(node.class_name) }}


        {% endfor %}

        /// \}


        /// \name Setters
        /// \{

        {% if node.is_name_node %}
        /**
         * \brief Set name for the current ast node
         *
         * Some ast nodes have a member marked designated as node name (e.g. nodes
         * derived from ast::Identifier). This method is used to set new name for those
         * nodes. This useful for passes like nmodl::visitor::RenameVisitor.
         *
         * \sa Ast::get_node_type_name Ast::get_node_name
         */
        {{ virtual(node) }}void set_name(const std::string& name) override {
            value->set(name);
        }
        {% endif %}

        {% if node.has_token %}
        /**
         * \brief Set token for the current ast node
         */
        void set_token(const ModToken& tok) { token = std::make_shared<ModToken>(tok); }
        {% endif %}

        {% if node.is_symtab_needed %}
        /**
         * \brief Set symbol table for the current ast node
         *
         * Top level, block scoped nodes store symbol table in the ast node.
         * nmodl::visitor::SymtabVisitor then used this method to setup symbol table
         * for every node in the ast.
         *
         * \sa nmodl::visitor::SymtabVisitor
         */
        void set_symbol_table(symtab::SymbolTable* newsymtab) override {
            symtab = newsymtab;
        }
        {% endif %}

        {# if node is base data type but not enum then add set method #}
        {% if node.is_data_type_node and not node.is_enum_node %}
            /**
             * \brief Set new value to the current ast node
             * \sa {{ node.class_name }}::eval
             */
            void set({{ node.get_data_type_name() }} _value) {
                value = _value;
            }
        {% endif %}

        {# doxygen for these methods is handled by nodes.py #}
        {% for child in node.children %}
        {{ child.get_setter_method_declaration(node.class_name) }}
        {% endfor %}

        /// \}


        /// \name Visitor
        /// \{

        /**
         * \brief visit children i.e. member variables of current node using provided visitor
         *
         * Different nodes in the AST have different members (i.e. children). This method
         * recursively visits children using provided visitor.
         *
         * @param v Concrete visitor that will be used to recursively visit children
         *
         * \sa Ast::visit_children for example.
         */
        {{ virtual(node) }} void visit_children (visitor::Visitor& v) override;

        /**
         * \brief accept (or visit) the current AST node using provided visitor
         *
         * Instead of visiting children of AST node, like Ast::visit_children,
         * accept allows to visit the current node itself using provided concrete
         * visitor.
         *
         * @param v Concrete visitor that will be used to recursively visit node
         *
         * \sa Ast::accept for example.
         */
        {{ virtual(node) }} void accept(visitor::Visitor& v) override {
            v.visit_{{ node.class_name | snake_case }}(this);
        }

        /// \}


        {% if node.is_base_class_number_node %}
        /**
         * \brief Negate the value of current ast node
         *
         * Parser parse `-x` in two parts : `x` and then `-`. Once second token
         * `-` is encountered, the corresponding value of ast node needs to be
         * multiplied by `-1` for ast::Number node types.
         */
        void negate() override {
            value = {{ node.negation }}value;
        }

        /**
         * \brief Return value of the current ast node as double
         */
        double to_double() override { return value; }
        {% endif %}

        {% if node.is_data_type_node %}
            {# if node is of enum type then return enum value #}
            {% if node.is_enum_node %}
                /**
                 * \brief Return enum value in string form
                 *
                 * Enum variables (e.g. ast::BinaryOp, ast::UnitStateType) have
                 * string representation when they are converted from AST back to
                 * NMODL. This method is used to return corresponding string representation.
                 */
                std::string eval() const { return {{
                    node.get_data_type_name() }}Names[value];
                }
            {# but if basic data type then eval return their value #}
            {% else %}
                /**
                 * \brief Return value of the ast node
                 *
                 * Base data type nodes like ast::Inetegr, ast::Double can be evaluated
                 * to their literal values. This method is used to access underlying
                 * literal value.
                 *
                 * \sa {{ node.class_name }}::set
                 */
                {{ node.get_data_type_name() }} eval() const {
                    return value;
                }
            {% endif %}
        {% endif %}

        {% if node.children %}
            /**
             * \brief Set parents in children
             *
             * Usually called in constructors
             */
            virtual void set_parent_in_children() override;
        {% endif %}
    };

    {% endfor %}

    /** @} */  // end of ast_class

}  // namespace ast
}  // namespace nmodl
