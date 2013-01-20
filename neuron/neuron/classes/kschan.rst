.. _kschan:

         
KSChan
------



.. class:: KSChan


    Syntax:
        :code:`kschan = new KSChan()`


    Description:
        Declare and manage a new density channel type which can 
        be instantiated in sections with the :meth:`keywords.insert` 
        statement. After the type comes into existence it 
        is always a valid type and the conductance style, 
        ligands, name, gating functions, etc can be changed 
        at any time. The type cannot be destroyed. 
         
        This is an extension of the KSChan managed by the 
        Java catacomb channel builder tool 
        for the past several 
        years. The primary functional extension is the 
        ability to define HH-style gates in addition to 
        kinetic scheme gates. The administrative extensions 
        allowed a more convenient re-implementation of the 
        channel builder gui in HOC --- albeit substantially 
        similar. The KSChan :meth:`KSChan.setstructure` method 
        uses a slightly modified vector format so the old 
        Java channelbuilder tool will not work without 
        updating the Java implementation. 
         

----



.. method:: KSChan.setstructure


    Syntax:
        :code:`kschan.setstructure(vec)`


    Description:

         

----



.. method:: KSChan.remove_state


    Syntax:
        :code:`kschan.remove_state(index)`

        :code:`kschan.remove_state(ksstate)`


    Description:


----



.. method:: KSChan.remove_transition


    Syntax:
        :code:`kschan.remove_transition(index)`

        :code:`kschan.remove_transition(kstrans)`


    Description:

         

----



.. method:: KSChan.ngate


    Syntax:
        :code:`n = kschan.ngate()`


    Description:


----



.. method:: KSChan.nstate


    Syntax:
        :code:`n = kschan.nstate()`


    Description:


----



.. method:: KSChan.ntrans


    Syntax:
        :code:`n = kschan.ntrans()`


    Description:


----



.. method:: KSChan.nligand


    Syntax:
        :code:`n = kschan.nligand()`


    Description:


----



.. method:: KSChan.pr


    Syntax:
        :code:`kschan.pr()`


    Description:

         

----



.. method:: KSChan.iv_type


    Syntax:
        :code:`type = kschan.iv_type()`

        :code:`type = kschan.iv_type(type)`


    Description:


----



.. method:: KSChan.gmax


    Syntax:
        :code:`val = kschan.gmax()`

        :code:`val = kschan.gmax(val)`


    Description:


----



.. method:: KSChan.erev


    Syntax:
        :code:`val = kschan.erev()`

        :code:`val = kschan.erev(val)`


    Description:

         

----



.. method:: KSChan.add_hhstate


    Syntax:
        :code:`ksstate = kschan.add_hhstate(name)`


    Description:


----



.. method:: KSChan.add_ksstate


    Syntax:
        :code:`ksstate = kschan.add_ksstate(name)`


    Description:


----



.. method:: KSChan.add_transition


    Syntax:
        :code:`kstrans = kschan.add_transition(src_index, target_index)`

        :code:`kstrans = kschan.add_transition(src_ksstate, target_ksstate)`


    Description:


----



.. method:: KSChan.trans


    Syntax:
        :code:`kstrans = kschan.trans(index)`

        :code:`kstrans = kschan.trans(src_ksstate, target_ksstate)`


    Description:


----



.. method:: KSChan.state


    Syntax:
        :code:`ksstate = kschan.state(index)`


    Description:


----



.. method:: KSChan.gate


    Syntax:
        :code:`ksgate = kschan.gate(index)`


    Description:

         

----



.. method:: KSChan.name


    Syntax:
        :code:`string = kschan.name()`

        :code:`string = kschan.name(string)`


    Description:


----



.. method:: KSChan.ion


    Syntax:
        :code:`string = kschan.ion()`

        :code:`string = kschan.ion(string)`


    Description:


----



.. method:: KSChan.ligand


    Syntax:
        :code:`string = kschan.ligand(index)`


    Description:

         

----



.. method:: KSChan.KSState


    Syntax:
        :code:`cannot be created directly`


    Description:
        A helper class for :func:`KSChan` . KSChan creates and destroys 
        these objects internally. It cannot be created directly 
        with the "new" keyword. An error message will be printed 
        if a hoc reference is used after KSChan has destroyed 
        the referenced KSState. 

    .. seealso::
        :meth:`KSChan.add_hhstate`, :meth:`KSChan.add_ksstate`

         

----



.. method:: KSChan.frac


    Syntax:
        :code:`val = ksstate.frac()`

        :code:`val = ksstate.frac(val)`


    Description:


----



.. method:: KSChan.index


    Syntax:
        :code:`index = ksstate.index()`


    Description:

         

----



.. method:: KSChan.gate


    Syntax:
        :code:`ksgate = ksstate.gate()`


    Description:

         

----



.. method:: KSChan.name


    Syntax:
        :code:`string = ksstate.name()`

        :code:`string = ksstate.name(string)`


    Description:

         

----



.. method:: KSChan.KSGate


    Syntax:
        :code:`cannot be created directly`


    Description:
        A helper class for :func:`KSChan` . KSChan creates and destroys 
        these objects internally. It cannot be created directly 
        with the "new" keyword. An error message will be printed 
        if a hoc reference is used after KSChan has destroyed 
        the referenced KSGate. 

    .. seealso::
        :meth:`KSChan.gate`

         

----



.. method:: KSChan.nstate


    Syntax:
        :code:`n = ksgate.nstate()`


    Description:


----



.. method:: KSChan.power


    Syntax:
        :code:`i = ksgate.power()`

        :code:`i = ksgate.power(i)`


    Description:


----



.. method:: KSChan.sindex


    Syntax:
        :code:`i = ksgate.sindex()`


    Description:


----



.. method:: KSChan.index


    Syntax:
        :code:`i = ksgate.index()`


    Description:

         

----



.. method:: KSChan.KSTrans


    Syntax:
        :code:`cannot be created directly`


    Description:
        A helper class for :func:`KSChan` . KSChan creates and destroys 
        these objects internally. It cannot be created directly 
        with the "new" keyword. An error message will be printed 
        if a hoc reference is used after KSChan has destroyed 
        the referenced KSTrans. 

    .. seealso::
        :meth:`KSChan.add_transition`, :meth:`KSChan.trans`

         

----



.. method:: KSChan.set_f


    Syntax:
        :code:`kstrans.set_f(direction, ftype, parmvec)`


    Description:


----



.. method:: KSChan.index


    Syntax:
        :code:`i = kstrans.index()`


    Description:


----



.. method:: KSChan.type


    Syntax:
        :code:`i = kstrans.type()`

        :code:`i = kstrans.type(i)`


    Description:


----



.. method:: KSChan.ftype


    Syntax:
        :code:`i = kstrans.ftype(direction)`


    Description:


----



.. method:: KSChan.ab


    Syntax:
        :code:`kstrans.ab(vvec, avec, bvec)`


    Description:


----



.. method:: KSChan.inftau


    Syntax:
        :code:`kstrans.inftau(vvec, infvec, tauvec)`


    Description:


----



.. method:: KSChan.f


    Syntax:
        :code:`val = kstrans.f(direction, v)`


    Description:

         

----



.. method:: KSChan.src


    Syntax:
        :code:`ksstate = kstrans.src()`


    Description:


----



.. method:: KSChan.target


    Syntax:
        :code:`ksstate = kstrans.target()`


    Description:


----



.. method:: KSChan.parm


    Syntax:
        :code:`parmvec = kstrans.parm(direction)`


    Description:

         

----



.. method:: KSChan.ligand


    Syntax:
        :code:`string = kstrans.ligand()`

        :code:`string = kstrans.ligand(string)`


    Description:

         

