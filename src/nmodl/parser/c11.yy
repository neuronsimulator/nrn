/**********************************************************************************
 *
 * @brief Bison grammar and parser implementation for C (11)
 *
 * NMODL has verbatim constructs that allow to specify C code sections within
 * nmodl implementation.
 *
 * CREDIT : This is based on bison specification available at
 *          http://www.quut.com/c/ANSI-C-grammar-y-2011.html
 *****************************************************************************/

%code requires
{
    #include <string>
    #include <sstream>

    #include "parser/c11_driver.hpp"
}

/** use C++ parser interface of bison */
%skeleton "lalr1.cc"

/** require modern bison version */
%require  "3.0.2"

/** print verbose error messages instead of just message 'syntax error' */
%define parse.error verbose

/** enable tracing parser for debugging */
%define parse.trace

/** add extra arguments to yyparse() and yylex() methods */
%parse-param {class CLexer& scanner}
%parse-param {class CDriver& driver}
%lex-param   {nmodl::CScanner &scanner}
%lex-param   {nmodl::CDriver &driver}

/** use variant based implementation of semantic values */
%define api.value.type variant

/** assert correct cleanup of semantic value objects */
%define parse.assert

/** handle symbol to be handled as a whole (type, value, and possibly location) in scanner */
%define api.token.constructor

/** specify the namespace for the parser class */
%define api.namespace {nmodl::parser}

/** set the parser's class identifier */
%define parser_class_name {CParser}

/** keep track of the current position within the input */
%locations

%token    <std::string>    IDENTIFIER
%token    <std::string>    I_CONSTANT
%token    <std::string>    F_CONSTANT
%token    <std::string>    STRING_LITERAL
%token    <std::string>    FUNC_NAME
%token    <std::string>    SIZEOF
%token    <std::string>    PTR_OP
%token    <std::string>    INC_OP
%token    <std::string>    DEC_OP
%token    <std::string>    LEFT_OP
%token    <std::string>    RIGHT_OP
%token    <std::string>    LE_OP
%token    <std::string>    GE_OP
%token    <std::string>    EQ_OP
%token    <std::string>    NE_OP
%token    <std::string>    AND_OP
%token    <std::string>    OR_OP
%token    <std::string>    MUL_ASSIGN
%token    <std::string>    DIV_ASSIGN
%token    <std::string>    MOD_ASSIGN
%token    <std::string>    ADD_ASSIGN
%token    <std::string>    SUB_ASSIGN
%token    <std::string>    LEFT_ASSIGN
%token    <std::string>    RIGHT_ASSIGN
%token    <std::string>    AND_ASSIGN
%token    <std::string>    XOR_ASSIGN
%token    <std::string>    OR_ASSIGN
%token    <std::string>    TYPEDEF_NAME
%token    <std::string>    ENUMERATION_CONSTANT

%token    <std::string>    TYPEDEF
%token    <std::string>    EXTERN
%token    <std::string>    STATIC
%token    <std::string>    AUTO
%token    <std::string>    REGISTER
%token    <std::string>    INLINE
%token    <std::string>    CONST
%token    <std::string>    RESTRICT
%token    <std::string>    VOLATILE
%token    <std::string>    BOOL
%token    <std::string>    CHAR
%token    <std::string>    SHORT
%token    <std::string>    INT
%token    <std::string>    LONG
%token    <std::string>    SIGNED
%token    <std::string>    UNSIGNED
%token    <std::string>    FLOAT
%token    <std::string>    DOUBLE
%token    <std::string>    VOID
%token    <std::string>    COMPLEX
%token    <std::string>    IMAGINARY
%token    <std::string>    STRUCT
%token    <std::string>    UNION
%token    <std::string>    ENUM
%token    <std::string>    ELLIPSIS

%token    <std::string>    CASE
%token    <std::string>    DEFAULT
%token    <std::string>    IF
%token    <std::string>    THEN
%token    <std::string>    ELSE
%token    <std::string>    SWITCH
%token    <std::string>    WHILE
%token    <std::string>    DO
%token    <std::string>    FOR
%token    <std::string>    GOTO
%token    <std::string>    CONTINUE
%token    <std::string>    BREAK
%token    <std::string>    RETURN

%token    <std::string>    ALIGNAS
%token    <std::string>    ALIGNOF
%token    <std::string>    ATOMIC
%token    <std::string>    GENERIC
%token    <std::string>    NORETURN
%token    <std::string>    STATIC_ASSERT
%token    <std::string>    THREAD_LOCAL

%token    <std::string>    SEMICOLON           ";"
%token    <std::string>    OPEN_BRACE          "{"
%token    <std::string>    CLOSE_BRACE         "}"
%token    <std::string>    COMMA               ","
%token    <std::string>    COLON               ":"
%token    <std::string>    ASSIGN              "="
%token    <std::string>    OPEN_PARENTHESIS    "("
%token    <std::string>    CLOSE_PARENTHESIS   ")"
%token    <std::string>    OPEN_BRACKET        "["
%token    <std::string>    CLOSE_BRACKET       "]"
%token    <std::string>    PERIOD              "."
%token    <std::string>    AMPERSTAND          "&"
%token    <std::string>    NEGATION            "!"
%token    <std::string>    TILDE               "~"
%token    <std::string>    ADD                 "+"
%token    <std::string>    MULTIPLY            "*"
%token    <std::string>    MINUS               "-"
%token    <std::string>    DIVIDE              "/"
%token    <std::string>    PERCENT             "%"
%token    <std::string>    LT                  "<"
%token    <std::string>    GT                  ">"
%token    <std::string>    CARET               "^"
%token    <std::string>    OR                  "|"
%token    <std::string>    QUESTION            "?"
%token    END              0                   "End of file"

// Priorities/associativities.
%nonassoc ATOMIC
%right THEN ELSE // Same precedence, but "shift" wins.
%left OPEN_PARENTHESIS

%{
    #include "lexer/c11_lexer.hpp"
    #include "parser/c11_driver.hpp"

    using nmodl::parser::CParser;
    using nmodl::parser::CLexer;
    using nmodl::parser::CDriver;

    /// yylex takes scanner as well as driver reference
    /// \todo Check if driver argument is required
    static CParser::symbol_type yylex(CLexer &scanner, CDriver &/*driver*/) {
        return scanner.next_token();
    }
%}


%start translation_unit

%%

primary_expression
    : IDENTIFIER
    | constant
    | string
    | "(" expression ")"
    | generic_selection
    ;

constant
    : I_CONSTANT                /* includes character_constant */
    | F_CONSTANT
    | ENUMERATION_CONSTANT      /* after it has been defined as such */
    ;

enumeration_constant            /* before it has been defined as such */
    : IDENTIFIER
    ;

string
    : STRING_LITERAL
    | FUNC_NAME
    ;

generic_selection
    : GENERIC "(" assignment_expression "," generic_assoc_list ")"
    ;

generic_assoc_list
    : generic_association
    | generic_assoc_list "," generic_association
    ;

generic_association
    : type_name ":" assignment_expression
    | DEFAULT ":" assignment_expression
    ;

postfix_expression
    : primary_expression
    | postfix_expression "[" expression "]"
    | postfix_expression "(" ")"
    | postfix_expression "(" argument_expression_list ")"
    | postfix_expression "." IDENTIFIER
    | postfix_expression PTR_OP IDENTIFIER
    | postfix_expression INC_OP
    | postfix_expression DEC_OP
    | "(" type_name ")" "{" initializer_list "}"
    | "(" type_name ")" "{" initializer_list "," "}"
    ;

argument_expression_list
    : assignment_expression
    | argument_expression_list "," assignment_expression
    ;

unary_expression
    : postfix_expression
    | INC_OP unary_expression
    | DEC_OP unary_expression
    | unary_operator cast_expression
    | SIZEOF unary_expression
    | SIZEOF "(" type_name ")"
    | ALIGNOF "(" type_name ")"
    ;

unary_operator
    : "&"
    | "*"
    | "+"
    | "-"
    | "~"
    | "!"
    ;

cast_expression
    : unary_expression
    | "(" type_name ")" cast_expression
    ;

multiplicative_expression
    : cast_expression
    | multiplicative_expression "*" cast_expression
    | multiplicative_expression "/" cast_expression
    | multiplicative_expression "%" cast_expression
    ;

additive_expression
    : multiplicative_expression
    | additive_expression "+" multiplicative_expression
    | additive_expression "-" multiplicative_expression
    ;

shift_expression
    : additive_expression
    | shift_expression LEFT_OP additive_expression
    | shift_expression RIGHT_OP additive_expression
    ;

relational_expression
    : shift_expression
    | relational_expression "<" shift_expression
    | relational_expression ">" shift_expression
    | relational_expression LE_OP shift_expression
    | relational_expression GE_OP shift_expression
    ;

equality_expression
    : relational_expression
    | equality_expression EQ_OP relational_expression
    | equality_expression NE_OP relational_expression
    ;

and_expression
    : equality_expression
    | and_expression "&" equality_expression
    ;

exclusive_or_expression
    : and_expression
    | exclusive_or_expression "^" and_expression
    ;

inclusive_or_expression
    : exclusive_or_expression
    | inclusive_or_expression "|" exclusive_or_expression
    ;

logical_and_expression
    : inclusive_or_expression
    | logical_and_expression AND_OP inclusive_or_expression
    ;

logical_or_expression
    : logical_and_expression
    | logical_or_expression OR_OP logical_and_expression
    ;

conditional_expression
    : logical_or_expression
    | logical_or_expression "?" expression ":" conditional_expression
    ;

assignment_expression
    : conditional_expression
    | unary_expression assignment_operator assignment_expression
    ;

assignment_operator
    : "="
    | MUL_ASSIGN
    | DIV_ASSIGN
    | MOD_ASSIGN
    | ADD_ASSIGN
    | SUB_ASSIGN
    | LEFT_ASSIGN
    | RIGHT_ASSIGN
    | AND_ASSIGN
    | XOR_ASSIGN
    | OR_ASSIGN
    ;

expression
    : assignment_expression
    | expression "," assignment_expression
    ;

constant_expression
    : conditional_expression    /* with constraints */
    ;

declaration
    : declaration_specifiers ";"
    | declaration_specifiers init_declarator_list ";"
    | static_assert_declaration
    ;

declaration_specifiers
    : storage_class_specifier declaration_specifiers
    | storage_class_specifier
    | type_specifier declaration_specifiers
    | type_specifier
    | type_qualifier declaration_specifiers
    | type_qualifier
    | function_specifier declaration_specifiers
    | function_specifier
    | alignment_specifier declaration_specifiers
    | alignment_specifier
    ;

init_declarator_list
    : init_declarator
    | init_declarator_list "," init_declarator
    ;

init_declarator
    : declarator "=" initializer
    | declarator
    ;

storage_class_specifier
    : TYPEDEF    /* identifiers must be flagged as TYPEDEF_NAME */
    | EXTERN
    | STATIC
    | THREAD_LOCAL
    | AUTO
    | REGISTER
    ;

type_specifier
    : VOID
    | CHAR
    | SHORT
    | INT
    | LONG
    | FLOAT
    | DOUBLE
    | SIGNED
    | UNSIGNED
    | BOOL
    | COMPLEX
    | IMAGINARY          /* non-mandated extension */
    | atomic_type_specifier
    | struct_or_union_specifier
    | enum_specifier
    | TYPEDEF_NAME        /* after it has been defined as such */
    ;

struct_or_union_specifier
    : struct_or_union "{" struct_declaration_list "}"
    | struct_or_union IDENTIFIER "{" struct_declaration_list "}"
    | struct_or_union IDENTIFIER
    ;

struct_or_union
    : STRUCT
    | UNION
    ;

struct_declaration_list
    : struct_declaration
    | struct_declaration_list struct_declaration
    ;

struct_declaration
    : specifier_qualifier_list ";"    /* for anonymous struct/union */
    | specifier_qualifier_list struct_declarator_list ";"
    | static_assert_declaration
    ;

specifier_qualifier_list
    : type_specifier specifier_qualifier_list
    | type_specifier
    | type_qualifier specifier_qualifier_list
    | type_qualifier
    ;

struct_declarator_list
    : struct_declarator
    | struct_declarator_list "," struct_declarator
    ;

struct_declarator
    : ":" constant_expression
    | declarator ":" constant_expression
    | declarator
    ;

enum_specifier
    : ENUM "{" enumerator_list "}"
    | ENUM "{" enumerator_list "," "}"
    | ENUM IDENTIFIER "{" enumerator_list "}"
    | ENUM IDENTIFIER "{" enumerator_list "," "}"
    | ENUM IDENTIFIER
    ;

enumerator_list
    : enumerator
    | enumerator_list "," enumerator
    ;

enumerator    /* identifiers must be flagged as ENUMERATION_CONSTANT */
    : enumeration_constant "=" constant_expression
    | enumeration_constant
    ;

atomic_type_specifier
    : ATOMIC "(" type_name ")"
    ;

type_qualifier
    : CONST
    | RESTRICT
    | VOLATILE
    | ATOMIC
    ;

function_specifier
    : INLINE
    | NORETURN
    ;

alignment_specifier
    : ALIGNAS "(" type_name ")"
    | ALIGNAS "(" constant_expression ")"
    ;

declarator
    : pointer direct_declarator
    | direct_declarator
    ;

direct_declarator
    : IDENTIFIER
    | "(" declarator ")"
    | direct_declarator "[" "]"
    | direct_declarator "[" "*" "]"
    | direct_declarator "[" STATIC type_qualifier_list assignment_expression "]"
    | direct_declarator "[" STATIC assignment_expression "]"
    | direct_declarator "[" type_qualifier_list "*" "]"
    | direct_declarator "[" type_qualifier_list STATIC assignment_expression "]"
    | direct_declarator "[" type_qualifier_list assignment_expression "]"
    | direct_declarator "[" type_qualifier_list "]"
    | direct_declarator "[" assignment_expression "]"
    | direct_declarator "(" parameter_type_list ")"
    | direct_declarator "(" ")"
    | direct_declarator "(" identifier_list ")"
    ;

pointer
    : "*" type_qualifier_list pointer
    | "*" type_qualifier_list
    | "*" pointer
    | "*"
    ;

type_qualifier_list
    : type_qualifier
    | type_qualifier_list type_qualifier
    ;


parameter_type_list
    : parameter_list "," ELLIPSIS
    | parameter_list
    ;

parameter_list
    : parameter_declaration
    | parameter_list "," parameter_declaration
    ;

parameter_declaration
    : declaration_specifiers declarator
    | declaration_specifiers abstract_declarator
    | declaration_specifiers
    ;

identifier_list
    : IDENTIFIER
    | identifier_list "," IDENTIFIER
    ;

type_name
    : specifier_qualifier_list abstract_declarator
    | specifier_qualifier_list
    ;

abstract_declarator
    : pointer direct_abstract_declarator
    | pointer
    | direct_abstract_declarator
    ;

direct_abstract_declarator
    : "(" abstract_declarator ")"
    | "[" "]"
    | "[" "*" "]"
    | "[" STATIC type_qualifier_list assignment_expression "]"
    | "[" STATIC assignment_expression "]"
    | "[" type_qualifier_list STATIC assignment_expression "]"
    | "[" type_qualifier_list assignment_expression "]"
    | "[" type_qualifier_list "]"
    | "[" assignment_expression "]"
    | direct_abstract_declarator "[" "]"
    | direct_abstract_declarator "[" "*" "]"
    | direct_abstract_declarator "[" STATIC type_qualifier_list assignment_expression "]"
    | direct_abstract_declarator "[" STATIC assignment_expression "]"
    | direct_abstract_declarator "[" type_qualifier_list assignment_expression "]"
    | direct_abstract_declarator "[" type_qualifier_list STATIC assignment_expression "]"
    | direct_abstract_declarator "[" type_qualifier_list "]"
    | direct_abstract_declarator "[" assignment_expression "]"
    | "(" ")"
    | "(" parameter_type_list ")"
    | direct_abstract_declarator "(" ")"
    | direct_abstract_declarator "(" parameter_type_list ")"
    ;

initializer
    : "{" initializer_list "}"
    | "{" initializer_list "," "}"
    | assignment_expression
    ;

initializer_list
    : designation initializer
    | initializer
    | initializer_list "," designation initializer
    | initializer_list "," initializer
    ;

designation
    : designator_list "="
    ;

designator_list
    : designator
    | designator_list designator
    ;

designator
    : "[" constant_expression "]"
    | "." IDENTIFIER
    ;

static_assert_declaration
    : STATIC_ASSERT "(" constant_expression "," STRING_LITERAL ")" ";"
    ;

statement
    : labeled_statement
    | compound_statement
    | expression_statement
    | selection_statement
    | iteration_statement
    | jump_statement
    ;

labeled_statement
    : IDENTIFIER ":" statement
    | CASE constant_expression ":" statement
    | DEFAULT ":" statement
    ;

compound_statement
    : "{" "}"
    | "{"  block_item_list "}"
    ;

block_item_list
    : block_item
    | block_item_list block_item
    ;

block_item
    : declaration
    | statement
    ;

expression_statement
    : ";"
    | expression ";"
    ;

selection_statement
    : IF "(" expression ")" statement ELSE statement
    | IF "(" expression ")" statement %prec THEN
    | SWITCH "(" expression ")" statement
    ;

iteration_statement
    : WHILE "(" expression ")" statement
    | DO statement WHILE "(" expression ")" ";"
    | FOR "(" expression_statement expression_statement ")" statement
    | FOR "(" expression_statement expression_statement expression ")" statement
    | FOR "(" declaration expression_statement ")" statement
    | FOR "(" declaration expression_statement expression ")" statement
    ;

jump_statement
    : GOTO IDENTIFIER ";"
    | CONTINUE ";"
    | BREAK ";"
    | RETURN ";"
    | RETURN expression ";"
    ;

translation_unit
    : external_declaration
    | translation_unit external_declaration
    ;

external_declaration
    : function_definition
    | declaration
    ;

function_definition
    : declaration_specifiers declarator declaration_list compound_statement
    | declaration_specifiers declarator compound_statement
    ;

declaration_list
    : declaration
    | declaration_list declaration
    ;

%%

/** Bison expects error handler for parser */

void CParser::error(const location &loc , const std::string &msg) {
    std::stringstream ss;
    ss << "C Parser Error : " << msg << " [Location : " << loc << "]";
    throw std::runtime_error(ss.str());
}
