.. _kschan:

         
KSChan
------



.. class:: KSChan


    Syntax:
        ``kschan = h.KSChan()``
        
        ``kschan = h.KSChan(is_PointProcess)``


    Description:
        Declare and manage a new density channel or PointProcess type.
        Density channels can be instantiated in sections with the :ref:`insert <keyword_insert>`
        statement. PointProcess channels are instantiated like :class:`IClamp`.  After the type comes into existence it
        is always a valid type and the conductance style,
        ligands, name, gating functions, etc can be changed
        at any time. However if an instance of the channel is currently in existance
        no change is allowed to the number of states or parameters of the channel.
        For example, if an instance of the channel exists, one cannot switch between single
        channel mode and continuous mode as the former has an extra range variable called Nsingle.
        (But when Nsingle is a parameter, a value  of 0 causes the channel to compute in continuous mode).
        The type cannot be destroyed.
         
        This is an extension of the KSChan managed by the
        Java catacomb channel builder tool
        for the past several
        years. The primary functional extension is the
        ability to define HH-style gates in addition to
        kinetic scheme gates. The administrative extensions
        allowed a more convenient re-implementation of the
        channel builder gui in NEURON --- albeit substantially
        similar. The KSChan :meth:`KSChan.setstructure` method
        uses a slightly modified vector format so the old
        Java channelbuilder tool will not work without
        updating the Java implementation.
         

----



.. method:: KSChan.setstructure


    Syntax:
        ``kschan.setstructure(vec)``



         

----



.. method:: KSChan.remove_state


    Syntax:
        ``kschan.remove_state(index)``

        ``kschan.remove_state(ksstate)``




----



.. method:: KSChan.remove_transition


    Syntax:
        ``kschan.remove_transition(index)``

        ``kschan.remove_transition(kstrans)``



         

----



.. method:: KSChan.ngate


    Syntax:
        ``n = kschan.ngate()``




----



.. method:: KSChan.nstate


    Syntax:
        ``n = kschan.nstate()``



----



.. method:: KSChan.ntrans


    Syntax:
        ``n = kschan.ntrans()``




----



.. method:: KSChan.nligand


    Syntax:
        ``n = kschan.nligand()``




----



.. method:: KSChan.pr


    Syntax:
        ``kschan.pr()``



         

----



.. method:: KSChan.iv_type


    Syntax:
        ``type = kschan.iv_type()``

        ``type = kschan.iv_type(type)``



----



.. method:: KSChan.gmax


    Syntax:
        ``val = kschan.gmax()``

        ``val = kschan.gmax(val)``




----



.. method:: KSChan.erev


    Syntax:
        ``val = kschan.erev()``

        ``val = kschan.erev(val)``


         

----



.. method:: KSChan.add_hhstate


    Syntax:
        ``ksstate = kschan.add_hhstate(name)``




----



.. method:: KSChan.add_ksstate


    Syntax:
        ``ksstate = kschan.add_ksstate(name)``



----



.. method:: KSChan.add_transition


    Syntax:
        ``kstrans = kschan.add_transition(src_index, target_index)``

        ``kstrans = kschan.add_transition(src_ksstate, target_ksstate)``




----



.. method:: KSChan.trans


    Syntax:
        ``kstrans = kschan.trans(index)``

        ``kstrans = kschan.trans(src_ksstate, target_ksstate)``




----



.. method:: KSChan.state


    Syntax:
        ``ksstate = kschan.state(index)``



----



.. method:: KSChan.gate


    Syntax:
        ``ksgate = kschan.gate(index)``



         

----



.. method:: KSChan.name


    Syntax:
        ``string = kschan.name()``

        ``string = kschan.name(string)``




----



.. method:: KSChan.ion


    Syntax:
        ``string = kschan.ion()``

        ``string = kschan.ion(string)``




----



.. method:: KSChan.ligand


    Syntax:
        ``string = kschan.ligand(index)``



         

----



.. class:: KSState


    Syntax:
        cannot be created directly


    Description:
        A helper class for :class:`KSChan`. KSChan creates and destroys 
        these objects internally. It cannot be created directly 
        with the "new" keyword. An error message will be printed 
        if a reference is used after KSChan has destroyed 
        the referenced KSState. 

    .. seealso::
        :meth:`KSChan.add_hhstate`, :meth:`KSChan.add_ksstate`

         

----



.. method:: KSState.frac


    Syntax:
        ``val = ksstate.frac()``

        ``val = ksstate.frac(val)``




----



.. method:: KSState.index


    Syntax:
        ``index = ksstate.index()``



         

----



.. method:: KSState.gate


    Syntax:
        ``ksgate = ksstate.gate()``



         

----



.. method:: KSState.name


    Syntax:
        ``string = ksstate.name()``

        ``string = ksstate.name(string)``



         

----



.. class:: KSGate


    Syntax:
        cannot be created directly


    Description:
        A helper class for :class:`KSChan`. KSChan creates and destroys 
        these objects internally. It cannot be created directly 
        with h.KSGate. An error message will be printed 
        if a reference is used after KSChan has destroyed 
        the referenced KSGate. 

    .. seealso::
        :meth:`KSChan.gate`

         

----



.. method:: KSGate.nstate


    Syntax:
        ``n = ksgate.nstate()``




----



.. method:: KSGate.power


    Syntax:
        ``i = ksgate.power()``

        ``i = ksgate.power(i)``



----



.. method:: KSGate.sindex


    Syntax:
        ``i = ksgate.sindex()``




----



.. method:: KSGate.index


    Syntax:
        ``i = ksgate.index()``


         

----



.. class:: KSTrans


    Syntax:
        cannot be created directly


    Description:
        A helper class for :class:`KSChan`. KSChan creates and destroys 
        these objects internally. It cannot be created directly 
        by KSTrans. An error message will be printed 
        if a reference is used after KSChan has destroyed 
        the referenced KSTrans. 

    .. seealso::
        :meth:`KSChan.add_transition`, :meth:`KSChan.trans`

         

----



.. method:: KSTrans.set_f


    Syntax:
        ``kstrans.set_f(direction, ftype, parmvec)``




----



.. method:: KSTrans.index


    Syntax:
        ``i = kstrans.index()``




----



.. method:: KSTrans.type


    Syntax:
        ``i = kstrans.type()``

        ``i = kstrans.type(i)``




----



.. method:: KSTrans.ftype


    Syntax:
        ``i = kstrans.ftype(direction)``




----



.. method:: KSTrans.ab


    Syntax:
        ``kstrans.ab(vvec, avec, bvec)``




----



.. method:: KSTrans.inftau


    Syntax:
        ``kstrans.inftau(vvec, infvec, tauvec)``




----



.. method:: KSTrans.f


    Syntax:
        ``val = kstrans.f(direction, v)``


         

----



.. method:: KSTrans.src


    Syntax:
        ``ksstate = kstrans.src()``




----



.. method:: KSTrans.target


    Syntax:
        ``ksstate = kstrans.target()``




----



.. method:: KSTrans.parm


    Syntax:
        ``parmvec = kstrans.parm(direction)``



         

----



.. method:: KSTrans.ligand


    Syntax:
        ``string = kstrans.ligand()``

        ``string = kstrans.ligand(string)``



         

