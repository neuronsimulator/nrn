NMODL development tutorial
====

This is a tutorial for making changes to the NMODL codebase.


NMODL components
=======

To translate a mod file into a C++ source file, NMODL employs several stages that we will go through in varying degrees of detail.


Lexer and parser
---

.. note::

   Normally, you will not need to modify anything regarding the lexer and parser, since the grammar of NMODL itself is fixed.

The content of a given mod file is initially converted to a list of tokens using a lexer (flex). All of the code in charge of lexing a mod file is in ``src/nmodl/lexer``. The code is then converted to an abstract syntax tree (AST), which is composed of nodes, using a parser (bison). This is basically done using the following C++ code:

.. code:: cpp

        /// driver object creates lexer and parser, just call parser method
        NmodlDriver driver;

        /// parse mod file and construct AST
        const auto& ast = driver.parse_string(content);

.. note::

   The code above may look like it creates an immutable AST, but due to the use of smart pointers, the tree itself actually _can_ be modified! More on that later.

As a result, we will get an AST which can be traversed and modified using various visitors.


The visitors
---

NMODL employs the `visitor pattern <https://en.wikipedia.org/wiki/Visitor_pattern>__` to traverse the AST and perform operations on its nodes. The basic operations that can be performed on the AST are:

* insertion of a new node
* modification of an existing node
* deletion of a node

Before we get into the details of the visitors, it is helpful to understand which types of nodes are there, and which properties they have. All of the various types of nodes are actually generated from a single YAML file, ``src/nmodl/language/nmodl.yaml``. For instance, this is the definition of a ``FUNCTION`` block in the YAML file:

.. code:: yaml
    - FunctionBlock:
        nmodl: "FUNCTION "
        members:
          - name:
              brief: "Name of the function"
              type: Name
              node_name: true
          - parameters:
              brief: "Vector of the parameters"
              type: Argument
              vector: true
              prefix: {value: "(", force: true}
              suffix: {value: ")", force: true}
              separator: ", "
              getter: {override: true}
          - unit:
              brief: "Unit if specified"
              type: Unit
              optional: true
              prefix: {value: " "}
              suffix: {value: " ", force: true}
          - statement_block:
              brief: "Block with statements vector"
              type: StatementBlock
              getter: {override: true}

The above provides us with the following information:

* the NMODL statement corresponding to a ``FunctionBlock`` is ``FUNCTION ``, and a ``FunctionBlock`` node has a name (indicated by ``node_name: true``), meaning that we can't have a nameless function, and the name of the function has a node type ``Name``
* the function can take a vector of parameters (indicated by ``vector: true``), and each argument is a node of type ``Argument``. The list of arguments must be preceded by an opening paren (``(``) and succeeded by a closing paren (``)``), otherwise the lexer or parser will throw an error. Furthermore, the arguments must be separated by a semicolon (``,``)
* the function can optionally contain units
* the function must always contain a statement block (even if the statement block itself is empty)


Visiting a block
---

Assuming that we want to visit a function definition, and maybe do some manipulation of its children, how can we accomplish this? NMODL provides us with the ``nmodl::ast::AstVisitor`` class, which we can override to visit a particular node:

.. code:: cpp

    // in a header file
    #include "visitors/ast_visitor.hpp"
    #include "ast/function_block.hpp"
    namespace nmodl {
    namespace visitor {
    class MyFunctionVisitor : public ast::AstVisitor {
      public:
        void visit_function_block(ast::FunctionBlock&) override;
    };
    }
    }

Next, you need to write the actual implementation:

.. code:: cpp

    // in the implementation file
    #include "visitors/my_function_visitor.hpp"
    namespace nmodl {
    namespace visitor {
    void MyFunctionVisitor::visit_function_block(ast::FunctionBlock& node) {
        // implementation goes here
    }
    }
    }

In general, the naming convention is ``snake_case`` for the methods and header files, and ``TitleCase`` for the classes themselves.

.. note::

   When overriding a given method, the const-ness of the method signature corresponds to whether or not the visitor it inherits from is `AstVisitor` or `ConstAstVisitor`. In the above example, if we inherited from `ConstAstVisitor`, then the signature of `visit_function_block` would be `const ast::FunctionBlock&`.

.. warning::

   Don't forget to add your visitor implementation file to the ``visitor`` library in the CMake build code (currently in ``src/nmodl/visitors/CMakeLists.txt``)!


Visiting nested nodes
---

Sometimes you may want to visit multiple nested nodes; for instance, if your node can appear globally, i.e. anywhere, but has a special meaning inside of a ``FUNCTION`` block. NMODL provides the ``visit_children`` method for this, that performs the recursive descent into the children, which should be called as:

.. code:: cpp

    void MyFunctionVisitor::visit_function_block(ast::FunctionBlock& node) {
        // some code
        node.visit_children(*this);
        // other code that needs to run _after_ the children have been visited
    }

A common pattern used troughout the codebase is a toggle flag to notify the children node that it is inside of some parent node:

.. code:: cpp

    void MyFunctionVisitor::visit_function_block(ast::FunctionBlock& node) {
        // assuming ``in_function`` is a private member of ``MyFunctionVisitor``
        in_function = true;
        node.visit_children(*this);
        in_function = false;
    }


You can also override the ``accept`` method, which allows you to visit the current node itself, but not any of the children.

.. warning::

   It may seem tempting to add C++ code directly to the AST by inserting VERBATIM blocks; this mixes implementation details (C++ code) with the overall abstraction (the AST), so it should only be done sparsely.

Calling the visitor
---

To actually perform any transformations you implemented, you need to call the visitor:

.. code:: cpp

   MyFunctionVisitor().visit_program(*ast);

.. tip::

   The constructor of the visitor can take any arguments you specify; this is typically useful if you only want to perform changes to the AST depending on whether some CLI flag is present or not.

.. warning::

   If you are performing changes to the AST that also involve adding additional symbols (for instance, a LOCAL variable), in order to keep the symbol table synchronized with the contents of the AST, you should call ``SymtabVisitor().visit_program(*ast)`` afterwards.


Adding a new node type
---

On certain occasions, it may be necessary to add new nodes to NMODL; this is usually due to some internal requirements for codegen, and the right place to add it is in ``src/nmodl/language/codegen.yaml``. For example, the following node type aws added for keeping track of the information required for CVODE codegen:

.. code:: yaml

    - CvodeBlock:
        nmodl: "CVODE_BLOCK "
        members:
          - name:
              brief: "Name of the block"
              type: Name
              node_name: true
              suffix: {value: " "}
          - n_odes:
              brief: "number of ODEs to solve"
              type: Integer
              prefix: {value: "["}
              suffix: {value: "]"}
          - non_stiff_block:
              brief: "Block with statements of the form Dvar = f(var), used for updating non-stiff systems"
              type: StatementBlock
          - stiff_block:
              brief: "Block with statements of the form Dvar = Dvar / (1 - dt * J(f)), used for updating stiff systems"
              type: StatementBlock
        brief: "Represents a block used for variable timestep integration (CVODE) of DERIVATIVE blocks"

If a mod file can use CVODE time integration method, the above will be used to generate an intermediate file which contains something like:

.. code::

    CVODE_BLOCK myblock [3] {
        : statements for non-stiff block
    }
    {
        : statements for stiff block
    }


Adding code generation
---

After performing any manipulations to the AST, it is time to generate actual code that a given compiler can use. NMODL has 3 code backends:

* the NEURON code backend for generating C++ code compatible with NEURON (``codegen_neuron_cpp_visitor.cpp``)
* the coreNEURON code backend for generating C++ code compatible with coreNEURON using CPUs (``codegen_coreneuron_cpp_visitor.cpp``)
* the coreNEURON code backend for generating C++ code compatible with coreNEURON using GPUs (``codegen_acc_visitor.cpp``)

We will not go into the details of the last two, but will instead focus on the NEURON code backend.

This backend 

.. warning::

   The codegen 
