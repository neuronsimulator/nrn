.. _kschan:

         
KSChan
------



.. class:: KSChan

    .. tab:: Python
    
    
        Syntax:
            ``kschan = n.KSChan()``
        
            ``kschan = n.KSChan(is_PointProcess)``


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
         

    .. tab:: HOC


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
            similar. The KSChan :meth:`KSChan.setstructure` method
            uses a slightly modified vector format so the old 
            Java channelbuilder tool will not work without 
            updating the Java implementation. 
        
----



.. method:: KSChan.setstructure

    .. tab:: Python
    
    
        Syntax:
            ``kschan.setstructure(vec)``



         

    .. tab:: HOC


        Syntax:
            ``kschan.setstructure(vec)``
        
        
        Description:
        
----



.. method:: KSChan.remove_state

    .. tab:: Python
    
    
        Syntax:
            ``kschan.remove_state(index)``

            ``kschan.remove_state(ksstate)``




    .. tab:: HOC


        Syntax:
            ``kschan.remove_state(index)``
        
        
            ``kschan.remove_state(ksstate)``
        
        
        Description:
        
----



.. method:: KSChan.remove_transition

    .. tab:: Python
    
    
        Syntax:
            ``kschan.remove_transition(index)``

            ``kschan.remove_transition(kstrans)``



         

    .. tab:: HOC


        Syntax:
            ``kschan.remove_transition(index)``
        
        
            ``kschan.remove_transition(kstrans)``
        
        
        Description:
        
----



.. method:: KSChan.ngate

    .. tab:: Python
    
    
        Syntax:
            ``n = kschan.ngate()``




    .. tab:: HOC


        Syntax:
            ``n = kschan.ngate()``
        
        
        Description:
        
----



.. method:: KSChan.nstate

    .. tab:: Python
    
    
        Syntax:
            ``n = kschan.nstate()``



    .. tab:: HOC


        Syntax:
            ``n = kschan.nstate()``
        
        
        Description:
        
----



.. method:: KSChan.ntrans

    .. tab:: Python
    
    
        Syntax:
            ``n = kschan.ntrans()``




    .. tab:: HOC


        Syntax:
            ``n = kschan.ntrans()``
        
        
        Description:
        
----



.. method:: KSChan.nligand

    .. tab:: Python
    
    
        Syntax:
            ``n = kschan.nligand()``




    .. tab:: HOC


        Syntax:
            ``n = kschan.nligand()``
        
        
        Description:
        
----



.. method:: KSChan.pr

    .. tab:: Python
    
    
        Syntax:
            ``kschan.pr()``



         

    .. tab:: HOC


        Syntax:
            ``kschan.pr()``
        
        
        Description:
        
----



.. method:: KSChan.iv_type

    .. tab:: Python
    
    
        Syntax:
            ``type = kschan.iv_type()``

            ``type = kschan.iv_type(type)``



    .. tab:: HOC


        Syntax:
            ``type = kschan.iv_type()``
        
        
            ``type = kschan.iv_type(type)``
        
        
        Description:
        
----



.. method:: KSChan.gmax

    .. tab:: Python
    
    
        Syntax:
            ``val = kschan.gmax()``

            ``val = kschan.gmax(val)``




    .. tab:: HOC


        Syntax:
            ``val = kschan.gmax()``
        
        
            ``val = kschan.gmax(val)``
        
        
        Description:
        
----



.. method:: KSChan.erev

    .. tab:: Python
    
    
        Syntax:
            ``val = kschan.erev()``

            ``val = kschan.erev(val)``


         

    .. tab:: HOC


        Syntax:
            ``val = kschan.erev()``
        
        
            ``val = kschan.erev(val)``
        
        
        Description:
        
----



.. method:: KSChan.add_hhstate

    .. tab:: Python
    
    
        Syntax:
            ``ksstate = kschan.add_hhstate(name)``




    .. tab:: HOC


        Syntax:
            ``ksstate = kschan.add_hhstate(name)``
        
        
        Description:
        
----



.. method:: KSChan.add_ksstate

    .. tab:: Python
    
    
        Syntax:
            ``ksstate = kschan.add_ksstate(name)``



    .. tab:: HOC


        Syntax:
            ``ksstate = kschan.add_ksstate(name)``
        
        
        Description:
        
----



.. method:: KSChan.add_transition

    .. tab:: Python
    
    
        Syntax:
            ``kstrans = kschan.add_transition(src_index, target_index)``

            ``kstrans = kschan.add_transition(src_ksstate, target_ksstate)``




    .. tab:: HOC


        Syntax:
            ``kstrans = kschan.add_transition(src_index, target_index)``
        
        
            ``kstrans = kschan.add_transition(src_ksstate, target_ksstate)``
        
        
        Description:
        
----



.. method:: KSChan.trans

    .. tab:: Python
    
    
        Syntax:
            ``kstrans = kschan.trans(index)``

            ``kstrans = kschan.trans(src_ksstate, target_ksstate)``




    .. tab:: HOC


        Syntax:
            ``kstrans = kschan.trans(index)``
        
        
            ``kstrans = kschan.trans(src_ksstate, target_ksstate)``
        
        
        Description:
        
----



.. method:: KSChan.state

    .. tab:: Python
    
    
        Syntax:
            ``ksstate = kschan.state(index)``



    .. tab:: HOC


        Syntax:
            ``ksstate = kschan.state(index)``
        
        
        Description:
        
----



.. method:: KSChan.gate

    .. tab:: Python
    
    
        Syntax:
            ``ksgate = kschan.gate(index)``



         

    .. tab:: HOC


        Syntax:
            ``ksgate = kschan.gate(index)``
        
        
        Description:
        
----



.. method:: KSChan.name

    .. tab:: Python
    
    
        Syntax:
            ``string = kschan.name()``

            ``string = kschan.name(string)``




    .. tab:: HOC


        Syntax:
            ``string = kschan.name()``
        
        
            ``string = kschan.name(string)``
        
        
        Description:
        
----



.. method:: KSChan.ion

    .. tab:: Python
    
    
        Syntax:
            ``string = kschan.ion()``

            ``string = kschan.ion(string)``




    .. tab:: HOC


        Syntax:
            ``string = kschan.ion()``
        
        
            ``string = kschan.ion(string)``
        
        
        Description:
        
----



.. method:: KSChan.ligand

    .. tab:: Python
    
    
        Syntax:
            ``string = kschan.ligand(index)``



         

    .. tab:: HOC


        Syntax:
            ``string = kschan.ligand(index)``
        
        
        Description:
        
----



.. class:: KSState

    .. tab:: Python
    
    
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

         

    .. tab:: HOC


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

    .. tab:: Python
    
    
        Syntax:
            ``val = ksstate.frac()``

            ``val = ksstate.frac(val)``




    .. tab:: HOC


        Syntax:
            ``val = ksstate.frac()``
        
        
            ``val = ksstate.frac(val)``
        
        
        Description:
        
----



.. method:: KSState.index

    .. tab:: Python
    
    
        Syntax:
            ``index = ksstate.index()``



         

    .. tab:: HOC


        Syntax:
            ``index = ksstate.index()``
        
        
        Description:
        
----



.. method:: KSState.gate

    .. tab:: Python
    
    
        Syntax:
            ``ksgate = ksstate.gate()``



         

    .. tab:: HOC


        Syntax:
            ``ksgate = ksstate.gate()``
        
        
        Description:
        
----



.. method:: KSState.name

    .. tab:: Python
    
    
        Syntax:
            ``string = ksstate.name()``

            ``string = ksstate.name(string)``



         

    .. tab:: HOC


        Syntax:
            ``string = ksstate.name()``
        
        
            ``string = ksstate.name(string)``
        
        
        Description:
        
----



.. class:: KSGate

    .. tab:: Python
    
    
        Syntax:
            cannot be created directly


        Description:
            A helper class for :class:`KSChan`. KSChan creates and destroys 
            these objects internally. It cannot be created directly 
            with n.KSGate. An error message will be printed 
            if a reference is used after KSChan has destroyed 
            the referenced KSGate. 

        .. seealso::
            :meth:`KSChan.gate`

         

    .. tab:: HOC


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

    .. tab:: Python
    
    
        Syntax:
            ``n = ksgate.nstate()``




    .. tab:: HOC


        Syntax:
            ``n = ksgate.nstate()``
        
        
        Description:
        
----



.. method:: KSGate.power

    .. tab:: Python
    
    
        Syntax:
            ``i = ksgate.power()``

            ``i = ksgate.power(i)``



    .. tab:: HOC


        Syntax:
            ``i = ksgate.power()``
        
        
            ``i = ksgate.power(i)``
        
        
        Description:
        
----



.. method:: KSGate.sindex

    .. tab:: Python
    
    
        Syntax:
            ``i = ksgate.sindex()``




    .. tab:: HOC


        Syntax:
            ``i = ksgate.sindex()``
        
        
        Description:
        
----



.. method:: KSGate.index

    .. tab:: Python
    
    
        Syntax:
            ``i = ksgate.index()``


         

    .. tab:: HOC


        Syntax:
            ``i = ksgate.index()``
        
        
        Description:
        
----



.. class:: KSTrans

    .. tab:: Python
    
    
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

         

    .. tab:: HOC


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

    .. tab:: Python
    
    
        Syntax:
            ``kstrans.set_f(direction, ftype, parmvec)``




    .. tab:: HOC


        Syntax:
            ``kstrans.set_f(direction, ftype, parmvec)``
        
        
        Description:
        
----



.. method:: KSTrans.index

    .. tab:: Python
    
    
        Syntax:
            ``i = kstrans.index()``




    .. tab:: HOC


        Syntax:
            ``i = kstrans.index()``
        
        
        Description:
        
----



.. method:: KSTrans.type

    .. tab:: Python
    
    
        Syntax:
            ``i = kstrans.type()``

            ``i = kstrans.type(i)``




    .. tab:: HOC


        Syntax:
            ``i = kstrans.type()``
        
        
            ``i = kstrans.type(i)``
        
        
        Description:
        
----



.. method:: KSTrans.ftype

    .. tab:: Python
    
    
        Syntax:
            ``i = kstrans.ftype(direction)``




    .. tab:: HOC


        Syntax:
            ``i = kstrans.ftype(direction)``
        
        
        Description:
        
----



.. method:: KSTrans.ab

    .. tab:: Python
    
    
        Syntax:
            ``kstrans.ab(vvec, avec, bvec)``




    .. tab:: HOC


        Syntax:
            ``kstrans.ab(vvec, avec, bvec)``
        
        
        Description:
        
----



.. method:: KSTrans.inftau

    .. tab:: Python
    
    
        Syntax:
            ``kstrans.inftau(vvec, infvec, tauvec)``




    .. tab:: HOC


        Syntax:
            ``kstrans.inftau(vvec, infvec, tauvec)``
        
        
        Description:
        
----



.. method:: KSTrans.f

    .. tab:: Python
    
    
        Syntax:
            ``val = kstrans.f(direction, v)``


         

    .. tab:: HOC


        Syntax:
            ``val = kstrans.f(direction, v)``
        
        
        Description:
        
----



.. method:: KSTrans.src

    .. tab:: Python
    
    
        Syntax:
            ``ksstate = kstrans.src()``




    .. tab:: HOC


        Syntax:
            ``ksstate = kstrans.src()``
        
        
        Description:
        
----



.. method:: KSTrans.target

    .. tab:: Python
    
    
        Syntax:
            ``ksstate = kstrans.target()``




    .. tab:: HOC


        Syntax:
            ``ksstate = kstrans.target()``
        
        
        Description:
        
----



.. method:: KSTrans.parm

    .. tab:: Python
    
    
        Syntax:
            ``parmvec = kstrans.parm(direction)``



         

    .. tab:: HOC


        Syntax:
            ``parmvec = kstrans.parm(direction)``
        
        
        Description:
        
----



.. method:: KSTrans.ligand

    .. tab:: Python
    
    
        Syntax:
            ``string = kstrans.ligand()``

            ``string = kstrans.ligand(string)``



         

    .. tab:: HOC


        Syntax:
            ``string = kstrans.ligand()``
        
        
            ``string = kstrans.ligand(string)``
        
        
        Description:
        
