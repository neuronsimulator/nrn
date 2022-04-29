# ***********************************************************************
# Copyright (C) 2018-2022 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

"""Define node and data types used in parser and ast generator

While generating ast and visitors we need to lookup for base data types,
nmodl blocks defining variables, nmodl constructs that define variables
etc. These all data types are defined in this file.

TODO: Other way is to add extra attributes to YAML language definitions.
YAML will  become more verbose but advantage would be that the YAML will be
self sufficient, single definition file instead of hard-coded names here.
We should properties like is_enum, data_type, is_symbol, is_global etc.
"""

# yapf: disable
INTEGRAL_TYPES = {"short",
                  "int",
                  "float",
                  "double",
                  "BinaryOp",
                  "UnaryOp",
                  "ReactionOp",
                  "FirstLastType",
                  "QueueType",
                  "BAType",
                  "UnitStateType",
                  }

BASE_TYPES = {"std::string" } | INTEGRAL_TYPES

# base types which are enums
ENUM_BASE_TYPES = {"BinaryOp",
                   "UnaryOp",
                   "ReactionOp",
                   "FirstLastType",
                   "QueueType",
                   "BAType",
                   "UnitStateType",
                   }

# data types and then their return types
DATA_TYPES = {"Boolean": "bool",
              "Integer": "int",
              "Float": "std::string",
              "Double": "std::string",
              "String": "std::string",
              "BinaryOperator": "BinaryOp",
              "UnaryOperator": "UnaryOp",
              "ReactionOperator": "ReactionOp",
              "UnitState": "UnitStateType",
              "BABlockType": "BAType",
              "QueueExpressionType": "QueueType",
              "FirstLastTypeIndex": "FirstLastType",
              }

# nodes which will go into symbol table
SYMBOL_VAR_TYPES = {"LocalVar",
                    "ParamAssign",
                    "Argument",
                    "AssignedDefinition",
                    "UnitDef",
                    "FactorDef",
                    "RangeVar",
                    "Useion",
                    "ReadIonVar",
                    "WriteIonVar",
                    "NonspecificCurVar",
                    "ElectrodeCurVar",
                    "SectionVar",
                    "GlobalVar",
                    "PointerVar",
                    "BbcorePointerVar",
                    "ExternVar",
                    "PrimeName",
                    "ConstantVar",
                    "Define",
                    }

# block nodes which will go into symbol table
# these blocks doesn't define global variables but they
# use variables from other global variables
SYMBOL_BLOCK_TYPES = {"FunctionBlock",
                      "ProcedureBlock",
                      "DerivativeBlock",
                      "LinearBlock",
                      "NonLinearBlock",
                      "DiscreteBlock",
                      "PartialBlock",
                      "KineticBlock",
                      "FunctionTableBlock"
                      }

# nodes which need extra handling to augument symbol table
SYMBOL_TABLE_HELPER_NODES = {"TableStatement",
                             }

# blocks defining global variables
GLOBAL_BLOCKS = {"NeuronBlock",
                 "ParamBlock",
                 "UnitBlock",
                 "StepBlock",
                 "IndependentBlock",
                 "AssignedBlock",
                 "StateBlock",
                 "ConstantBlock",
                 }

# when translating back to nmodl, we need print each statement
# to new line. Those nodes are are used from this list.
STATEMENT_TYPES = {"Statement",
                   "IndependentDefinition",
                   "AssignedDefinition",
                   "ParamAssign",
                   "ConstantStatement",
                   "Stepped",
                   }

# data types which have token as an argument to the constructor
LEXER_DATA_TYPES = {"Name",
                    "PrimeName",
                    "Integer",
                    "Double",
                    "String",
                    "FactorDef",
                    }

# while printing symbol table we needed setToken() method for StatementBlock and
# hence need to add this
ADDITIONAL_TOKEN_BLOCKS = {"StatementBlock",
                           }

# for printing NMODL, we need to know which nodes are block types.
# TODO: NEURON block is removed because it has internal statement block
# and we don't want to print extra brace block for NMODL
# We are removing NeuronBlock because it has statement block which
# prints braces already.
BLOCK_TYPES = GLOBAL_BLOCKS | ADDITIONAL_TOKEN_BLOCKS
BLOCK_TYPES.remove("NeuronBlock")

# Note that these are nodes which are not of type pointer in AST.
# This also means that they can't be optional because they will appear
# as value type in AST.
PTR_EXCLUDE_TYPES = {"BinaryOperator",
                     "UnaryOperator",
                     "ReactionOperator",
                     }

# these node names are explicitly added because they are used in ast/visitor
# printer classes. In order to avoid hardcoding in the printer functions, they
# are defined here.
BASE_BLOCK = "Block"
BINARY_EXPRESSION_NODE = "BinaryExpression"
BOOLEAN_NODE = "Boolean"
DOUBLE_NODE = "Double"
FLOAT_NODE = "Float"
INCLUDE_NODE = "Include"
INTEGER_NODE = "Integer"
NAME_NODE = "Name"
NUMBER_NODE = "Number"
PRIME_NAME_NODE = "PrimeName"
PROGRAM_BLOCK = "Program"
STATEMENT_BLOCK_NODE = "StatementBlock"
STRING_NODE = "String"
UNIT_BLOCK = "UnitBlock"

# name of variable in prime node which represent order of derivative
ORDER_VAR_NAME = "order"
BINARY_OPERATOR_NAME = "op"
# yapf: enable
