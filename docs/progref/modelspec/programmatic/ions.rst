Ions
====



.. function:: ion_style

    .. tab:: Python

        Syntax:

        .. code-block:: python

            oldstyle = n.ion_style("name_ion", c_style, e_style, einit, eadvance, cinit, sec=section)
            oldstyle = n.ion_style("name_ion", sec=section)

        Description:
            In the ``section``, 
            force the named ion (eg. na_ion, k_ion, ca_ion, etc) to handle 
            reversal potential and concentrations according to the indicated 
            styles. 
            You will not often need this function since the 
            style chosen automatically on a per section basis should be 
            appropriate to the set of mechanisms inserted in each section. 
            
            The ``oldstyle`` value is the previous internal setting of 
            ``c_style + 4*cinit + 8*e_style + 32*einit + 64*eadvance``. Additionally
            the two internal bit values of ``128*ciwrite`` and ``256*cowrite`` are
            present and set (not setable by user) only if the section has mechanisms that USEION
            WRITE the internal or external ion concentration, e.g. nai or nao.
            That is, to get the parameter values from ``oldstyle``, use

            .. code-block::
                python

                c_style = oldstyle % 4
                cinit = (oldstyle // 4) % 2
                e_style = (oldstyle // 8) % 4
                einit = (oldstyle // 32) % 2
                eadvance = (oldstyle // 64) % 2
                ci_is_written = (oldstyle // 128) % 2
                co_is_written = (oldsytle // 256) % 2

            c_style: 0, 1, 2, 3. 
                Concentrations respectively treated as UNUSED, 
                PARAMETER, ASSIGNED, or STATE variables.  Determines which panel (if 
                any) will show the concentrations. 

            e_style: 0, 1, 2, 3. 
                Reversal potential respectively treated as 
                UNUSED, PARAMETER, ASSIGNED, or STATE variable. 

            einit: 0 or 1. 
                If 1 then reversal potential computed by Nernst equation 
                on call to :func:`finitialize` using values of concentrations. 

            eadvance: 0 or 1. 
                If 1 then reversal potential computed every call to 
                :func:`fadvance` using the values of the concentrations. 

            cinit: 0 or 1. 
                If 1 then a call to :func:`finitialize` sets the concentrations 
                to the values of the global initial concentrations. eg. ``nai`` set to 
                ``n.nai0_na_ion`` and ``nao`` set to ``n.nao0_na_ion``. 

            The automatic style is chosen based on how the set of mechanisms that 
            have been inserted in a section use the ion. Note that the precedence is 
            WRITE > READ > unused in the USEION statement; so if one mechanism 
            READ's  cai/cao and another mechanism WRITE's them then WRITE takes precedence 
            in the following table. For compactness, the table assumes the ca ion. 
            Each table entry identifies the equivalent parameters to the ``ion_style`` 
            function. 

                ==========    =========   =========   =========
                cai/cao ->    unused      read        write 
                ==========    =========   =========   =========
                eca unused    0,0,0,0,0   1,0,0,0,0   3,0,0,0,1 
                eca read      0,1,0,0,0   1,2,1,0,0   3,2,1,1,1 
                eca write     0,2,0,0,0   1,2,0,0,0   3,2,0,0,1 
                ==========    =========   =========   =========

            For example suppose one has inserted a mechanism that READ's eca, 
            a mechanism that READ's cai, cao and a mechanism that WRITE's cai, cao 
            Then, since WRITE takes precedence over READ in the above table, 
            ``cai/cao`` would appear in the STATE variable panel (first arg is 3), 
            ``eca`` would appear in the ASSIGNED variable panel (second arg is 2), 
            ``eca`` would be calculated on a call to finitialize (third arg is 1), 
            ``eca`` would be calculated on every call to fadvance (fourth arg is 1), 
            ``cai/cao`` would be initialized (on finitialize) to the global variables 
            ``cai0_ca_ion`` and ``cao0_ca_ion`` respectively. (note that this takes place just 
            before the calculation of ``eca``). 

            .. warning::
            
                If other mechanisms are inserted subsequent to a call 
                of this function, the style will be "promoted" according to 
                the rules associated with adding the used ions to the style 
                previously in effect. 
            
            .. warning::

                If ``section`` is not specified, the style is set for the currently accessed
                section only (the section returned by :func:`n.cas() <cas>`). In particular,
                this does not change the style for all sections. If you need to change for all
                sections, loop over the sections, e.g.,

                .. code-block:: python

                    for sec in n.allsec():
                        n.ion_style("ca_ion", 3, 2, 1, 1, 1, sec=sec)

    .. tab:: HOC

        Syntax:

        .. code-block:: C++

            section {oldstyle = ion_style("name_ion", c_style, e_style, einit, eadvance, cinit)}
            section {oldstyle = ion_style("name_ion")}

        Description:
            In the ``section``, 
            force the named ion (eg. na_ion, k_ion, ca_ion, etc) to handle 
            reversal potential and concentrations according to the indicated 
            styles. 
            You will not often need this function since the 
            style chosen automatically on a per section basis should be 
            appropriate to the set of mechanisms inserted in each section. 
            
            The ``oldstyle`` value is the previous internal setting of 
            c_style + 4*cinit + 8*e_style + 32*einit + 64*eadvance. 
            That is, to get the parameter values from ``oldstyle``, use
            ``c_style = oldstyle % 4``,
            ``cinit = int(oldstyle / 4) % 2``,
            ``e_style = int(oldstyle / 8) % 4``,
            ``einit = int(oldstyle / 32) % 2``, and
            ``eadvance = int(oldstyle / 64) % 2``.
            Additionally
            the two internal bit values of ``128*ciwrite`` and ``256*cowrite`` are
            present and set (not setable by user) only if the section has mechanisms that USEION
            WRITE the internal or external ion concentration, e.g. nai or nao.
            That is, to get the parameter values from ``oldstyle``, use
            ``c_style = oldstyle % 4``,
            ``cinit = int(oldstyle / 4) % 2``,
            ``e_style = int(oldstyle / 8) % 4``,
            ``einit = int(oldstyle / 32) % 2``,
            ``eadvance = int(oldstyle / 64) % 2``,
            ``ciwrite = int(oldstyle / 128) % 2``, and
            ``cowrite = int(oldstyle / 256) % 2``.

            c_style: 0, 1, 2, 3. 
                Concentrations respectively treated as UNUSED, 
                PARAMETER, ASSIGNED, or STATE variables.  Determines which panel (if 
                any) will show the concentrations. 

            e_style: 0, 1, 2, 3. 
                Reversal potential respectively treated as 
                UNUSED, PARAMETER, ASSIGNED, or STATE variable. 

            einit: 0 or 1. 
                If 1 then reversal potential computed by Nernst equation 
                on call to :func:`finitialize` using values of concentrations. 

            eadvance: 0 or 1. 
                If 1 then reversal potential computed every call to 
                :func:`fadvance` using the values of the concentrations. 

            cinit: 0 or 1. 
                If 1 then a call to :func:`finitialize` sets the concentrations 
                to the values of the global initial concentrations. eg. ``nai`` set to 
                ``nai0_na_ion`` and ``nao`` set to ``nao0_na_ion``. 

            The automatic style is chosen based on how the set of mechanisms that 
            have been inserted in a section use the ion. Note that the precedence is 
            WRITE > READ > unused in the USEION statement; so if one mechanism 
            READ's  cai/cao and another mechanism WRITE's them then WRITE takes precedence 
            in the following table. For compactness, the table assumes the ca ion. 
            Each table entry identifies the equivalent parameters to the ``ion_style`` 
            function. 

                ==========    =========   =========   =========
                cai/cao ->    unused      read        write 
                ==========    =========   =========   =========
                eca unused    0,0,0,0,0   1,0,0,0,0   3,0,0,0,1 
                eca read      0,1,0,0,0   1,2,1,0,0   3,2,1,1,1 
                eca write     0,2,0,0,0   1,2,0,0,0   3,2,0,0,1 
                ==========    =========   =========   =========

            For example suppose one has inserted a mechanism that READ's eca, 
            a mechanism that READ's cai, cao and a mechanism that WRITE's cai, cao 
            Then, since WRITE takes precedence over READ in the above table, 
            ``cai/cao`` would appear in the STATE variable panel (first arg is 3), 
            ``eca`` would appear in the ASSIGNED variable panel (second arg is 2), 
            ``eca`` would be calculated on a call to finitialize (third arg is 1), 
            ``eca`` would be calculated on every call to fadvance (fourth arg is 1), 
            ``cai/cao`` would be initialized (on finitialize) to the global variables 
            ``cai0_ca_ion`` and ``cao0_ca_ion`` respectively. (note that this takes place just 
            before the calculation of ``eca``). 

            .. warning::
            
                If other mechanisms are inserted subsequent to a call 
                of this function, the style will be "promoted" according to 
                the rules associated with adding the used ions to the style 
                previously in effect. 
            
            .. warning::

                If ``section`` is not specified, the style is set for the currently accessed
                section only. In particular,
                this does not change the style for all sections. If you need to change for all
                sections, loop over the sections, e.g.,

                .. code-block:: C++

                    forall ion_style("ca_ion", 3, 2, 1, 1, 1)


    .. tab:: MATLAB

        Syntax:

        .. code-block:: matlab

            oldstyle = n.ion_style("name_ion", c_style, e_style, einit, eadvance, cinit, section);
            oldstyle = n.ion_style("name_ion", section);

        Description:
            In the ``section``, 
            force the named ion (eg. na_ion, k_ion, ca_ion, etc) to handle 
            reversal potential and concentrations according to the indicated 
            styles. 
            You will not often need this function since the 
            style chosen automatically on a per section basis should be 
            appropriate to the set of mechanisms inserted in each section. 
            
            The ``oldstyle`` value is the previous internal setting of 
            c_style + 4*cinit + 8*e_style + 32*einit + 64*eadvance. 
            Additionally
            the two internal bit values of ``128*ciwrite`` and ``256*cowrite`` are
            present and set (not setable by user) only if the section has mechanisms that USEION
            WRITE the internal or external ion concentration, e.g. nai or nao.
            That is, to get the parameter values from ``oldstyle``, use
]            ``c_style = mod(oldstyle, 4);``
            ``cinit = mod(floor(oldstyle / 4), 2);``
            ``e_style = mod(floor(oldstyle / 8), 4);``
            ``einit = mod(floor(oldstyle / 32), 2);`` 
            ``eadvance = mod(floor(oldstyle / 64), 2);``
            ``ciwrite = mod(floor(oldstyle / 128) , 2);`` and
            ``cowrite = mod(floor(oldstyle / 256), 2);``

            c_style: 0, 1, 2, 3. 
                Concentrations respectively treated as UNUSED, 
                PARAMETER, ASSIGNED, or STATE variables.  Determines which panel (if 
                any) will show the concentrations. 

            e_style: 0, 1, 2, 3. 
                Reversal potential respectively treated as 
                UNUSED, PARAMETER, ASSIGNED, or STATE variable. 

            einit: 0 or 1. 
                If 1 then reversal potential computed by Nernst equation 
                on call to :func:`finitialize` using values of concentrations. 

            eadvance: 0 or 1. 
                If 1 then reversal potential computed every call to 
                :func:`fadvance` using the values of the concentrations. 

            cinit: 0 or 1. 
                If 1 then a call to :func:`finitialize` sets the concentrations 
                to the values of the global initial concentrations. eg. ``nai`` set to 
                ``n.nai0_na_ion`` and ``nao`` set to ``n.nao0_na_ion``. 

            The automatic style is chosen based on how the set of mechanisms that 
            have been inserted in a section use the ion. Note that the precedence is 
            WRITE > READ > unused in the USEION statement; so if one mechanism 
            READ's  cai/cao and another mechanism WRITE's them then WRITE takes precedence 
            in the following table. For compactness, the table assumes the ca ion. 
            Each table entry identifies the equivalent parameters to the ``ion_style`` 
            function. 

                ==========    =========   =========   =========
                cai/cao ->    unused      read        write 
                ==========    =========   =========   =========
                eca unused    0,0,0,0,0   1,0,0,0,0   3,0,0,0,1 
                eca read      0,1,0,0,0   1,2,1,0,0   3,2,1,1,1 
                eca write     0,2,0,0,0   1,2,0,0,0   3,2,0,0,1 
                ==========    =========   =========   =========

            For example suppose one has inserted a mechanism that READ's eca, 
            a mechanism that READ's cai, cao and a mechanism that WRITE's cai, cao 
            Then, since WRITE takes precedence over READ in the above table, 
            ``cai/cao`` would appear in the STATE variable panel (first arg is 3), 
            ``eca`` would appear in the ASSIGNED variable panel (second arg is 2), 
            ``eca`` would be calculated on a call to finitialize (third arg is 1), 
            ``eca`` would be calculated on every call to fadvance (fourth arg is 1), 
            ``cai/cao`` would be initialized (on finitialize) to the global variables 
            ``cai0_ca_ion`` and ``cao0_ca_ion`` respectively. (note that this takes place just 
            before the calculation of ``eca``). 

            .. warning::
            
                If other mechanisms are inserted subsequent to a call 
                of this function, the style will be "promoted" according to 
                the rules associated with adding the used ions to the style 
                previously in effect. 
            
            .. warning::

                If ``section`` is not specified, the style is set for the currently accessed
                section only (the section returned by :func:`n.cas() <cas>`). In particular,
                this does not change the style for all sections. If you need to change for all
                sections, loop over the sections, e.g.,

                .. code-block:: matlab

                    for sec = n.allsec()
                        n.ion_style("ca_ion", 3, 2, 1, 1, 1, sec);
                    end

----



.. function:: ion_register

    .. tab:: Python

        Syntax:

        .. code-block:: python

            ion_type = n.ion_register("name", charge)

        Description:
            Create a new ion type with mechanism name, "name_ion", and associated 
            variables: ``iname``, ``nameo``, ``namei``, ``ename``, ``diname_dv``. 
            If any of these names already 
            exists and ``name_ion`` is not already an ion, the function returns -1, 
            otherwise it returns the mechanism type index. If ``name_ion`` is already 
            an ion the charge is ignored but the type index is returned. 

    .. tab:: HOC

        Syntax:

        .. code-block:: python

            ion_type = ion_register("name", charge)

        Description:
            Create a new ion type with mechanism name, "name_ion", and associated 
            variables: ``iname``, ``nameo``, ``namei``, ``ename``, ``diname_dv``. 
            If any of these names already 
            exists and ``name_ion`` is not already an ion, the function returns -1, 
            otherwise it returns the mechanism type index. If ``name_ion`` is already 
            an ion the charge is ignored but the type index is returned. 

    .. tab:: MATLAB

        Syntax:

        .. code-block:: matlab

            ion_type = n.ion_register("name", charge);

        Description:
            Create a new ion type with mechanism name, "name_ion", and associated 
            variables: ``iname``, ``nameo``, ``namei``, ``ename``, ``diname_dv``. 
            If any of these names already 
            exists and ``name_ion`` is not already an ion, the function returns -1, 
            otherwise it returns the mechanism type index. If ``name_ion`` is already 
            an ion the charge is ignored but the type index is returned. 

----



.. function:: ion_charge

    .. tab:: Python

        Syntax:

        .. code-block:: python

            charge = n.ion_charge("name_ion")

        Description:
            Return the charge for the indicated ion mechanism. An error message is 
            printed if ``name_ion`` is not an ion mechanism.

    .. tab:: HOC

        Syntax:

        .. code-block:: C++

            charge = ion_charge("name_ion")

        Description:
            Return the charge for the indicated ion mechanism. An error message is 
            printed if ``name_ion`` is not an ion mechanism.

    .. tab:: MATLAB

        Syntax:

        .. code-block:: matlab

            charge = n.ion_charge("name_ion");

        Description:
            Return the charge for the indicated ion mechanism. An error message is 
            printed if ``name_ion`` is not an ion mechanism.

