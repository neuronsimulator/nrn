.. _kschan:

         
KSChan
------



.. class:: KSChan


    Syntax:
        ``kschan = new KSChan()``


    Description:
        Declare and manage a new density channel type which can 
        be instantiated in sections with the :ref:`insert <keyword_insert>` 
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
        ``kschan.setstructure(vec)``


    Description:

         

----



.. method:: KSChan.remove_state


    Syntax:
        ``kschan.remove_state(index)``

        ``kschan.remove_state(ksstate)``


    Description:


----



.. method:: KSChan.remove_transition


    Syntax:
        ``kschan.remove_transition(index)``

        ``kschan.remove_transition(kstrans)``


    Description:

         

----



.. method:: KSChan.ngate


    Syntax:
        ``n = kschan.ngate()``


    Description:


----



.. method:: KSChan.nstate


    Syntax:
        ``n = kschan.nstate()``


    Description:


----



.. method:: KSChan.ntrans


    Syntax:
        ``n = kschan.ntrans()``


    Description:


----



.. method:: KSChan.nligand


    Syntax:
        ``n = kschan.nligand()``


    Description:


----



.. method:: KSChan.pr


    Syntax:
        ``kschan.pr()``


    Description:

         

----



.. method:: KSChan.iv_type


    Syntax:
        ``type = kschan.iv_type()``

        ``type = kschan.iv_type(type)``


    Description:


----



.. method:: KSChan.gmax


    Syntax:
        ``val = kschan.gmax()``

        ``val = kschan.gmax(val)``


    Description:


----



.. method:: KSChan.erev


    Syntax:
        ``val = kschan.erev()``

        ``val = kschan.erev(val)``


    Description:

         

----



.. method:: KSChan.add_hhstate


    Syntax:
        ``ksstate = kschan.add_hhstate(name)``


    Description:


----



.. method:: KSChan.add_ksstate


    Syntax:
        ``ksstate = kschan.add_ksstate(name)``


    Description:


----



.. method:: KSChan.add_transition


    Syntax:
        ``kstrans = kschan.add_transition(src_index, target_index)``

        ``kstrans = kschan.add_transition(src_ksstate, target_ksstate)``


    Description:


----



.. method:: KSChan.trans


    Syntax:
        ``kstrans = kschan.trans(index)``

        ``kstrans = kschan.trans(src_ksstate, target_ksstate)``


    Description:


----



.. method:: KSChan.state


    Syntax:
        ``ksstate = kschan.state(index)``


    Description:


----



.. method:: KSChan.gate


    Syntax:
        ``ksgate = kschan.gate(index)``


    Description:

         

----



.. method:: KSChan.name


    Syntax:
        ``string = kschan.name()``

        ``string = kschan.name(string)``


    Description:


----



.. method:: KSChan.ion


    Syntax:
        ``string = kschan.ion()``

        ``string = kschan.ion(string)``


    Description:


----



.. method:: KSChan.ligand


    Syntax:
        ``string = kschan.ligand(index)``


    Description:

         

----



.. class:: KSState


    Syntax:
        cannot be created directly


    Description:
        A helper class for :class:`KSChan`. KSChan creates and destroys 
        these objects internally. It cannot be created directly 
        with the "new" keyword. An error message will be printed 
        if a hoc reference is used after KSChan has destroyed 
        the referenced KSState. 

    .. seealso::
        :meth:`KSChan.add_hhstate`, :meth:`KSChan.add_ksstate`

         

----



.. method:: KSState.frac


    Syntax:
        ``val = ksstate.frac()``

        ``val = ksstate.frac(val)``


    Description:


----



.. method:: KSState.index


    Syntax:
        ``index = ksstate.index()``


    Description:

         

----



.. method:: KSState.gate


    Syntax:
        ``ksgate = ksstate.gate()``


    Description:

         

----



.. method:: KSState.name


    Syntax:
        ``string = ksstate.name()``

        ``string = ksstate.name(string)``


    Description:

         

----



.. class:: KSGate


    Syntax:
        cannot be created directly


    Description:
        A helper class for :class:`KSChan`. KSChan creates and destroys 
        these objects internally. It cannot be created directly 
        with the "new" keyword. An error message will be printed 
        if a hoc reference is used after KSChan has destroyed 
        the referenced KSGate. 

    .. seealso::
        :meth:`KSChan.gate`

         

----



.. method:: KSGate.nstate


    Syntax:
        ``n = ksgate.nstate()``


    Description:


----



.. method:: KSGate.power


    Syntax:
        ``i = ksgate.power()``

        ``i = ksgate.power(i)``


    Description:


----



.. method:: KSGate.sindex


    Syntax:
        ``i = ksgate.sindex()``


    Description:


----



.. method:: KSGate.index


    Syntax:
        ``i = ksgate.index()``


    Description:

         

----



.. class:: KSTrans


    Syntax:
        cannot be created directly


    Description:
        A helper class for :class:`KSChan`. KSChan creates and destroys 
        these objects internally. It cannot be created directly 
        with the "new" keyword. An error message will be printed 
        if a hoc reference is used after KSChan has destroyed 
        the referenced KSTrans. 

    .. seealso::
        :meth:`KSChan.add_transition`, :meth:`KSChan.trans`

         

----



.. method:: KSTrans.set_f


    Syntax:
        ``kstrans.set_f(direction, ftype, parmvec)``


    Description:


----



.. method:: KSTrans.index


    Syntax:
        ``i = kstrans.index()``


    Description:


----



.. method:: KSTrans.type


    Syntax:
        ``i = kstrans.type()``

        ``i = kstrans.type(i)``


    Description:


----



.. method:: KSTrans.ftype


    Syntax:
        ``i = kstrans.ftype(direction)``


    Description:


----



.. method:: KSTrans.ab


    Syntax:
        ``kstrans.ab(vvec, avec, bvec)``


    Description:


----



.. method:: KSTrans.inftau


    Syntax:
        ``kstrans.inftau(vvec, infvec, tauvec)``


    Description:


----



.. method:: KSTrans.f


    Syntax:
        ``val = kstrans.f(direction, v)``


    Description:

         

----



.. method:: KSTrans.src


    Syntax:
        ``ksstate = kstrans.src()``


    Description:


----



.. method:: KSTrans.target


    Syntax:
        ``ksstate = kstrans.target()``


    Description:


----



.. method:: KSTrans.parm


    Syntax:
        ``parmvec = kstrans.parm(direction)``


    Description:

         

----



.. method:: KSTrans.ligand


    Syntax:
        ``string = kstrans.ligand()``

        ``string = kstrans.ligand(string)``


    Description:

         

