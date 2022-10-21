
.. _hoc_kschan:

         
KSChan
------



.. hoc:class:: KSChan


    Syntax:
        ``kschan = new KSChan()``


    Description:
        Declare and manage a new density channel type which can 
        be instantiated in sections with the :ref:`insert <hoc_keyword_insert>`
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
        similar. The KSChan :hoc:meth:`KSChan.setstructure` method
        uses a slightly modified vector format so the old 
        Java channelbuilder tool will not work without 
        updating the Java implementation. 
         

----



.. hoc:method:: KSChan.setstructure


    Syntax:
        ``kschan.setstructure(vec)``


    Description:

         

----



.. hoc:method:: KSChan.remove_state


    Syntax:
        ``kschan.remove_state(index)``

        ``kschan.remove_state(ksstate)``


    Description:


----



.. hoc:method:: KSChan.remove_transition


    Syntax:
        ``kschan.remove_transition(index)``

        ``kschan.remove_transition(kstrans)``


    Description:

         

----



.. hoc:method:: KSChan.ngate


    Syntax:
        ``n = kschan.ngate()``


    Description:


----



.. hoc:method:: KSChan.nstate


    Syntax:
        ``n = kschan.nstate()``


    Description:


----



.. hoc:method:: KSChan.ntrans


    Syntax:
        ``n = kschan.ntrans()``


    Description:


----



.. hoc:method:: KSChan.nligand


    Syntax:
        ``n = kschan.nligand()``


    Description:


----



.. hoc:method:: KSChan.pr


    Syntax:
        ``kschan.pr()``


    Description:

         

----



.. hoc:method:: KSChan.iv_type


    Syntax:
        ``type = kschan.iv_type()``

        ``type = kschan.iv_type(type)``


    Description:


----



.. hoc:method:: KSChan.gmax


    Syntax:
        ``val = kschan.gmax()``

        ``val = kschan.gmax(val)``


    Description:


----



.. hoc:method:: KSChan.erev


    Syntax:
        ``val = kschan.erev()``

        ``val = kschan.erev(val)``


    Description:

         

----



.. hoc:method:: KSChan.add_hhstate


    Syntax:
        ``ksstate = kschan.add_hhstate(name)``


    Description:


----



.. hoc:method:: KSChan.add_ksstate


    Syntax:
        ``ksstate = kschan.add_ksstate(name)``


    Description:


----



.. hoc:method:: KSChan.add_transition


    Syntax:
        ``kstrans = kschan.add_transition(src_index, target_index)``

        ``kstrans = kschan.add_transition(src_ksstate, target_ksstate)``


    Description:


----



.. hoc:method:: KSChan.trans


    Syntax:
        ``kstrans = kschan.trans(index)``

        ``kstrans = kschan.trans(src_ksstate, target_ksstate)``


    Description:


----



.. hoc:method:: KSChan.state


    Syntax:
        ``ksstate = kschan.state(index)``


    Description:


----



.. hoc:method:: KSChan.gate


    Syntax:
        ``ksgate = kschan.gate(index)``


    Description:

         

----



.. hoc:method:: KSChan.name


    Syntax:
        ``string = kschan.name()``

        ``string = kschan.name(string)``


    Description:


----



.. hoc:method:: KSChan.ion


    Syntax:
        ``string = kschan.ion()``

        ``string = kschan.ion(string)``


    Description:


----



.. hoc:method:: KSChan.ligand


    Syntax:
        ``string = kschan.ligand(index)``


    Description:

         

----



.. hoc:class:: KSState


    Syntax:
        cannot be created directly


    Description:
        A helper class for :hoc:class:`KSChan`. KSChan creates and destroys
        these objects internally. It cannot be created directly 
        with the "new" keyword. An error message will be printed 
        if a hoc reference is used after KSChan has destroyed 
        the referenced KSState. 

    .. seealso::
        :hoc:meth:`KSChan.add_hhstate`, :hoc:meth:`KSChan.add_ksstate`

         

----



.. hoc:method:: KSState.frac


    Syntax:
        ``val = ksstate.frac()``

        ``val = ksstate.frac(val)``


    Description:


----



.. hoc:method:: KSState.index


    Syntax:
        ``index = ksstate.index()``


    Description:

         

----



.. hoc:method:: KSState.gate


    Syntax:
        ``ksgate = ksstate.gate()``


    Description:

         

----



.. hoc:method:: KSState.name


    Syntax:
        ``string = ksstate.name()``

        ``string = ksstate.name(string)``


    Description:

         

----



.. hoc:class:: KSGate


    Syntax:
        cannot be created directly


    Description:
        A helper class for :hoc:class:`KSChan`. KSChan creates and destroys
        these objects internally. It cannot be created directly 
        with the "new" keyword. An error message will be printed 
        if a hoc reference is used after KSChan has destroyed 
        the referenced KSGate. 

    .. seealso::
        :hoc:meth:`KSChan.gate`

         

----



.. hoc:method:: KSGate.nstate


    Syntax:
        ``n = ksgate.nstate()``


    Description:


----



.. hoc:method:: KSGate.power


    Syntax:
        ``i = ksgate.power()``

        ``i = ksgate.power(i)``


    Description:


----



.. hoc:method:: KSGate.sindex


    Syntax:
        ``i = ksgate.sindex()``


    Description:


----



.. hoc:method:: KSGate.index


    Syntax:
        ``i = ksgate.index()``


    Description:

         

----



.. hoc:class:: KSTrans


    Syntax:
        cannot be created directly


    Description:
        A helper class for :hoc:class:`KSChan`. KSChan creates and destroys
        these objects internally. It cannot be created directly 
        with the "new" keyword. An error message will be printed 
        if a hoc reference is used after KSChan has destroyed 
        the referenced KSTrans. 

    .. seealso::
        :hoc:meth:`KSChan.add_transition`, :hoc:meth:`KSChan.trans`

         

----



.. hoc:method:: KSTrans.set_f


    Syntax:
        ``kstrans.set_f(direction, ftype, parmvec)``


    Description:


----



.. hoc:method:: KSTrans.index


    Syntax:
        ``i = kstrans.index()``


    Description:


----



.. hoc:method:: KSTrans.type


    Syntax:
        ``i = kstrans.type()``

        ``i = kstrans.type(i)``


    Description:


----



.. hoc:method:: KSTrans.ftype


    Syntax:
        ``i = kstrans.ftype(direction)``


    Description:


----



.. hoc:method:: KSTrans.ab


    Syntax:
        ``kstrans.ab(vvec, avec, bvec)``


    Description:


----



.. hoc:method:: KSTrans.inftau


    Syntax:
        ``kstrans.inftau(vvec, infvec, tauvec)``


    Description:


----



.. hoc:method:: KSTrans.f


    Syntax:
        ``val = kstrans.f(direction, v)``


    Description:

         

----



.. hoc:method:: KSTrans.src


    Syntax:
        ``ksstate = kstrans.src()``


    Description:


----



.. hoc:method:: KSTrans.target


    Syntax:
        ``ksstate = kstrans.target()``


    Description:


----



.. hoc:method:: KSTrans.parm


    Syntax:
        ``parmvec = kstrans.parm(direction)``


    Description:

         

----



.. hoc:method:: KSTrans.ligand


    Syntax:
        ``string = kstrans.ligand()``

        ``string = kstrans.ligand(string)``


    Description:

         

