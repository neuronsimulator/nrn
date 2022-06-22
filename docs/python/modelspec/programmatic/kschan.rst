.. _kschan:

         
KSChan
------



.. class:: h.KSChan()


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
    channel builder gui in NEURON --- albeit substantially 
    similar. The KSChan :meth:`KSChan.setstructure` method 
    uses a slightly modified vector format so the old 
    Java channelbuilder tool will not work without 
    updating the Java implementation. 
         

----



.. method:: KSChan.setstructure(vec)




----



.. method:: KSChan.remove_state(index)
            KSChan.remove_state(ksstate)


 

----



.. method:: KSChan.remove_transition(index)
            KSChan.remove_transition(kstrans)


   

----



.. method:: KSChan.ngate()





----



.. method:: KSChan.nstate()


----



.. method:: KSChan.ntrans()


   

----



.. method:: KSChan.nligand()




----



.. method:: KSChan.pr()


 

----



.. method:: KSChan.iv_type()
            KSChan.iv_type(type)


 

----



.. method:: KSChan.gmax()
            KSChan.gmax(val)


   

----



.. method:: KSChan.erev()
            KSChan.erev(val)

   

----



.. method:: KSChan.add_hhstate(name)


   

----



.. method:: KSChan.add_ksstate(name)




----



.. method:: KSChan.add_transition(src_index, target_index)
            KSChan.add_transition(src_ksstate, target_ksstate)



----



.. method:: KSChan.trans(index)
            KSChan.trans(src_ksstate, target_ksstate)



----



.. method:: KSChan.state(index)


    



----



.. method:: KSChan.gate(index)


  

         

----



.. method:: KSChan.name()
            KSChan.name(string)

   


----



.. method:: KSChan.ion()
            KSChan.ion(string)

   



----



.. method:: KSChan.ligand(index)


 

         

----



.. class:: KSState


    A helper class for :class:`KSChan`. KSChan creates and destroys 
    these objects internally. It cannot be created directly 
    with the "new" keyword. An error message will be printed 
    if a reference is used after KSChan has destroyed 
    the referenced KSState. 

    .. seealso::
        :meth:`KSChan.add_hhstate`, :meth:`KSChan.add_ksstate`

         

----



.. method:: KSState.frac()
            KSState.frac(val)

   



----



.. method:: KSState.index()




         

----



.. method:: KSState.gate()


  
         

----



.. method:: KSState.name()
            KSState.name(string)

  

         

----



.. class:: KSGate


    A helper class for :class:`KSChan`. KSChan creates and destroys 
    these objects internally. It cannot be created directly 
    with h.KSGate. An error message will be printed 
    if a reference is used after KSChan has destroyed 
    the referenced KSGate. 

    .. seealso::
        :meth:`KSChan.gate`

         

----



.. method:: KSGate.nstate()


   



----



.. method:: KSGate.power()
            KSGate.power(i)

  

----



.. method:: KSGate.sindex()


 


----



.. method:: KSGate.index()


 

----



.. class:: KSTrans



    A helper class for :class:`KSChan`. KSChan creates and destroys 
    these objects internally. It cannot be created directly 
    by KSTrans. An error message will be printed 
    if a reference is used after KSChan has destroyed 
    the referenced KSTrans. 

    .. seealso::
        :meth:`KSChan.add_transition`, :meth:`KSChan.trans`

         

----



.. method:: KSTrans.set_f(direction, ftype, parmvec)


    


----



.. method:: KSTrans.index()


   




----



.. method:: KSTrans.type()
            KSTrans.type(i)

   


----



.. method:: KSTrans.ftype(direction)


   


----



.. method:: KSTrans.ab(vvec, avec, bvec)


  



----



.. method:: KSTrans.inftau(vvec, infvec, tauvec)


  


----



.. method:: KSTrans.f(direction, v)


   

         

----



.. method:: KSTrans.src()


   




----



.. method:: KSTrans.target()


 


----



.. method:: KSTrans.parm(direction)





         

----



.. method:: KSTrans.ligand()
            KSTrans.ligand(string)

   
         

