.. _linmod:

         
LinearMechanism
---------------



.. class:: LinearMechanism


    Syntax:
        :code:`lm = new LinearMechanism(c, g, y, [y0], b)`

        :code:`section lm = new LinearMechanism(c, g, y, [y0], b, x)`

        :code:`lm = new LinearMechanism(c, g, y, [y0], b, sl, xvec, [layervec])`



    Description:
        Adds linear equations to the tree matrix current balance equations. 
        I.e. the equations are solved 
        simultaneously with the current balance equations. 
        These equations may modify current balance equations and involve 
        membrane potentials as dependent variables. 
         
        The equations added are of the differential-algebraic form 
        c*dy/dt + g*y = b 
        with initial conditions specified by the optional y0 vector argument. 
        c and g must be square matrices of the same rank as the y and b vectors. 
        The implementation is more efficient if c is a sparse matrix since 
        at every time step c*y/dt must be computed. 
         
        When a LinearMechanism is created, all the potentially non-zero elements 
        for the c and g matrices must be actually non-zero so that 
        the mathematical topology of the matrices is known in advance. 
        After creation, elements can be set to 0 if desired. 
         
        The arguments after the b vector specify which voltages and current 
        balance equations are coupled to this system. The scalar form, x, with 
        a currently accessed section means that the first equation 
        is added to the current balance equation at this location and the first 
        dependent variable is a copy of the membrane potential. If the 
        system is coupled to more than one location, then  sl must be a SectionList 
        and xvec a Vector of relative positions (0 ... 1) specifying the 
        locations. In this case, the first xvec.size equations are added to the 
        corresponding current balance equations and the first xvec.size dependent 
        y variables are copies of the membrane potentials at this location. 
        If the optional layervec argument is present then the values must be 
        0, 1, or 2 (or up to however many layers are defined in src/nrnoc/options.h) 
        0 refers to the internal potential (equal to the membrane potential when 
        the extracellular mechanism is not inserted), and higher numbers refer 
        to the \ :code:`vext[layer-1]` layer (or ground if the extracellular mechanism is 
        not inserted). 
         
        If some y variables correspond to membrane potential, the corresponding 
        initial values in the y0 vector are ignored and the initial values come 
        from the values of v during the normal :func:`finitialize` call. If you change 
        the value of v after finitialize, then you should also change the 
        corresponding y values if the linear system involves derivatives of v. 
         
        Note that current balance equations of sections when 0 < x < 1 have dimensions 
        of milliamp/cm2 and positive terms are outward. Thus 
        c elements involving voltages in mV 
        have dimensions of 1000uF/cm2 (so a value of .001 corresponds to 
        1 uF/cm2), g elements have dimensions of S/cm2, and b elements have 
        dimensions of outward current in milliamp/cm2. The current balance 
        equations for the zero area nodes at the beginning and end 
        of a section ( x = 0 and x = 1 ) have terms with the dimensions of 
        nanoamps. Thus c elements involving voltages in mV have dimensions 
        of nF and g elements have dimensions of uS. 
         
        The existence of one or more LinearMechanism switches the gaussian elimination 
        solver to the general sparse linear equation solver written by 
        Kenneth S. Kundert and available from 
        <a href="//www.netlib.org/sparse/index.html">http://www.netlib.org/sparse/index.html</a> 
        Although, even with no added equations, the solving of m*x=b takes more 
        than twice as long as the original default solver, there is no restriction 
        to a tree topology. 

    Example:
        load_file("nrngui.hoc") 

        .. code-block::
            none

            create soma 
            soma { insert hh } 
             
            //ideal voltage clamp. 
            objref c, g, y, b, model 
            c = new Matrix(2,2,2) //sparse - no elements used 
            g = new Matrix(2,2) 
            y = new Vector(2) // y.x[1] is injected current 
            b = new Vector(2) 
            g.x[0][1] = -1 
            g.x[1][0] = 1 
            b.x[1] = 10 // voltage clamp level 
             
            soma model = new LinearMechanism(c, g, y, b, .5) 
             
            proc advance() { 
            	printf("t=%g v=%g y.x[1]=%g\n", t, soma.v(.5), y.x[1]) 
            	fadvance() 
            } 
            run() 


    .. warning::
        Does not work yet with cvode. 
        Does not allow changes to coupling locations. 
        Is not notified when matrices, vectors, or segments it depends on 
        disappear. 


