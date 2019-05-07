Visitors
########

One of the strengths of the NMODL python interface is access to inbuilt Visitors.
One can perform different queries and analysis on AST using different visitors. Let us start with examples of inbuilt visitors.

Parsing Model and constructing AST
==================================

Once the NMODL is setup properly we should be able to import nmodl module and create the channel:

    >>> import nmodl.dsl as nmodl
    >>> channel = """
    ... NEURON  {
    ...     SUFFIX CaDynamics
    ...     USEION ca READ ica WRITE cai
    ...     RANGE decay, gamma, minCai, depth
    ... }
    ... UNITS   {
    ...    (mV) = (millivolt)
    ...     (mA) = (milliamp)
    ...     FARADAY = (faraday) (coulombs)
    ...     (molar) = (1/liter)
    ...     (mM) = (millimolar)
    ...     (um)    = (micron)
    ... }
    ... PARAMETER   {
    ...     gamma = 0.05 : percent of free calcium (not buffered)
    ...     decay = 80 (ms) : rate of removal of calcium
    ...     depth = 0.1 (um) : depth of shell
    ...     minCai = 1e-4 (mM)
    ... }
    ... ASSIGNED    {ica (mA/cm2)}
    ... INITIAL {
    ...     cai = minCai
    ... }
    ... STATE   {
    ...     cai (mM)
    ... }
    ... BREAKPOINT  { SOLVE states METHOD cnexp }
    ... DERIVATIVE states   {
    ...     cai' = -(10000)*(ica*gamma/(2*FARADAY*depth)) - (cai - minCai)/decay
    ... }
    ... FUNCTION foo() {
    ...     LOCAL temp
    ...     foo = 1.0 + gamma
    ... }
    ... """

Now we can parse any valid NMODL constructs using parsing interface.
First, we have to create nmodl parser object using :class:`nmodl.NmodlDriver` and then we can use :func:`nmodl.NmodlDriver.parse_string` method:

    >>> driver = nmodl.NmodlDriver()
    >>> modast = driver.parse_string(channel)

The :func:`nmodl.NmodlDriver.parse_string` method will throw an exception with parsing error if the input is invalid.
Otherwise it return :class:`nmodl.ast.AST` object.

If we simply print AST object, we can see JSON representation:

    >>> print ('%.100s' % modast)  # only first 100 characters
    {"Program":[{"NeuronBlock":[{"StatementBlock":[{"Suffix":[{"Name":[{"String":[{"name":"SUFFIX"}]}]},


Querying AST objects with Visitors
===========


Lookup Visitor
-----------

As name suggest, lookup visitor allows to search different NMODL constructs in the AST. The `visitor` module provides access to inbuilt visitors. In order to use this visitor, we create an object of :class:`nmodl.visitor.AstLookupVisitor`:

    >>> from nmodl.dsl import visitor
    >>> from nmodl.dsl import ast
    >>> lookup_visitor = visitor.AstLookupVisitor()

Assuming we have created :class:`nmodl.ast` object (as shown here), we can search for any NMODL construct in the AST using :class:`nmodl.visitor.AstLookupVisitor`. For example, to find out `STATE` block in the mod file, we can simply do:

    >>> states = lookup_visitor.lookup(modast, ast.AstNodeType.STATE_BLOCK)
    >>> for state in states:
    ...     print (nmodl.to_nmodl(state))
    STATE {
        cai (mM)
    }


Symbol Table Visitor
----------

Symbol table visitor is used to find out all variables and their usage in mod file. To use this, just create a visitor object as:

    >>> from nmodl.dsl import symtab
    >>> symv = symtab.SymtabVisitor()

Once the visitor object is created, we can run visitor on AST object to populate symbol table. Symbol table provides print method that can be used to print whole symbol table:

    >>> symv.visit_program(modast)
    >>> table = modast.get_symbol_table()
    >>> table_s = str(table)

Now we can query for variables in the symbol table based on name of variable:

    >>> cai = table.lookup('cai')
    >>> print (cai)
    cai [Properties : prime_name assigned_definition write_ion state_var]


Custom AST Visitor
----------

If predefined visitors are limited, we can implement new visitor using :class:`nmodl.visitor.AstVisitor` interface. Let us say we want to implement a visitor that prints every floating point numbers in MOD file. Here is how it can be done:

    >>> from nmodl.dsl import ast, visitor
    >>> class DoubleVisitor(visitor.AstVisitor):
    ...     def visit_double(self, node):
    ...         print (node.eval())  # or, can use nmodl.to_nmodl(node)
    >>> d_visitor = DoubleVisitor()
    >>> modast.accept(d_visitor)
    0.05
    0.1
    0.0001
    10000.0
    2.0
    1.0

