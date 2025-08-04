C API
=====

.. contents::
    :local:
    :depth: 2

Initialization
--------------

.. c:function:: int nrn_init(int argc, const char** argv)

    Initialize the NEURON environment. This function must be called before using any other NEURON API functions.
    The initialization sets up the NEURON interpreter and internal data structures.

    :param argc: Argument count (should be at least 1, the name of the program)
    :param argv: Argument vector (length should be one longer than argc and end with ``nullptr``).
    :returns: 0 on success, non-zero on failure.

    **Usage Pattern:**

    This function is called at the beginning of a NEURON session. 
    The arguments are passed to the NEURON simulator as if it was launched with that argv.
    The first argument is typically the program name (e.g., "NEURON").
    Some error messages may include the program name.

    **Example:**
    
    .. code-block:: c++

        static std::array<const char*, 4> argv = {"NEURON", "-nogui", "-nopython", nullptr};

        if (nrn_init(3, argv.data()) != 0) {
            // handle initialization error
        }

.. c:function:: void nrn_stdout_redirect(int (*myprint)(int, char*))

    Redirect NEURON's stdout to a custom print function. This allows capturing NEURON's output
    and redirecting it to alternative destinations (e.g., Jupyter notebook or MATLAB command window).

    :param myprint: Function pointer for custom printing. The function should accept two parameters:
                    stream (1 for stdout, other values for stderr) and the message string.


    **Example:**
    
    .. code-block:: c
    
        int my_print_func(int stream, char* msg) {
            if (stream == 1) {
                printf("[STDOUT] %s", msg);
            } else {
                printf("[STDERR] %s", msg);
            }
            return 0;
        }
        
        nrn_stdout_redirect(my_print_func);


Sections
--------

.. c:function:: Section* nrn_section_new(const char* name)

    Create a new section with the given name. Sections are the fundamental building blocks
    of NEURON morphologies, representing cable segments of neurons. HOC functions can
    see the Section name but they cannot be referred to directly in HOC like a Section created
    in HOC (i.e., they do not occupy the HOC namespace). The returned Section pointer is used
    in subsequent operations to reference the section.

    :param name: Name of the new section (must be unique within the model).
    :returns: Pointer to the newly created Section object.

    **C Usage:**
    
    .. code-block:: c
    
        Section* soma = nrn_section_new("soma");
        Section* dendrite = nrn_section_new("dendrite");

    **Python Equivalent:**
    
    .. code-block:: python
    
        from neuron import n
        soma = n.Section('soma')
        dendrite = n.Section('dendrite')

.. c:function:: void nrn_section_connect(Section* child_sec, double child_x, Section* parent_sec, double parent_x)

    Connect a child section to a parent section at specified locations. This defines
    the topological structure of the neuron. Typically, dendrites and axons
    are connected to the soma, and further branches are connected to primary dendrites.
    That is, the 0 end of the child is usually connected to the 1 end of the parent.

    :param child_sec: Pointer to the child section to be connected.
    :param child_x: Connection point on child section (must be either 0 or 1, but typically 0).
    :param parent_sec: Pointer to the parent section.
    :param parent_x: Connection point on parent section (any value between 0 and 1, but typically 1).


    **C Usage:**
    
    .. code-block:: c
    
        // Connect beginning of dendrite to end of soma
        nrn_section_connect(dendrite, 0.0, soma, 1.0);

    **Python Equivalent:**
    
    .. code-block:: python
    
        # Connect beginning of dendrite to end of soma
        dendrite.connect(soma(1))

.. c:function:: void nrn_section_length_set(Section* sec, double length)

    Set the length of a section in microns.

    :param sec: Pointer to the section.
    :param length: Length in microns.

    **C Usage:**
    
    .. code-block:: c
    
        nrn_section_length_set(soma, 20.0);    // Set soma length to 20 μm
        nrn_section_length_set(dendrite, 100.0); // Set dendrite length to 100 μm

    **Python Equivalent:**
    
    .. code-block:: python
    
        soma.L = 20    # Set soma length to 20 μm
        dendrite.L = 100   # Set dendrite length to 100 μm

.. c:function:: double nrn_section_length_get(Section* sec)

    Get the length of a section in microns.

    :param sec: Pointer to the section.
    :returns: Length of the section in microns.

    **C Usage:**
    
    .. code-block:: c
    
        double length = nrn_section_length_get(soma);

    **Python Equivalent:**
    
    .. code-block:: python
    
        length = soma.L  # Get soma length

.. c:function:: double nrn_section_Ra_get(Section* sec)

    Get the axial resistance (Ra) of a section in ohm⋅cm.
    Ra represents the resistance of the cytoplasm along the length of the section.
    Lower values indicate better electrical connectivity.

    :param sec: Pointer to the section.
    :returns: Axial resistance in ohm⋅cm.

    **C Usage:**
    
    .. code-block:: c

        double Ra = nrn_section_Ra_get(soma);
    
    .. note:: 

        Ra and rallbranch are Section level properties; they are not range variables
        and do not vary within a Section.

.. c:function:: void nrn_section_Ra_set(Section* sec, double val)

    Set the axial resistance (Ra) of a section in ohm⋅cm.

    :param sec: Pointer to the section.
    :param val: Axial resistance value in ohm⋅cm.

    **C Usage:**
    
    .. code-block:: c
    
        nrn_section_Ra_set(soma, 100.0);  // Set axial resistance to 100 ohm⋅cm

    **Python Equivalent:**
    
    .. code-block:: python
    
        soma.Ra = 100  # Set axial resistance to 100 ohm⋅cm

.. c:function:: double nrn_section_rallbranch_get(const Section* sec)

    Get the Rallbranch value of a section. This is used in models with branching corrections.

    :param sec: Pointer to the section.
    :returns: Rallbranch value.

    .. note:: 

        Ra and rallbranch are Section level properties; they are not range variables
        and do not vary within a Section.

.. c:function:: void nrn_section_rallbranch_set(Section* sec, double val)

    Set the Rallbranch value of a section.

    :param sec: Pointer to the section.
    :param val: Rallbranch value to set.

.. c:function:: char const* nrn_secname(Section* sec)

    Get the name of a section.

    :param sec: Pointer to the section.
    :returns: Null-terminated string containing the section name.

    **Usage Pattern:**

    Used for debugging, logging, or displaying section information.
    Inside of a loop, this is sometimes used to identify the Section category
    (e.g., does the section name start with ``dend``? If so, maybe we have a special
    rule for how to handle dendrites), but that effect could also be obtained by
    using a SectionList.

    Once a Section has been created, its name cannot be changed.

    **C Usage:**
    
    .. code-block:: c
    
        const char* name = nrn_secname(soma);

    **Python Equivalent:**
    
    .. code-block:: python
    
        name = str(soma)

.. c:function:: void nrn_section_push(Section* sec)

    Push a section onto the section stack, making it the currently accessed section.
    Many NEURON operations work on the currently accessed section.

    :param sec: Pointer to the section to push.

    **Usage Pattern:**

    Used when you need to perform operations that require a section to be "currently accessed."
    Always pair with ``nrn_section_pop()`` to restore the previous state.

    A call to a NEURON function from Python with a ``sec=`` effectively pushes the section,
    runs the function, and then pops the section.

    **C Usage:**
    
    .. code-block:: c
    
        nrn_section_push(soma);           // Make soma current
        // Perform operations on the soma
        nrn_section_pop();                // Restore previous section

    .. seealso::
    
        :c:func:`nrn_section_pop`

.. c:function:: void nrn_section_pop(void)

    Pop the top section from the section stack, restoring the previously accessed section.

    **Usage Pattern:**

    Always used after ``nrn_section_push()`` to restore the section stack state.

    .. seealso::
    
        :c:func:`nrn_section_push`

.. c:function:: void nrn_mechanism_insert(Section* sec, const Symbol* mechanism)

    Insert a density mechanism into a section. 

    :param sec: Pointer to the target section.
    :param mechanism: Symbol representing the mechanism to insert.

    **Usage Pattern:**
    Used to add biophysical properties to sections. 
    Density mechanisms are present at all locations within the section, but their
    properties (when specified as RANGE) may vary within the section.
    Built-in mechanisms include 'pas' (passive) and 'hh' (Hodgkin-Huxley). 
    Others are available from many sources, including `ModelDB <https://modeldb.science>`_ 
    and `Channelpedia <https://channelpedia.epfl.ch>`_.

    **C Usage:**
    
    .. code-block:: c
    
        Symbol* pas_symbol = nrn_symbol("pas");
        nrn_mechanism_insert(soma, pas_symbol);  // Insert passive mechanism
        
        Symbol* hh_symbol = nrn_symbol("hh");
        nrn_mechanism_insert(soma, hh_symbol);   // Insert Hodgkin-Huxley mechanism

    **Python Equivalent:**
    
    .. code-block:: python
    
        soma.insert('pas')  # Insert passive mechanism; or alternatively soma.insert(n.pas)
        soma.insert('hh')   # Insert Hodgkin-Huxley mechanism
        
    .. seealso::
    
        :c:func:`nrn_symbol`    

.. c:function:: nrn_Item* nrn_allsec(void)

    Get all sections in the current model.

    :returns: Pointer to a ``nrn_Item`` containing the list of all sections.

    **Usage Pattern:**

    Used with :c:func:`nrn_sectionlist_iterator_new` to iterate over all sections
    in the model, often for applying operations globally or for analysis purposes.
    This is equivalent to using ``n.allsec()`` in Python and shares the same caveats.
    In particular, future versions of the model may introduce new sections whose
    properties would be different, so consider using specifically chosen SectionLists
    instead of looping over all sections.

    **C Usage:**
    
    .. code-block:: c
    
        nrn_Item* all_sections = nrn_allsec();
        // Use iterator to process all sections
        SectionListIterator* iter = nrn_sectionlist_iterator_new(all_sections);
        while (!nrn_sectionlist_iterator_done(iter)) {
            Section* sec = nrn_sectionlist_iterator_next(iter);
            // Process section
        }
        nrn_sectionlist_iterator_free(iter);

    **Python Equivalent:**
    
    .. code-block:: python
    
        for sec in n.allsec():
            # Process section
            pass

.. c:function:: nrn_Item* nrn_sectionlist_data(const Object* obj)

    Given a NEURON ``SectionList`` object, return a ``nrn_Item*`` that can be used to
    loop over the sections.

    :param obj: Pointer to a SectionList object.
    :returns: ``nrn_Item*`` suitable for iteration.

    The ``nrn_Item*`` returned can be used for loops in the same way as the ``all_sections`` variable in
    the example in :c:func:`nrn_allsec`.

.. c:function:: bool nrn_section_is_active(const Section* sec)

    Check if a section is active (exists and is valid).

    :param sec: Pointer to the section to check.
    :returns: ``true`` if the section is active, ``false`` otherwise.

    **Usage Pattern:**

    Used for validation before performing operations on sections, especially
    when sections might have been deleted or are from external sources.
    Inactive sections might arise if the section has been explicitly deleted
    but is referenced in a SectionList. Each iteration over a SectionList
    checks for inactive sections and removes them (they are not returned).
    Only after there are no references to a deleted Section will its memory be freed.

.. c:function:: Section* nrn_cas(void)

    Get the currently accessed section (top of the section stack).

    :returns: Pointer to the currently accessed section, or NULL if stack is empty.

    **Usage Pattern:**

    Used to determine which section is currently active for operations that
    depend on the section stack state.

    **C Usage:**
    
    .. code-block:: c
    
        Section* current_sec = nrn_cas();  // Get currently accessed section

    **Python Equivalent:**
    
    .. code-block:: python
    
        current_sec = n.cas()  # Get currently accessed section


Segments
--------

.. c:function:: int nrn_nseg_get(const Section* sec)

    Get the number of segments in a section. Segments are computational compartments
    within a section used for numerical integration.

    :param sec: Pointer to the section.
    :returns: Number of segments in the section.

    **Usage Pattern:**

    The number of segments determines the spatial resolution of the simulation.
    More segments provide higher accuracy but increase computational cost.
    The number of segments is sometimes set based on the d-lambda rule.

    **C Usage:**
    
    .. code-block:: c
    
        int n_segments = nrn_nseg_get(soma);

    **Python Equivalent:**
    
    .. code-block:: python
    
        n_segments = soma.nseg  # Get number of segments

.. c:function:: void nrn_nseg_set(Section* sec, int nseg)

    Set the number of segments in a section.

    :param sec: Pointer to the section.
    :param nseg: Number of segments to set (must be ≥ 1).

    **Usage Pattern:**

    Typically set based on the d-lambda rule or manual specification for accuracy.
    Common values are 1, 3, 5, etc. (We recommend using odd numbers for nseg so
    that there is always a node centered around the section center. With an even number of segments,
    the center node would be at the border between two segments.))

    **C Usage:**
    
    .. code-block:: c
    
        nrn_nseg_set(soma, 1);     // Single compartment
        nrn_nseg_set(dendrite, 5); // Five compartments 

    **Python Equivalent:**
    
    .. code-block:: python
    
        soma.nseg = 1     # Single compartment
        dendrite.nseg = 5 # Five compartments

.. c:function:: void nrn_segment_diam_set(Section* sec, double x, double diam)

    Set the diameter of a segment (specified as normalized position x along the section).

    :param sec: Pointer to the section.
    :param x: Normalized position along section (0.0 to 1.0).
    :param diam: Diameter in microns.

    **Usage Pattern:**

    Used to define the morphological shape of sections. The diameter can vary
    along the length of a section to model tapering dendrites or axons.

    **C Usage:**
    
    .. code-block:: c
    
        // Set at specific location
        nrn_segment_diam_set(soma, 0.5, 25.0);  // Set diameter at middle to 25 μm

        // set diameter for all of dend uniformly
        int nseg = nrn_nseg_get(dend);
        for (int i = 0; i < nseg; i++) {
            double x = (i + 0.5) / nseg;
            nrn_segment_diam_set(dend, x, 10.0); // Set each segment diameter to 10 μm
        }

    **Python Equivalent:**
    
    .. code-block:: python
    
        # Set at specific location
        soma(0.5).diam = 25   # Set diameter at middle of section

        # Set diameter everywhere
        dend.diam = 10
    
    .. warning::

        Setting segment diameters will have no effect if 3d points have been specified
        for that Section via ``pt3dadd``. To allow diameter specification after that,
        first call ``pt3dclear`` to remove the 3d points.

.. c:function:: double nrn_segment_diam_get(Section* sec, double x)

    Get the diameter of a segment at normalized position x along the section.

    :param sec: Pointer to the section.
    :param x: Normalized position along section (0.0 to 1.0).
    :returns: Diameter in microns at the specified position.

    **C Usage:**
    
    .. code-block:: c
    
        double diameter = nrn_segment_diam_get(soma, 0.5);  // Get diameter at middle

    **Python Equivalent:**
    
    .. code-block:: python
    
        diameter = soma(0.5).diam  # Get diameter at middle of section

.. c:function:: void nrn_rangevar_push(Symbol* sym, Section* sec, double x)

    Push a range variable for a section at position x onto the NEURON stack.
    Range variables are properties that can vary along the length of a section.

    :param sym: Symbol representing the range variable.
    :param sec: Pointer to the section.
    :param x: Normalized position along section (0.0 to 1.0).

    **Usage Pattern:**

    Push a range variable when it is the argument to a NEURON function/method call.
    For memory safety, use functions like this instead of passing around raw pointers.

    **Example:**

    .. code-block:: c
    
        // Push the range variable for soma(0.5).v onto the stack
        // assumes soma is a Section* and we wish to record the voltage at 0.5 over time
        Symbol* sym = nrn_symbol("v");
        nrn_rangevar_push(sym, soma, 0.5);

        // Now you can use this pushed variable in a method call
        // For example, assume vec is a NEURON Vector Object*
        nrn_method_call(vec, "record", 1);  // Call record method with 1 argument (the pushed range variable)

.. c:function:: double nrn_rangevar_get(Symbol* sym, Section* sec, double x)

    Get the value of a range variable at position x in a section.

    :param sym: Symbol representing the range variable.
    :param sec: Pointer to the section.
    :param x: Normalized position along section (0.0 to 1.0).
    :returns: Value of the range variable at the specified position.

    **Usage Pattern:**

    Used to read spatially distributed properties such as:
    - ``g_pas``: passive conductance
    - ``m_hh``: sodium channel gating variable
    - ``v``: membrane voltage

    **C Usage:**
    
    .. code-block:: c
            
        // Get membrane voltage
        Symbol* v_sym = nrn_symbol("v");
        double voltage = nrn_rangevar_get(v_sym, soma, 0.5);

    **Python Equivalent:**
    
    .. code-block:: python
    
        # Get membrane voltage
        voltage = soma(0.5).v

.. c:function:: void nrn_rangevar_set(Symbol* sym, Section* sec, double x, double value)

    Set the value of a range variable at position x in a section.

    :param sym: Symbol representing the range variable.
    :param sec: Pointer to the section.
    :param x: Normalized position along section (0.0 to 1.0).
    :param value: Value to set for the range variable.

    **Usage Pattern:**

    Used to configure biophysical properties of sections, such as:
    - Setting channel densities
    - Configuring passive properties
    - Initializing membrane voltages

    **C Usage:**
    
    .. code-block:: c
    
        // Set initial voltage
        Symbol* v_sym = nrn_symbol("v");
        nrn_rangevar_set(v_sym, soma, 0.5, -65.0);  // mV

        // set passive conductance at all segments of dend
        Symbol* g_pas_sym = nrn_symbol("g_pas");
        int nseg = nrn_nseg_get(dend);
        for (int i = 0; i < nseg; i++) {
            double x = (i + 0.5) / nseg;
            nrn_rangevar_set(g_pas_sym, dend, x, 0.001); // 0.001 S/cm²
        }


    **Python Equivalent:**
    
    .. code-block:: python

        # Set initial voltage at the center of the soma
        soma(0.5).v = -65  # mV

        # set passive conductance at all segments of dend
        for seg in dend:
            seg.g_pas = 0.001  # S/cm²


Functions, objects, and the stack
---------------------------------

.. c:function:: Symbol* nrn_symbol(const char* name)

    Get a symbol by name from NEURON's symbol table. Symbols represent variables,
    functions, mechanisms, and other named entities in NEURON.

    :param name: Name of the symbol to lookup.
    :returns: Pointer to the Symbol object, or NULL if not found.

    **Usage Pattern:**

    Used to access NEURON built-in functions, variables, and mechanisms by name.
    Symbols only need to be looked up once; the returned pointer can be reused.

    **C Usage:**
    
    .. code-block:: c
    
        // Access built-in NEURON functions
        Symbol* finitialize_sym = nrn_symbol("finitialize");
        nrn_double_push(-65.0);  // Push voltage argument
        nrn_function_call(finitialize_sym, 1);  // Initialize membrane voltage
        
        Symbol* fadvance_sym = nrn_symbol("fadvance");
        nrn_function_call(fadvance_sym, 0);  // Advance simulation by one time step

    **Python Equivalent:**
    
    .. code-block:: python
    
        # Access built-in NEURON functions
        h.finitialize(-65)  # Initialize membrane voltage
        h.fadvance()        # Advance simulation by one time step

.. c:function:: void nrn_symbol_push(Symbol* sym)

    Push a symbol onto the HOC execution stack.

    :param sym: Pointer to the symbol to push.


.. c:function:: int nrn_symbol_type(const Symbol* sym)

    Get the type of a symbol (e.g., function, variable, mechanism).

    :param sym: Pointer to the symbol.
    :returns: Integer representing the symbol type.

    **Usage Pattern:**

    Used to determine what kind of entity a symbol represents before
    performing type-specific operations. For example, the MATLAB interface
    uses this as part of dynamically generating the interface.

.. c:function:: int nrn_symbol_subtype(const Symbol* sym)

    Get the subtype of a symbol, providing more detailed classification.

    :param sym: Pointer to the symbol.
    :returns: Integer representing the symbol subtype.

    The meanings of the symbol subtype code depends on the symbol type.
    For example, ``t`` is a built-in double variable and has a different subtype
    than a user-created double variable.

.. c:function:: double* nrn_symbol_dataptr(const Symbol* sym)

    Get a pointer to the data for a symbol (for variables).

    :param sym: Pointer to the symbol.
    :returns: Pointer to the symbol's data, or NULL if not applicable.

    **Usage Pattern:**

    Provides direct access to variable data for efficient reading/writing.
    e.g., use this for getting/setting the value of ``t`` (time).

.. c:function:: bool nrn_symbol_is_array(const Symbol* sym)

    Check if a symbol represents an array.

    :param sym: Pointer to the symbol.
    :returns: true if the symbol is an array, false otherwise.

    **Usage Pattern:**

    Used to determine if special array access methods are needed.
    For example, :class:`VClamp` objects have an array of ``amp`` values.

.. c:function:: void nrn_double_push(double val)

    Push a double value onto the NEURON execution stack.

    :param val: Double value to push.

    **Usage Pattern:**

    Used when preparing arguments for function/method calls.

.. c:function:: double nrn_double_pop(void)

    Pop a double value from the NEURON stack.

    :returns: Double value from the top of the stack.

    **Usage Pattern:**

    Used to retrieve function/method return values.

.. c:function:: void nrn_double_ptr_push(double* addr)

    Push a pointer to a double onto the stack.

    :param addr: Pointer to double to push.

    **Usage Pattern:**

    Used for passing references to variables that can be modified by functions.
    These pointers can be to variables from NEURON or to local variables.

.. c:function:: double* nrn_double_ptr_pop(void)

    Pop a pointer to a double from the stack.

    :returns: Pointer to double from the top of the stack.

    .. warning::

        Using pointers risks dereferencing invalid memory if the pointer is not valid.
        Prefer other strategies for memory safety.

.. c:function:: void nrn_str_push(char** str)

    Push a string onto the stack.

    :param str: Pointer to string pointer to push.

    **C++ Usage:**

    .. code-block:: C++

        // Load stdrun.hoc using the NEURON API
        std::string filename = "stdrun.hoc";
        char* cstr = const_cast<char*>(filename.c_str());
        nrn_str_push(&cstr);
        Symbol* load_file_sym = nrn_symbol("load_file");
        nrn_function_call(load_file_sym, 1);
    
    **Python Equivalent:**

    .. code-block:: python
    
        # Load stdrun.hoc using the NEURON API (Python version)
        h.load_file("stdrun.hoc")

.. c:function:: char** nrn_pop_str(void)

    Pop a string from the stack.

    :returns: Pointer to string pointer from the top of the stack.

    **Usage Pattern:**

    Used to retrieve function/method return values.

.. c:function:: void nrn_int_push(int i)

    Push an integer onto the stack.

    :param i: Integer value to push.

    .. warning::

        Most NEURON functions expect doubles not ints and may fail if an int is pushed instead.

.. c:function:: int nrn_int_pop(void)

    Pop an integer from the stack.

    :returns: Integer value from the top of the stack.

    **Usage Pattern:**

    Used to retrieve function/method return values.

    .. warning::

        Most NEURON functions when accessed through the API return doubles not ints and may fail if an int is pushed instead.
        This is true even for functions that return an integer value in Python.

.. c:function:: void nrn_object_push(Object* obj)

    Push an object onto the stack.

    :param obj: Pointer to object to push.

    **Usage Pattern:**

    Used when passing objects as arguments to functions or methods.

.. c:function:: Object* nrn_object_pop(void)

    Pop an object from the stack.

    :returns: Pointer to object from the top of the stack.

    **Usage Pattern:**

    Used to retrieve function/method return values. Use :c:func:`nrn_stack_type` to check the type
    before popping, or use the type of the function/method to know the expected return type in
    advance.


.. c:function:: nrn_stack_types_t nrn_stack_type(void)

    Get the type of the value on top of the stack without removing it.

    :returns: Enumeration value indicating the stack top type.

    **Usage Pattern:**

    Used for type checking before popping values to ensure correct handling.

    .. seealso::
    
        :c:func:`nrn_stack_type_name`, 
        :c:func:`nrn_double_pop`,
        :c:func:`nrn_double_ptr_pop`,
        :c:func:`nrn_int_pop`,
        :c:func:`nrn_object_pop`,
        :c:func:`nrn_pop_str`

.. c:function:: char const* nrn_stack_type_name(nrn_stack_types_t id)

    Get the name of a stack type as a human-readable string.

    :param id: Stack type enumeration value.
    :returns: String representation of the stack type.


.. c:function:: Object* nrn_object_new(Symbol* sym, int narg)

    Create a new object instance of the type represented by the symbol.

    :param sym: Symbol representing the object class/type.
    :param narg: Number of constructor arguments on the stack.
    :returns: Pointer to the newly created object.

    **Usage Pattern:**

    Used to instantiate NEURON objects like :class:`Vector`, :class:`NetCon`, :class:`SEClamp`, etc.
    Constructor arguments must be pushed onto the stack before calling.

    **C Usage:**
    
    .. code-block:: c
    
        // Create NEURON Vector with 100 elements
        Symbol* vec_sym = nrn_symbol("Vector");
        nrn_double_push(100);
        Object* vec = nrn_object_new(vec_sym, 1);
        
        // Create current clamp at soma
        Symbol* iclamp_sym = nrn_symbol("IClamp");
        nrn_object_push((Object*)soma);  // Push soma as object
        nrn_double_push(0.0);            // Push location (0.0)
        Object* iclamp = nrn_object_new(iclamp_sym, 2);

    **Python Equivalent:**
    
    .. code-block:: python
    
        # Create NEURON objects
        vec = h.Vector(100)           # Vector with 100 elements
        iclamp = h.IClamp(soma(0))    # Current clamp at soma

.. c:function:: Symbol* nrn_method_symbol(const Object* obj, const char* name)

    Get a method symbol from an object by name.

    :param obj: Pointer to the object.
    :param name: Name of the method to lookup.
    :returns: Pointer to the method symbol, or NULL if not found.

    **Usage Pattern:**

    Used to access object methods dynamically; essential for method calls.
    See :c:func:`nrn_method_call` for an example.

.. c:function:: void nrn_method_call(Object* obj, Symbol* method_sym, int narg)

    Call a method on a NEURON object.

    :param obj: Pointer to the object.
    :param method_sym: Symbol representing the method to call.
    :param narg: Number of arguments on the stack.

    **Usage Pattern:**

    Used to invoke object methods. Arguments must be pushed onto the stack
    before calling. Return values (if any) are left on the stack.

    **C Usage:**
    
    .. code-block:: c
    
        // Resize vector to 200 elements
        Symbol* resize_method = nrn_method_symbol(vec, "resize");
        nrn_double_push(200);
        nrn_method_call(vec, resize_method, 1);
        Object* returned_obj = nrn_object_pop();
        
        // Fill vector with zeros
        Symbol* fill_method = nrn_method_symbol(vec, "fill");
        nrn_double_push(0.0);
        nrn_method_call(vec, fill_method, 1);
        Object* returned_obj2 = nrn_object_pop();
        
        // Get vector size
        Symbol* size_method = nrn_method_symbol(vec, "size");
        nrn_method_call(vec, size_method, 0);
        double length = nrn_double_pop();

    **Python Equivalent:**
    
    .. code-block:: python
    
        vec.resize(200)     # Resize vector to 200 elements
        vec.fill(0)         # Fill vector with zeros
        length = vec.size() # Get vector size

.. c:function:: void nrn_function_call(Symbol* sym, int narg)

    Call a function by symbol.

    :param sym: Symbol representing the function to call.
    :param narg: Number of arguments on the stack.

    **Usage Pattern:**

    Used to call global functions and built-in NEURON functions.
    Arguments must be prepared on the stack before calling.

    **C Usage:**

    .. code-block:: c

        // Call finitialize(-65)
        Symbol* finitialize_sym = nrn_symbol("finitialize");
        nrn_double_push(-65.0);  // Push argument
        nrn_function_call(finitialize_sym, 1);

    .. code-block:: c

        // Call fadvance()
        Symbol* fadvance_sym = nrn_symbol("fadvance");
        nrn_function_call(fadvance_sym, 0);

    **Python Equivalent:**

    .. code-block:: python

        h.finitialize(-65)  # Initialize membrane voltage
        h.fadvance()         # Advance simulation by one time step

.. c:function:: void nrn_object_ref(Object* obj)

    Increment the reference count of an object.

    :param obj: Pointer to the object.

    **Usage Pattern:**

    Used for memory management. When storing object pointers, increment
    the reference count to prevent premature deletion. Decrement when done.

    .. note::

        Objects are automatically deleted when their reference count reaches zero.
        Always match ``nrn_object_ref()`` with a corresponding ``nrn_object_unref()`` call
        to prevent memory leaks.
    
    .. seealso::
    
        :c:func:`nrn_object_unref`

.. c:function:: void nrn_object_unref(Object* obj)

    Decrement the reference count of an object. When the count reaches zero,
    the object is automatically deleted.

    :param obj: Pointer to the object.

    **Usage Pattern:**

    Used for memory management. Always match with previous ``nrn_object_ref()``
    calls to prevent segmentation faults from premature deletion.

    .. seealso::

        :c:func:`nrn_object_ref`

.. c:function:: char const* nrn_class_name(const Object* obj)

    Get the class name of an object.

    :param obj: Pointer to the object.
    :returns: String containing the class name.

    **Usage Pattern:**

    Used for type identification, debugging, and polymorphic operations.

    **C Usage:**
    
    .. code-block:: c
    
        const char* class_name = nrn_class_name(obj);

    **Python Equivalent:**
    
    .. code-block:: python
    
        class_name = obj.hname().split('[')[0]

.. c:function:: bool nrn_prop_exists(const Object* obj)

    Check if properties exist for an object. Properties might not exist if the object
    is a point process that has not been placed into a Section.

    :param obj: Pointer to the object.
    :returns: true if the object has properties, false otherwise.

    **Usage Pattern:**

    Used for validation before attempting property access operations (getting/setting).
    Attempting to access properties (e.g., an :class:`IClamp` object's ``amp``) of a point process
    that has not been placed into a Section will result in a segmentation fault (so check with this
    function first).

.. c:function:: double nrn_distance(Section* sec0, double x0, Section* sec1, double x1)

    Compute the distance between two points in potentially different sections along the neuron.
    This calculates the path length through the dendritic tree between the specified points.

    :param sec0: Pointer to the first section.
    :param x0: Normalized position in first section (0.0 to 1.0).
    :param sec1: Pointer to the second section.
    :param x1: Normalized position in second section (0.0 to 1.0).
    :returns: Distance in microns along the morphological path.

    **Usage Pattern:**

    Used for spatial analysis, determining non-uniform ion channel conductances (e.g., in
    CA1 Pyramidal neurons the A current might increase with distance from the soma),
    calculating electrotonic distance, or determining
    the morphological distance between synapses and recording sites.

    **C Usage:**
    
    .. code-block:: c
    
        // Calculate distance from soma center to dendrite tip
        double dist = nrn_distance(soma, 0.5, dendrite, 1.0);

    **Python Equivalent:**
    
    .. code-block:: python
    
        # Calculate distance from soma center to dendrite tip
        dist = n.distance(soma(0.5), dendrite(1.0))
    

    .. note::

        This function exists to avoid having to set a global reference point when using
        :func:`distance`.

Shape Plot
----------

.. c:function:: ShapePlotInterface* nrn_get_plotshape_interface(Object* ps)

    Get the shape plot interface from a PlotShape object. This provides access
    to the internal plotting data and configuration.

    :param ps: Pointer to a PlotShape object.
    :returns: Pointer to the ShapePlotInterface.

    **Usage Pattern:**

    Used by plotting functions to extract morphological and variable
    data for visualization. The specific data may be queried with other functions,
    described below.

    **C Usage:**
    
    .. code-block:: c
    
        Symbol* plotshape_sym = nrn_symbol("PlotShape");
        Object* ps = nrn_object_new(plotshape_sym, 0);
        
        // Set variable to plot
        char const* var_name = "v";
        Symbol* variable_method = nrn_method_symbol(ps, "variable");
        nrn_str_push((char**)&var_name);
        nrn_method_call(ps, variable_method, 1);
        
        // Extract plot data
        ShapePlotInterface* spi = nrn_get_plotshape_interface(ps);

    **Python Equivalent:**
    
    .. code-block:: python
    
        ps = h.PlotShape(False)
        ps.variable('v')     # Set variable to plot
        # Data extraction would need custom implementation

.. c:function:: Object* nrn_get_plotshape_section_list(ShapePlotInterface* spi)

    Get the section list from a shape plot interface.

    :param spi: Pointer to the ShapePlotInterface.
    :returns: Pointer to the Object representing the section list.

    .. seealso::

        :c:func:`nrn_sectionlist_data`

.. c:function:: const char* nrn_get_plotshape_varname(ShapePlotInterface* spi)

    Get the variable name used in a shape plot.

    :param spi: Pointer to the ShapePlotInterface.
    :returns: String containing the variable name.

.. c:function:: float nrn_get_plotshape_low(ShapePlotInterface* spi)

    Get the lower bound for color scaling in a shape plot.

    :param spi: Pointer to the ShapePlotInterface.
    :returns: Lower bound value for color mapping.

.. c:function:: float nrn_get_plotshape_high(ShapePlotInterface* spi)

    Get the upper bound for color scaling in a shape plot.

    :param spi: Pointer to the ShapePlotInterface.
    :returns: Upper bound value for color mapping.


Miscellaneous
-------------

.. c:function:: int nrn_hoc_call(char const* command)

    Execute a HOC command string. HOC is NEURON's built-in scripting language.

    :param command: Null-terminated string containing the HOC command.
    :returns: Status code

    **Usage Pattern:**

    Provides a way to execute arbitrary NEURON/HOC commands from C code.
    Useful for operations not directly exposed through the C API.

    **C Usage:**
    
    .. code-block:: c
    
        nrn_hoc_call("topology()");           // Display topology
        nrn_hoc_call("forall psection()");    // Print all sections
        nrn_hoc_call("celsius = 37");         // Set temperature

    **Python Equivalent:**
    
    .. code-block:: python
    
        h('topology()')           # Display topology
        h('forall psection()')    # Print all sections
        h.celsius = 37            # Set temperature

    .. note::

        When constructing language bindings for NEURON, support for ``nrn_hoc_call`` is an
        important function, because it allows you to see the effects of each newly added
        feature and it provides a validation comparison.

.. c:function:: SectionListIterator* nrn_sectionlist_iterator_new(nrn_Item* my_sectionlist)

    Create a new section list iterator for traversing a list of sections.

    :param my_sectionlist: Pointer to the section list data.
    :returns: Pointer to the new SectionListIterator.

    **Usage Pattern:**

    Used to iterate over collections of sections efficiently. Essential for
    operations that need to process all sections or subsets.

    See :c:func:`nrn_allsec` for an example of iterating over all sections.

.. c:function:: void nrn_sectionlist_iterator_free(SectionListIterator* sl)

    Free a section list iterator and release associated resources.

    :param sl: Pointer to the SectionListIterator to free.

    **Usage Pattern:**

    Always call after finishing section list iteration to prevent memory leaks.

    See :c:func:`nrn_allsec` for an example of iterating over all sections.


.. c:function:: Section* nrn_sectionlist_iterator_next(SectionListIterator* sl)

    Get the next section from a section list iterator.

    :param sl: Pointer to the SectionListIterator.
    :returns: Pointer to the next Section.

    **Usage Pattern:**

    Used in loops to process each section in a list sequentially.

    Before calling, check with :c:func:`nrn_sectionlist_iterator_done` to ensure
    there are more sections to process.

    See :c:func:`nrn_allsec` for an example of iterating over all sections.


.. c:function:: int nrn_sectionlist_iterator_done(SectionListIterator* sl)

    Check if the section list iterator has finished iterating.

    :param sl: Pointer to the SectionListIterator.
    :returns: Non-zero if iteration is complete, 0 otherwise.

    **Usage Pattern:**

    Used as a loop termination condition.

    **Example iteration pattern:**
    
    .. code-block:: c
    
        SectionListIterator* iter = nrn_sectionlist_iterator_new(section_list);
        while (!nrn_sectionlist_iterator_done(iter)) {
            Section* sec = nrn_sectionlist_iterator_next(iter);
            // Process section
        }
        nrn_sectionlist_iterator_free(iter);

.. c:function:: SymbolTableIterator* nrn_symbol_table_iterator_new(Symlist* my_symbol_table)

    Create a new symbol table iterator for traversing symbols.

    :param my_symbol_table: Pointer to the symbol table.
    :returns: Pointer to the new SymbolTableIterator.

    **Usage Pattern:**

    Used to iterate over NEURON's symbol tables to discover available
    functions, variables, and mechanisms. This allows language bindings to dynamically
    discover the NEURON interface, including user-defined functions added during runtime.

    **C++ Usage:**

    .. code-block:: C++
    
        // Retrieve the global and top-level symbol tables
        auto global_symtable = nrn_global_symbol_table();
        auto top_level_symtable = nrn_top_level_symbol_table();
        std::string result;

        // Iterate over both symbol tables
        for (auto symtable : {global_symtable, top_level_symtable}) {
            // Create an iterator for the current symbol table
            auto iter = nrn_symbol_table_iterator_new(symtable);

            // Loop through all symbols in the table
            while (!nrn_symbol_table_iterator_done(iter)) {
                // Get symbol
                Symbol* sym = nrn_symbol_table_iterator_next(iter);

                // Retrieve the symbol name and its type/subtype
                const char* name = nrn_symbol_name(sym);
                int type = nrn_symbol_type(sym);
                int subtype = nrn_symbol_subtype(sym);

                std::cout << "Symbol: " << name 
                        << ", Type: " << type 
                        << ", Subtype: " << subtype << std::endl;
            }

            // Free the iterator after use
            nrn_symbol_table_iterator_free(iter);
        }


    .. seealso::

        :c:func:`nrn_global_symbol_table`, :c:func:`nrn_top_level_symbol_table`

.. c:function:: void nrn_symbol_table_iterator_free(SymbolTableIterator* st)

    Free a symbol table iterator.

    :param st: Pointer to the SymbolTableIterator to free.

    See :c:func:`nrn_symbol_table_iterator_new` for example usage.

.. c:function:: Symbol* nrn_symbol_table_iterator_next(SymbolTableIterator* st)

    Get the next symbol from a symbol table iterator.

    :param st: Pointer to the SymbolTableIterator.
    :returns: Pointer to the next Symbol.

    Be sure to check with :c:func:`nrn_symbol_table_iterator_done` before calling;
    this does not return NULL when done.

    See :c:func:`nrn_symbol_table_iterator_new` for example usage.


.. c:function:: int nrn_symbol_table_iterator_done(SymbolTableIterator* st)

    Check if the symbol table iterator has finished iterating.

    :param st: Pointer to the SymbolTableIterator.
    :returns: Non-zero if iteration is complete, 0 otherwise.

    See :c:func:`nrn_symbol_table_iterator_new` for example usage.


.. c:function:: int nrn_vector_capacity(const Object* vec)

    Get the capacity (allocated size) of a vector object.

    :param vec: Pointer to the Vector object.
    :returns: Capacity of the vector.

    **Usage Pattern:**

    Used for memory management and optimization when working with vectors.

.. c:function:: double* nrn_vector_data(Object* vec)

    Get direct access to the data array of a vector object.

    :param vec: Pointer to the Vector object.
    :returns: Pointer to the internal data array.

    **Usage Pattern:**

    Provides efficient access to vector data for bulk operations without
    going through the object interface.

    In language bindings, it may be possible to use this pointer to create
    a more native view into the data (e.g., in Python, a numpy array can be
    initialized from a pointer and a size, so the numpy array can directly
    be used to work with the Vector's data).

    **C Usage:**
    
    .. code-block:: c
    
        double* vec_data = nrn_vector_data(vec);  // Get vector data pointer

        // directly access elements
        for (int i = 0; i < 100; i++) {
            vec_data[i] = i * 0.1;  // Set values
        }

.. c:function:: double nrn_property_get(const Object* obj, const char* name)

    Get a property value from an object by name.

    :param obj: Pointer to the object.
    :param name: Name of the property.
    :returns: Value of the property.

    **Usage Pattern:**

    Used to read object properties dynamically by name. Essential for
    generic property access.

    **C Usage:**
    
    .. code-block:: c
    
        double amp = nrn_property_get(iclamp, "amp");      // Get current clamp amplitude
        double dur = nrn_property_get(iclamp, "dur");      // Get current clamp duration

    **Python Equivalent:**
    
    .. code-block:: python
    
        amp = iclamp.amp      # Get current clamp amplitude
        dur = iclamp.dur      # Get current clamp duration

.. c:function:: double nrn_property_array_get(const Object* obj, const char* name, int i)

    Get a value from a property array by index.

    :param obj: Pointer to the object.
    :param name: Name of the property array.
    :param i: Index into the array (0-based).
    :returns: Value at the specified index.

    **Usage Pattern:**

    Used for properties that are arrays.

    **C Usage:**

    .. code-block:: c
    
        double amp0 = nrn_property_array_get(vclamp, "amp", 0);  // Get first amplitude
        double amp1 = nrn_property_array_get(vclamp, "amp", 1);  // Get second amplitude

    **Python Equivalent:**

    .. code-block:: python
    
        amp0 = vclamp.amp[0]  # Get first amplitude
        amp1 = vclamp.amp[1]  # Get second amplitude

.. c:function:: void nrn_property_set(Object* obj, const char* name, double value)

    Set a property value on an object.

    :param obj: Pointer to the object.
    :param name: Name of the property.
    :param value: Value to set.

    **C Usage:**
    
    .. code-block:: c
    
        nrn_property_set(iclamp, "amp", 0.1);      // Set current amplitude to 0.1 nA
        nrn_property_set(iclamp, "del", 100.0);    // Set delay to 100 ms

    **Python Equivalent:**
    
    .. code-block:: python
    
        iclamp.amp = 0.1      # Set current amplitude to 0.1 nA
        iclamp.delay = 100    # Set delay to 100 ms

.. c:function:: void nrn_property_array_set(Object* obj, const char* name, int i, double value)

    Set a value in a property array.

    :param obj: Pointer to the object.
    :param name: Name of the property array.
    :param i: Index into the array (0-based).
    :param value: Value to set at the specified index.

.. c:function:: void nrn_property_push(Object* obj, const char* name)

    Push a property value onto the NEURON stack.

    :param obj: Pointer to the object.
    :param name: Name of the property.

    **Usage Pattern:**
    
    This allows the equivalent of the Python ``vec.play(iclamp._ref_amp, tvec, True)`` which
    is how NEURON can implement non-square-wave current clamps. Here ``iclamp._ref_amp`` is a reference
    to the ``amp`` property of the ``IClamp`` object.

.. c:function:: void nrn_property_array_push(Object* obj, const char* name, int i)

    Push a property array element onto the NEURON stack.

    :param obj: Pointer to the object.
    :param name: Name of the property array.
    :param i: Index into the array (0-based).

.. c:function:: char const* nrn_symbol_name(const Symbol* sym)

    Get the name of a symbol as a string.

    :param sym: Pointer to the symbol.
    :returns: String containing the symbol name.

    **Usage Pattern:**

    Used for debugging, introspection, and dynamic symbol handling.

    See :c:func:`nrn_symbol_table_iterator_new` for an example of iterating over symbols,
    which uses this function to get the symbol names.

.. c:function:: Symlist* nrn_symbol_table(const Symbol* sym)

    Get the symbol table that contains a symbol.

    :param sym: Pointer to the symbol.
    :returns: Pointer to the containing symbol table.

.. c:function:: Symlist* nrn_global_symbol_table(void)

    Get the global symbol table containing built-in NEURON functions and variables.

    :returns: Pointer to the global symbol table.

    See :c:func:`nrn_symbol_table_iterator_new` for an example of iterating over the
    global symbol table.


.. c:function:: Symlist* nrn_top_level_symbol_table(void)

    Get the top-level symbol table containing user-defined symbols.

    :returns: Pointer to the top-level symbol table.

.. c:function:: int nrn_symbol_array_length(const Symbol* sym)

    Get the length of a symbol array.

    :param sym: Pointer to the symbol.
    :returns: Length of the array, or 1 for non-arrays.

.. c:function:: void nrn_register_function(void (*proc)(), const char* func_name, int type)

    Register a C function to be callable from NEURON/HOC.

    :param proc: Pointer to the C function.
    :param func_name: Name by which the function will be known in NEURON.
    :param type: Function type identifier.

    **Usage Pattern:**

    Used to extend NEURON with custom C functions that can be called
    from HOC; for example, the MATLAB interface uses this to provide an ``nrn_matlab``
    function to HOC. Furtherore, this allows callback functions into a language binding,
    allowing, for example, callbacks in :meth:`CVode.event` or :class:`FInitializeHandler`.

    The function should end by calling :c:func:`nrn_hoc_ret()` and pushing its result to the stack.

.. c:function:: void nrn_hoc_ret(void)

    Return from a HOC function call.

    **Usage Pattern:**

    Used in custom functions registered with :c:func:`nrn_register_function()`
    to signal completion of execution.


Parameter-reading functions
---------------------------

.. c:function:: Object** nrn_objgetarg(int arg)

    Get an object argument from the NEURON stack during function execution.

    :param arg: Argument index (1-indexed).
    :returns: Pointer to pointer to the object argument.

    **Usage Pattern:**

    Used in custom functions registered with NEURON to access object
    arguments passed from NEURON/HOC.

    If it is not known that there is an argument at the specified index,
    use :c:func:`nrn_ifarg` to check before calling this function.

    If the type of the argument is not known in advance, use
    :c:func:`nrn_is_object_arg` to check before calling this function.

    **Example:**
    
    .. code-block:: c
    
        // In a custom function
        Object** obj_ptr = nrn_objgetarg(1);  // Get first object argument
        if (obj_ptr && *obj_ptr) {
            // Use the object
        }



.. c:function:: char* nrn_gargstr(int arg)

    Get a string argument from the NEURON stack during function execution.

    :param arg: Argument index (1-indexed).
    :returns: Pointer to the string argument.

    **Usage Pattern:**

    Used in custom functions registered with NEURON to access string
    arguments passed from NEURON/HOC.

    If it is not known that there is an argument at the specified index,
    use :c:func:`nrn_ifarg` to check before calling this function.

    If the type of the argument is not known in advance, use
    :c:func:`nrn_is_str_arg` to check before calling this function.

    **Example:**
    
    .. code-block:: c
    
        char* filename = nrn_gargstr(1);  // Get first string argument

.. c:function:: double* nrn_getarg(int arg)

    Get a double argument from the NEURON stack during function execution.

    :param arg: Argument index (1-indexed).
    :returns: Pointer to the double argument.

    **Usage Pattern:**

    Used in custom functions registered with NEURON to access double
    arguments passed from NEURON/HOC.

    If it is not known that there is an argument at the specified index,
    use :c:func:`nrn_ifarg` to check before calling this function.

    If the type of the argument is not known in advance, use
    :c:func:`nrn_is_double_arg` to check before calling this function.


    **Example:**
    
    .. code-block:: c
    
        double value = *nrn_getarg(1);  // Get first double argument

.. c:function:: FILE* nrn_obj_file_arg(int i)

    Get a file argument from the HOC stack during function execution.

    :param i: Argument index (1-indexed).
    :returns: Pointer to the FILE object.

    **Usage Pattern:**

    Used when custom functions need to work with file objects passed
    from NEURON/HOC.

.. c:function:: bool nrn_ifarg(int arg)

    Check if an argument exists at the specified position.

    :param arg: Argument index (1-indexed).
    :returns: true if argument exists, false otherwise.

    **Usage Pattern:**

    Used to implement optional parameters in custom functions.

    **Example:**
    
    .. code-block:: c
    
        if (nrn_ifarg(2)) {
            // Second argument was provided
            optional_param = *nrn_getarg(2);
        } else {
            // Use default value
            optional_param = default_value;
        }

.. c:function:: bool nrn_is_object_arg(int arg)

    Check if an argument is an object.

    :param arg: Argument index (1-indexed).
    :returns: true if argument is an object, false otherwise.

.. c:function:: bool nrn_is_str_arg(int arg)

    Check if an argument is a string.

    :param arg: Argument index (1-indexed).
    :returns: true if argument is a string, false otherwise.

.. c:function:: bool nrn_is_double_arg(int arg)

    Check if an argument is a double.

    :param arg: Argument index (1-indexed).
    :returns: true if argument is a double, false otherwise.

.. c:function:: bool nrn_is_pdouble_arg(int arg)

    Check if an argument is a pointer to double (reference parameter).

    :param arg: Argument index (1-indexed).
    :returns: true if argument is a pointer to double, false otherwise.



Common Usage Patterns
---------------------


Creating and Connecting Sections:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Create sections
   Section* soma = nrn_section_new("soma");
   Section* dendrite = nrn_section_new("dendrite");
   
   // Set properties
   nrn_section_length_set(soma, 20.0);     // 20 μm
   nrn_section_length_set(dendrite, 100.0); // 100 μm
   
   // Connect dendrite to soma
   nrn_section_connect(dendrite, 0.0, soma, 1.0);

Working with Objects and Methods:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Get Vector class symbol
   Symbol* vec_sym = nrn_symbol("Vector");
   
   // Create Vector with 100 elements
   nrn_double_push(100);
   Object* vec = nrn_object_new(vec_sym, 1);
   
   // Call Vector.fill(0)
   Symbol* fill_method = nrn_method_symbol(vec, "fill");
   nrn_double_push(0.0);
   nrn_method_call(vec, fill_method, 1);

Memory Management:
~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // When storing object pointers
   nrn_object_ref(obj);    // Increment reference count
   
   // When done with object
   nrn_object_unref(obj);  // Decrement reference count
