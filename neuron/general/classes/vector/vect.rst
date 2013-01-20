.. _vect:

         
Vector
------



.. class:: Vector

         
        This class was implemented by 

        .. code-block::
            none

            --------------------------- 
            Zach Mainen 
            Computational Neurobiology Laboratory 
            Salk Institute  
            10010 N. Torrey Pines Rd. 
            La Jolla, CA 92037 
            zach@salk.edu 
            ---------------------------- 

         
         

    Syntax:
        :code:`obj = new Vector()`

        :code:`obj = new Vector(size)`

        :code:`obj = new Vector(size, init)`


    Description:
        The vector class provides convenient functions for manipulating one-dimensional 
        arrays of numbers. An object created with this class can be thought of as 
        containing a \ :code:`double x[]` variable. Individual elements of this array can 
        be manipulated with the normal *objref*\ :code:`.x[index]` notation. 
        Most of the Vector functions apply their operations to each element of the 
        x array thus avoiding the often tedious scaffolding required by an otherwise 
        un-encapsulated double array. 
         
        A vector can be created with length *size* and with each element set 
        to the value of *init*. 
         
        Vector methods that modify the elements are generally of the form 

        .. code-block::
            none

            obj = vsrcdest.method(...) 

        in which the values of vsrcdest on entry to the 
        method are used as source values by the method to compute values which replace 
        the old values in vsrcdest and the original vsrcdest object reference is 
        the return value of the method. For example, v1 = v2 + v3 would be 
        written, 

        .. code-block::
            none

            v1 = v2.add(v3) 

        However, this results in two, often serious, side effects. First, 
        the v2 elements are changed and so the original values are lost. Furthermore 
        v1 at the end is a reference to the same Vector object pointed to by v2. 
        That is, if you subsequently change the elements of v2, the elements 
        of v1 will change as well since v1 and v2 are in fact labels for the same object. 
         
        When these side effects need to be avoided, one uses the Vector.c function 
        which returns a 
        reference to a completely new Vector which is an identical copy. ie. 

        .. code-block::
            none

            	v1 = v2.c.add(v3) 

        leaves v2 unchanged, and v1 points to a completely new Vector. 
        One can build up elaborate vector expressions in this manner, ie 
        v1 = v2*s2 + v3*s3 + v4*s4could be written 

        .. code-block::
            none

            	v1 = v2.c.mul(s2).add(v3.c.mul(s3)).add(v4.c.mul(s4)) 

        but if the expressions get too complex it is probably clearer to employ 
        temporary objects to break the process into several separate expressions. 
         

    Example:

        .. code-block::
            none

            \ :code:`objref vec` 
            \ :code:`vec = new Vector(20,5)` 

        will create a vector with 20 indices, each having the value of 5. 

        .. code-block::
            none

            \ :code:`objref vec1` 
            \ :code:`vec1 = new Vector()` 

        will create a vector with 1 index which has value of 0. It is seldom 
        necessary to specify a size for a new vector since most operations, if necessary, 
        increase or decrease the number of available elements as needed. 
         

    .. seealso::
        :ref:`double <keyword_double>`,	x#Vector, :meth:`Vector.resize`

         

----



.. method:: Vector.x


    Syntax:
        :code:`vec.x[index]`


    Description:
        Elements of a vector can be accessed with \ :code:`vec.x[index]` notation. 
        Vector indices range from 0 to Vector.size()-1. 
        This 
        notation is superior to the older \ :code:`vec.get()` and \ :code:`vec.set()` notations for 
        three reasons: 
        <ol> 
        <li>It performs the roles of both 
        \ :code:`vec.get` and \ :code:`vec.set` with a syntax that is consistent with the normal 
        syntax for a \ :code:`double` array inside of an object. 
        <li> It can be viewed by a field editor (since it can appear on the left hand 
        side of an assignment statement). 
        <li> You can take its  address for functions which require that kind of argument. 
        </ol> 

    Example:
        \ :code:`print vec.x[0]` prints the value of the 0th (first) element. 
         
        \ :code:`vec.x[i] = 3` sets the i'th element to 3. 
         

        .. code-block::
            none

            xpanel("show a field editor") 
            xvalue("vec.x[3]") 
            xpvalue("last element", &vec.x[vec.size() - 1]) 
            xpanel() 

        Note, however, that there is a potential difficulty with the :func:`xpvalue` field 
        editor since, if vec is ever resized, then the pointer will be invalid. In 
        this case, the field editor will display the string, "Free'd". 

    .. warning::
        \ :code:`vec.x[-1]` returns the value of the first element of the vector, just as 
        would \ :code:`vec.x[0]`. 
         
        \ :code:`vec.x(i)` returns the value of index *i* just as does \ :code:`vec.x[i]`. 

         

----



.. method:: Vector.size


    Syntax:
        :code:`size = vec.size()`


    Description:
        Return the number of elements in the vector. The last element has the index: 
        \ :code:`vec.size() - 1`. Most explicit for loops over a vector can take the form: 

        .. code-block::
            none

            for i=0, vec.size()-1 {... vec.x[i] ...} 

        Note: There is a distinction between the size of a vector and the 
        amount of memory allocated to hold the vector. Generally, memory is only 
        freed and reallocated if the size needed is greater than the memory storage 
        previously allocated to the vector. Thus the memory used by vectors 
        tends to grow but not shrink. To reduce the memory used by a vector, one 
        can explicitly call :func:`buffer_size` . 

         

----



.. method:: Vector.resize


    Syntax:
        :code:`obj = vsrcdest.resize(new_size)`


    Description:
        Resize the vector.  If the vector is made smaller, then trailing elements 
        will be deleted.  If it is expanded, new elements will be initialized to 0 
        and original elements will remain unchanged. 
         
        Warning: Any function that 
        resizes the vector to a larger size than its available space 
        will make existing pointers to the elements invalid 
        (see note in :meth:`Vector.size` ). 
        For example, resizing vectors that have been plotted will remove that vector 
        from the plot list. Other functions may not be so forgiving and result in 
        a memory error (segmentation violation or unhandled exception). 

    Example:

        .. code-block::
            none

            objref vec 
            vec = new Vector(20,5) 
            vec.resize(30) 
            @endcode appends 10 elements, each having a value of 0, to \ :code:`vec`. 

            .. code-block::
                none

            \ :code:`vec.resize(10)` 

        removes the last 20 elements from the  \ :code:`vec`.The values of the first 
        10 elements are unchanged. 

    .. seealso::
        :meth:`Vector.buffer_size`

         

----



.. method:: Vector.buffer_size


    Syntax:
        :code:`space = vsrc.buffer_size()`

        :code:`space = vsrc.buffer_size(request)`


    Description:
        Returns the length of the double precision array memory allocated to hold the 
        vector. This is NOT the size of the vector. The vector size can efficiently 
        grow up to this value without reallocating memory. 
         
        With an argument, frees the old memory space and allocates new 
        memory space for the vector, copying old element values to the new elements. 
        If the request is less than the size, the size is truncated to the request. 
        For vectors that grow continuously, it may be more efficient to 
        allocate enough space at the outset, or else occasionally change the 
        buffer_size by larger chunks. It is not necessary to worry about the 
        efficiency of growth during a Vector.record since the space available 
        automatically increases by doubling. 

    Example:

        .. code-block::
            none

            objref y 
            y = new Vector(10) 
            y.size() 
            y.buffer_size() 
            y.resize(5) 
            y.size 
            y.buffer_size() 
            y.buffer_size(100) 
            y.size() 


         

----



.. method:: Vector.get


    Syntax:
        :code:`x = vec.get(index)`


    Description:
        Return the value of a vector element index.  This function 
        is superseded by the \ :code:`vec.x[]` notation but is retained for backward 
        compatibility. 

         

----



.. method:: Vector.set


    Syntax:
        :code:`obj = vsrcdest.set(index,value)`


    Description:
        Set vector element index to value.  This function is superseded by 
        the \ :code:`vec.x[i] = expr` notation but is retained for backward 
        compatibility. 

         
         

----



.. method:: Vector.fill


    Syntax:
        :code:`obj = vsrcdest.fil(value)`

        :code:`obj = vsrcdest.fill(value, start, end)`


    Description:
        The first form assigns *value* to every element in vsrcdest. 
         
        If *start* and 
        *end* arguments are present, they specify the index range for the assignment. 

    Example:

        .. code-block::
            none

            objref vec 
            vec = new Vector(20,5) 
            vec.fill(9,2,7) 

        assigns 9 to vec.x[2] through vec.x[7] 
        (a total of 6 elements) 

    .. seealso::
        :meth:`Vector.indgen`, :meth:`Vector.append`

         

----



.. method:: Vector.label


    Syntax:
        :code:`strdef s`

        :code:`s = vec.label()`

        :code:`s = vec.label(s)`


    Description:
        Label the vector with a string. 
        The return value is the label, which is an empty string if there is no label. 
        Labels are printed on a Graph when the :meth:`Graph.plot` method is called. 

    Example:

        .. code-block::
            none

            objref vec 
            vec = new Vector() 
            print vec.label() 
            vec.label("hello") 
            print vec.label() 


    .. seealso::
        :meth:`Graph.family`, :meth:`Graph.beginline`


----



.. method:: Vector.record


    Syntax:
        :code:`vdest.record(&var)`

        :code:`vdest.record(&var, Dt)`

        :code:`vdest.record(&var, tvec)`

        :code:`vdest.record(point_process_object, &varvar, ...)`


    Description:
        Save the stream of values of "*var*" during a simulation into the vdest vector. 
        Previous record and play specifications of this Vector (if any) 
        are destroyed. 
         
        Details: 
        Transfers take place on exit from \ :code:`finitialize()` and on exit from \ :code:`fadvance()`. 
        At the end of \ :code:`finitialize()`, \ :code:`v.x[0] = var`. At the end of \ :code:`fadvance`, 
        *var* will be saved if \ :code:`t` (after being incremented by \ :code:`fadvance`) 
        is equal or greater than the associated time of the 
        next index. The system maintains a set of record vectors and the vector will 
        be removed from the list if the vector or var is destroyed. 
        The vector is automatically increased in size by 100 elements at a time 
        if more space is required, so efficiency will be slightly improved if one 
        creates vectors with sufficient size to hold the entire stream, and plots will 
        be more persistent (recall that resizing may cause reallocation of memory 
        to hold elements and this will make pointers invalid). 
         
        The record semantics can be thought of as:
 
        \ :code:`var(t) -> v.x[index]` 
         
        The default relationship between \ :code:`index` and 
        \ :code:`t` is \ :code:`t = index*dt`. 
 
        In the second form, \ :code:`t = index*Dt`. 
 
        In the third form, \ :code:`t = tvec.x[index]`. 
         
        For the local variable timestep method, :meth:`CVode.use_local_dt` and/or multiple 
        threads, :meth:`ParallelContext.nthread` , it is 
        often helpful to provide specific information about which cell the 
        *var* pointer is associated with by inserting as the first arg some POINT_PROCESS 
        object which is located on the cell. This is necessary if the pointer is not 
        a RANGE variable and is much more efficient if it is. The fixed step and global 
        variable time step method do not need or use this information for the 
        local step method but will use it for multiple threads. It is therefore 
        a good idea to supply it if possible. 

    .. warning::
        record/play behavior is reasonable but surprising if \ :code:`dt` is greater than 
        \ :code:`Dt`. Things work best if \ :code:`Dt` happens to be a multiple of \ :code:`dt`. All combinations 
        of record ; play ; \ :code:`Dt =>< dt ` ; and tvec sequences 
        have not been tested. 

    Example:
        See tests/nrniv/vrecord.hoc for examples of usage. 
         
        If one is using the graphical interface generated by "Standard Run Library" 
        to simulate a neuron containing a "terminal" section, Then one can store 
        the time course of the terminal voltage (between runs) with: 

        .. code-block::
            none

            objref dv 
            dv = new Vector() 
            dv.record(&terminal.v(.5)) 
            init()	// or push the "Init and Run" button on the control panel 
            run() 

        Note that the next "run" will overwrite the previous time course stored 
        in the vector. Thus dv should be copied to another vector ( see :func:`copy` ). 
        To remove 
        dv from the list of record vectors, the easiest method is to destroy the instance 
        with 
        \ :code:`dv = new Vector()` 

    .. seealso::
        :meth:`Vector.finitialize`, :func:`fadvance`, :func:`play`, :meth:`Vector.t`, :func:`play_remove`

         

----



.. method:: Vector.play


    Syntax:
        :code:`vsrc.play(&var, Dt)`

        :code:`vsrc.play(&var, tvec)`

        :code:`vsrc.play("stmt involving $1", optional Dt or tvec arg)`

        :code:`vsrc.play(index)`

        :code:`vsrc.play(&var or stmt, Dt or Tvec, continuous)`

        :code:`vsrc.play(&var or stmt, tvec, indices_of_discontinuities_vector)`

        :code:`vsrc.play(point_process_object, &var, ...)`


    Description:
        The \ :code:`vsrc` vector values are assigned to the "*var*" variable during 
        a simulation. 
         
        The same vector can be played into different variables. 
         
        If the "stmt involving $1" form is used, that statement is executed with 
        the appropriate value of the $1 arg. This is not as efficient as the 
        pointer form but is useful for playing a value into a set of variables 
        as in 

        .. code-block::
            none

            forall g_pas = $1 

         
        The index form immediately sets the var (or executes the stmt) with the 
        value of vsrc.x[index] 
         
        The play semantics can be thought of as 
        \ :code:`v.x[index] -> var(t)` where t(index) is Dt*index or tvec.x[index] 
        The discrete event delivery system is used to determine the precise 
        time at which values are copied from vsrc to var. Note that for variable 
        step methods, unless continuity is specifically requested, the function 
        is a step function. Also, for the local variable dt method, var MUST be 
        associated with the cell that contains the currently accessed section 
        (but see the paragraph below about the use of a point_process_object 
        inserted as the first arg). 
         
        For the fixed step method 
        transfers take place on entry to \ :code:`finitialize()` and  on entry to \ :code:`fadvance()`. 
        At the beginning of \ :code:`finitialize()`, \ :code:`var = v.x[0]`. On \ :code:`fadvance` a transfer will 
        take place if t will be (after the \ :code:`fadvance` increment) equal 
        or greater than the associated time of the next index. For the variable step 
        methods, transfers take place exactly at the times specified by the Dt 
        or tvec arguments. 
         
        The system maintains a set of play vectors and the vector will be removed 
        from the list if the vector or var is destroyed. 
        If the end of the vector is reached, no further transfers are made (\ :code:`var` becomes 
        constant) 
         
        Note well: for the fixed step method, 
        if \ :code:`fadvance` exits with time equal to \ :code:`t` (ie enters at time t-dt), 
        then on entry to \ :code:`fadvance`, *var* is set equal to the value of 
        the vector at the index 
        appropriate to time t. Execute tests/nrniv/vrecord.hoc to see what this implies 
        during a simulation. ie the value of var from \ :code:`t-dt` to t played into by 
        a vector is equal to the value of the vector at \ :code:`index(t)`. If the vector 
        was meant to serve as a continuous stimulus function, this results in 
        a first order correct simulation with respect to dt. If a second order correct 
        simulation is desired, it is necessary (though perhaps not sufficient since 
        all other equations in the system must also be solved using methods at least 
        second order correct) to fill the vector with function values at f((i-.5)*dt). 
         
        When continuous is 1 then linear interpolation is used to define the values 
        between time points. However, events at each Dt or tvec are still used 
        and that has beneficial performance implications for variable step methods 
        since vsrc is equivalent to a piecewise linear function and variable step 
        methods can excessively reduce dt as one approaches a discontinuity in 
        the first derivative. Note that if there are discontinuities in the 
        function itself, then tvec should have adjacent elements with the same 
        time value. As of version 6.2, when a value is greater than the range of 
        the t vector, linear extrapolation of the last two points is used 
        instead of a constant last value. If a constant outside the range 
        is desired, make sure the last two points have the same y value and 
        have different t values (if the last two values are at the same time, 
        the constant average will be returned). 
        (note: the 6.2 change allows greater variable time step efficiency 
        as one approaches discontinuities.) 
         
        The indices_of_discontinuities_vector argument is used to 
        specifying the indices in tvec of the times at which discrete events should 
        be used to notify that a discontinuity in the function, or any derivative 
        of the function, occurs. Presently, linear interpolation is used to 
        determine var(t) in the interval between these discontinuities (instead of 
        cubic spline) so the length of steps used by variable step methods near 
        the breakpoints depends on the details of how the parameter being played 
        into affects the states. 
         
        For the local variable timestep method, :meth:`CVode.use_local_dt` and/or multiple 
        threads, :meth:`ParallelContext.nthread` , it is 
        often helpful to provide specific information about which cell the 
        *var* pointer is associated with by inserting as the first arg some POINT_PROCESS 
        object which is located on the cell. This is necessary if the pointer is not 
        a RANGE variable and is much more efficient if it is. The fixed step and global 
        variable time step method do not need or use this information for the 
        local step method but will use it for multiple threads. It is therefore 
        a good idea to supply it if possible. 
         

    .. seealso::
        :meth:`Vector.record`, :meth:`Vector.play_remove`

         

----



.. method:: Vector.play_remove


    Syntax:
        :code:`v.play_remove()`


    Description:
        Removes the vector from BOTH record and play lists. 
        Note that the vector is automatically removed if 
        the variable which is recorded or played is destroyed 
        or if the vector is destroyed. 
        This function is used in those 
        cases where one wishes to keep the vector data even under subsequent runs. 
         
        record and play have been implemented by Michael Hines. 
         

    .. seealso::
        :meth:`Vector.record`, :meth:`Vector.play`

         

----



.. method:: Vector.indgen


    Syntax:
        :code:`obj = vsrcdest.indgen()`

        :code:`obj = vsrcdest.indgen(stepsize)`

        :code:`obj = vsrcdest.indgen(start,stepsize)`

        :code:`obj = vsrcdest.indgen(start,stop,stepsize)`


    Description:
        Fill the elements of a vector with a sequence of values.  With no 
        arguments, the sequence is integers from 0 to (size-1). 
         
        With only *stepsize* passed, the sequence goes from 0 to 
        *stepsize**(size-1) 
        in steps of *stepsize*.  *Stepsize* does not have to be an integer. 
         
        With *start*, *stop* and *stepsize*, 
        the vector is resized to be 1 + (*stop* - $varstart)/*stepsize* long and the sequence goes from 
        *start* up to and including *stop* in increments of *stepsize*. 

    Example:

        .. code-block::
            none

            objref vec 
            vec = new Vector(100) 
            vec.indgen(5) 

        creates a vector with 100 elements going from 0 to 495 in increments of 5. 

        .. code-block::
            none

            vec.indgen(50, 100, 10) 

        reduces the vector to 6 elements going from 50 to 100 in increments of 10. 

        .. code-block::
            none

            vec.indgen(90, 1000, 30) 

        expands the vector to 31 elements going from 90 to 990 in increments of 30. 

    .. seealso::
        :meth:`Vector.fill`, :meth:`Vector.append`

         

----



.. method:: Vector.append


    Syntax:
        :code:`obj = vsrcdest.append(vec1, vec2, ...)`


    Description:
        Concatenate values onto the end of a vector. 
        The arguments may be either scalars or vectors. 
        The values are appended to the end of the \ :code:`vsrcdest` vector. 

    Example:

        .. code-block::
            none

            objref vec, vec1, vec2 
            vec = new Vector (10,4) 
            vec1 = new Vector (10,5) 
            vec2 = new Vector (10,6) 
            vec.append(vec1, vec2, 7, 8, 9) 

        turns \ :code:`vec` into a 33 element vector, whose first ten elements = 4, whose 
        second ten elements = 5, whose third ten elements = 6, and whose 31st, 32nd, 
        and 33rd elements = 7, 8, and 9, respectively. 
        Remember, index 32 refers to the 33rd element. 

         

----



.. method:: Vector.insrt


    Syntax:
        :code:`obj = vsrcdest.insrt(index, vec1, vec2, ...)`


    Description:
        Inserts values before the index element. 
        The arguments may be either scalars or vectors. 
         
        \ :code:`obj.insrt(obj.size, ...)` is equivalent to \ :code:`obj.append(...)` 

         

----



.. method:: Vector.remove


    Syntax:
        :code:`obj = vsrcdest.remove(index)`

        :code:`obj = vsrcdest.remove(start, end)`


    Description:
        Remove the indexed element (or inclusive range) from the vector. 
        The vector is resized. 

         

----



.. method:: Vector.contains


    Syntax:
        :code:`boolean = vsrc.contains(value)`


    Description:
        Return whether or not 
        the vector contains *value* as at least one 
        of its elements (to within :func:`float_epsilon` ). A return value of 1 signifies true; 0 signifies false. 

    Example:

        .. code-block::
            none

            vec = new Vector (10) 
            vec.indgen(5) 
            vec.contains(30) 

        returns a 1, meaning the vector does contain an element whose value is 30. 

        .. code-block::
            none

            vec.contains(50) 

        returns a 0.  The vector does not contain an element whose value is 50. 

         

----



.. method:: Vector.copy


    Syntax:
        :code:`obj = vdest.copy(vsrc)`

        :code:`obj = vdest.copy(vsrc, dest_start)`

        :code:`obj = vdest.copy(vsrc, src_start, src_end)`

        :code:`obj = vdest.copy(vsrc, dest_start, src_start, src_end)`

        :code:`obj = vdest.copy(vsrc, dest_start, src_start, src_end, dest_inc, src_inc)`

        :code:`obj = vdest.copy(vsrc, vsrcdestindex)`

        :code:`obj = vdest.copy(vsrc, vsrcindex, vdestindex)`


    Description:
        Copies some or all of *vsrc* into *vdest*. 
        If the dest_start argument is present (an integer index), 
        source elements (beginning at *src*\ :code:`.x[0]`) 
        are copied to  *vdest* beginning at *dest*\ :code:`.x[dest_start]`, 
        *Src_start* and *src_end* here refer to indices of *vsrcx*, 
        not *vdest*.  If *vdest* is too small for the size required by *vsrc* and the 
        arguments, then it is resized to hold the data. 
        If the *dest* is larger than required AND there is more than one 
        argument the *dest* is NOT resized. 
        One may use -1 for the 
        src_end argument to specify the entire size (instead of the 
        tedious \ :code:`src.size()-1`) 
         
        If the second (and third) argument is a vector, 
        the elements of that vector are the 
        indices of the vsrc to be copied to the same indices of the vdest. 
        In this case the vdest is not resized and any indices that are out of 
        range of either vsrc or vdest are ignored. This function allows mapping 
        of a subset of a source vector into the subset of a destination vector. 
         
        This function can be slightly more efficient than :func:`c` since 
        if vdest contains enough space, memory will not have to 
        be allocated for it. Also it is convenient for those cases 
        in which vdest is being plotted and therefore reallocation 
        of memory (with consequent removal of vdest from the Graph) 
        is to be explicitly avoided. 

    Example:
        To copy the odd elements use:
 
        objref v1, v2 
        v1 = new Vector(30) 
        v1.indgen() 
        v1.printf() 
        @code... 
        v2 = new Vector() 
        v2.copy(v1, 0, 1, -1, 1, 2) 
        v2.printf() 

        To merge or shuffle two vectors into a third, use:
 
        objref v1, v2, v3 
        v1 = new Vector(15) 
        v1.indgen() 
        v1.printf() 
        v2 = new Vector(15) 
        v2.indgen(10) 
        v2.printf() 
        @code... 
        v3 = new Vector() 
        v3.copy(v1, 0, 0, -1, 2, 1) 
        v3.copy(v2, 1, 0, -1, 2, 1) 
        v3.printf 


    Example:

        .. code-block::
            none

            vec = new Vector(100,10) 
            vec1 = new Vector() 
            vec1.indgen(5,105,10) 
            vec.copy(vec1, 50, 3, 6) 

        turns \ :code:`vec` from a 100 element into a 54 element vector. 
        The first 50 elements will each have the value 10 and the last four will 
        have the values 35, 45, 55, and 65 respectively. 

    .. warning::
        Vectors copied to themselves are not usually what is expected. eg. 

        .. code-block::
            none

            vec = new Vector(20) 
            vec.indgen() 
            vec.copy(vec, 10) 

        produces  a 30 element vector cycling three times from 0 to 9. However 
        the self copy may work if the src index is always greater than or equal 
        to the destination index. 

         

----



.. method:: Vector.c


    Syntax:
        :code:`newvec = vsrc.c`

        :code:`newvec = vsrc.c(srcstart)`

        :code:`newvec = vsrc.c(srcstart, srcend)`


    Description:
        Return a new vector which is a copy of the vsrc vector, but does not copy 
        the label. For a complete copy including the label use :meth:`Vector.cl` . 
        (Identical to the :meth:`Vector.at` function but has a short name that suggests 
        copy or clone). Useful in the construction of filter chains. 
        Note that with no arguments, it is not necessary to type the 
        parentheses. 
         

         

----



.. method:: Vector.cl


    Syntax:
        :code:`newvec = vsrc.cl`

        :code:`newvec = vsrc.cl(srcstart)`

        :code:`newvec = vsrc.cl(srcstart, srcend)`


    Description:
        Return a new vector which is a copy, including the label, of the vsrc vector. 
        (Similar to the :meth:`Vector.c` function which does not copy the label) 
        Useful in the construction of filter chains. 
        Note that with no arguments, it is not necessary to type the 
        parentheses. 

         

----



.. method:: Vector.at


    Syntax:
        :code:`newvec = vsrc.at()`

        :code:`newvec = vsrc.at(start)`

        :code:`newvec = vsrc.at(start,end)`


    Description:
        Return a new vector consisting of all or part of another. 
         
        This function predates the introduction of the vsrc.c, "clone", function 
        which is synonymous but is retained for backward compatibility. 
         
        It merely avoids the necessity of a \ :code:`vdest = new Vector()` command and 
        is equivalent to 

        .. code-block::
            none

            vdest = new Vector() 
            vdest.copy(vsrc, start, end) 


    Example:

        .. code-block::
            none

            objref vec, vec1 
            vec = new Vector() 
            vec.indgen(10,50,2) 
            vec1 = vec.at(2, 10) 

        creates \ :code:`vec1` with 9 elements which correspond to the values at indices 
        2 - 10 in \ :code:`vec`.  The contents of \ :code:`vec1` would then be, in order: 14, 16, 18, 
        20, 22, 24, 26, 28, 30. 

         

----



.. method:: Vector.from_double


    Syntax:
        :code:`double px[n]`

        :code:`obj = vdest.from_double(n, &px)`


    Description:
        Resizes the vector to size n and copies the values from the double array 
        to the vector. 


----



.. method:: Vector.where


    Syntax:
        :code:`obj = vdest.where(vsource, opstring, value1)`

        :code:`obj = vdest.where(vsource, op2string, value1, value2)`

        :code:`obj = vsrcdest.where(opstring, value1)`

        :code:`obj = vsrcdest.where(op2string, value1, value2)`


    Description:
        \ :code:`vdest` is vector consisting of those elements of the given vector, \ :code:`vsource` 
        that match the condition opstring. 
         
        Opstring is a string matching one of these (all comparisons 
        are with respect to :func:`float_epsilon` ): 

        .. code-block::
            none

             \ :code:`==`		\ :code:`!=` 
             \ :code:`>`		\ :code:`<` 
             \ :code:`>=`		\ :code:`<=` 

        Op2string requires two numbers defining open/closed ranges and matches one 
        of these: 

        .. code-block::
            none

             \ :code:`[]`		\ :code:`[)` 
             \ :code:`(]`		\ :code:`()` 

         

    Example:

        .. code-block::
            none

            vec = new Vector(25) 
            vec1 = new Vector() 
            vec.indgen(10) 
            vec1.where(vec, ">=", 50) 

        creates \ :code:`vec1` with 20 elements ranging in value from 50 to 240 in 
        increments of 10. 

        .. code-block::
            none

            objref r 
            r = new Random() 
            vec = new Vector(25) 
            vec1 = new Vector() 
            r.uniform(10,20) 
            vec.fill(r) 
            vec1.where(vec, ">", 15) 

        creates \ :code:`vec1` with random elements gotten from \ :code:`vec` which have values 
        greater than 15.  The new elements in vec1 will be ordered 
        according to the order of their appearance in \ :code:`vec`. 

    .. seealso::
        :meth:`Vector.indvwhere`, :meth:`Vector.indwhere`

         

----



.. method:: Vector.indwhere


    .. seealso::
        :meth:`Vector.indvwhere`

         

----



.. method:: Vector.indvwhere


    Syntax:
        :code:`i = vsrc.indwhere(opstring, value)`

        :code:`i = vsrc.indwhere(op2string, low, high)`


        :code:`obj = vsrcdest.indvwhere(opstring,value)`

        :code:`obj = vsrcdest.indvwhere(opstring,value)`

        :code:`obj = vdest.indvwhere(vsource,op2string,low, high)`

        :code:`obj = vdest.indvwhere(vsource,op2string,low, high)`


    Description:
        The  i = vsrc form returns the index of the first element of v matching 
        the criterion given by the opstring. If there is no match, the return value 
        is -1. 
         
        \ :code:`vdest` is a vector consisting of the indices of those elements of 
        the source vector that match the condition opstring. 
         
        Opstring is a string matching one of these: 

        .. code-block::
            none

             \ :code:`==`		\ :code:`!=` 
             \ :code:`>`		\ :code:`<` 
             \ :code:`>=`		\ :code:`<=` 

        Op2string is a string matching one of these: 

        .. code-block::
            none

             \ :code:`[]`		\ :code:`[)` 
             \ :code:`(]`		\ :code:`()` 

         
        Comparisons are relative to the :func:`float_epsilon` global variable. 
         

    Example:
        objref vs, vd 

        .. code-block::
            none

            vs = new Vector() 
             
            {vs.indgen(0, .9, .1) 
            vs.printf()} 
             
            print vs.indwhere(">", .3) 
            print "note roundoff error, vs.x[3] - .3 =", vs.x[3] - .3 
            print vs.indwhere("==", .5) 
             
            vd = vs.c.indvwhere(vs, "[)", .3, .7) 
            {vd.printf()} 


         

    .. seealso::
        :meth:`Vector.where`

         

----



.. method:: Vector.fwrite


    Syntax:
        :code:`n = vsrc.fwrite(fileobj)`

        :code:`n = vsrc.fwrite(fileobj, start, end)`


    Description:
        Write the vector \ :code:`vec` to an open *fileobj* of type :func:`File` in 
        machine dependent binary format. 
        You must keep track of the vector's 
        size for later reading, so it is recommended that you store the size of the 
        vector as the first element of the file. 
         
        It is almost always better to use :func:`vwrite` since it stores the size 
        of the vector automatically and is more portable since the corresponding 
        vread will take care of machine dependent binary byte ordering differences. 
         
        Return value is the number of items. (0 if error) 
         
        :func:`fread` is used to read a file containing numbers stored by \ :code:`fwrite` but 
        must have the same size. 

         

----



.. method:: Vector.fread


    Syntax:
        :code:`n = vdest.fread(fileobj)`

        :code:`n = vdest.fread(fileobj, n)`

        :code:`n = vdest.fread(fileobj, n, precision)`


    Description:
        Read the elements of a vector from the file in binary as written by \ :code:`fwrite.` 
        If *n* is present, the vector is resized before reading. Note that 
        files created with fwrite cannot be fread on a machine with different 
        byte ordering. E.g. spark and intel cpus have different byte ordering. 
         
        It is almost always better to use \ :code:`vwrite` in combination with \ :code:`vread`. 
        See vwrite for the meaning of the *precision* argment. 
         
        Return value is 1 (no error checking). 

         

----



.. method:: Vector.vwrite


    Syntax:
        :code:`n = vec.vwrite(fileobj)`

        :code:`n = vec.vwrite(fileobj, precision)`


    Description:
        Write the vector in binary format 
        to an already opened for writing * fileobj* of type 
        :func:`File` . 
        \ :code:`vwrite()` is easier to use than \ :code:`fwrite()` 
        since it stores the size of the vector and type information 
        for a more 
        automated read/write. The file data can also be vread on a machine with 
        different byte ordering. e.g. you can vwrite with an intel cpu and vread 
        on a sparc. 
        Precision formats 1 and 2 employ a simple automatic 
        compression which is uncompressed automatically by vread.  Formats 3 and 4 
        remain uncompressed. 
         
        Default precision is 4 (double) because this is the usual type 
        used for numbers in oc and therefore requires no conversion or 
        compression 

        .. code-block::
            none

            *  1 : char            shortest    8  bits    
            *  2 : short                       16 bits 
               3 : float                       32 bits 
               4 : double          longest     64 bits    
               5 : int                         sizeof(int) bytes 

         
        * Warning! these are useful primarily for storage of data: exact 
        values will not necessarily be maintained due to the conversion 
        process 
         
        Return value is 1 . Only if the type field is invalid will the return 
        value be 0. 

         

----



.. method:: Vector.vread


    Syntax:
        :code:`n = vec.vread(fileobj)`


    Description:
        Read vector from binary format file written with \ :code:`vwrite()`. 
        Size and data type have 
        been stored by \ :code:`vwrite()` to allow correct retrieval syntax, byte ordering, and 
        decompression (where necessary).  The vector is automatically resized. 
         
        Return value is 1. (No error checking.) 

    Example:

        .. code-block::
            none

            objref v1, v2, f 
            v1 = new Vector() 
            v1.indgen(20,30,2) 
            v1.printf() 
            f = new File() 
            f.wopen("temp.tmp") 
            v1.vwrite(f) 
             
            v2 = new Vector() 
            f.ropen("temp.tmp") 
            v2.vread(f) 
            v2.printf() 


         

----



.. method:: Vector.printf


    Syntax:
        :code:`n = vec.printf()`

        :code:`n = vec.printf(format_string)`

        :code:`n = vec.printf(format_string, start, end)`

        :code:`n = vec.printf(fileobj)`

        :code:`n = vec.printf(fileobj, format_string)`

        :code:`n = vec.printf(fileobj, format_string, start, end)`


    Description:
        Print the values of the vector in ascii either to the screen or a File instance 
        (if \ :code:`fileobj` is present).  *Start* and *end* enable you to specify 
        which particular set of indexed values to print. 
        Use \ :code:`format_string` for formatting the output of each element. 
        This string must contain exactly one \ :code:`%f`, \ :code:`%g`, or \ :code:`%e`, 
        but can also contain additional formatting instructions. 
         
        Return value is number of items printed. 

    Example:

        .. code-block::
            none

            vec = new Vector() 
            vec.indgen(0, 1, 0.1) 
            vec.printf("%8.4f\n") 

        prints the numbers 0.0000 through 0.9000 in increments of 0.1.  Each number will 
        take up a total of eight spaces, will have four decimal places 
        and will be printed on a new line. 

    .. warning::
        No error checking is done on the format string and invalid formats can cause 
        segmentation violations. 

         

----



.. method:: Vector.scanf


    Syntax:
        :code:`n = vec.scanf(fileobj)`

        :code:`n = vec.scanf(fileobj, n)`

        :code:`n = vec.scanf(fileobj, c, nc)`

        :code:`n = vec.scanf(fileobj, n, c, nc)`


    Description:
        Read ascii values from a :func:`File` instance (must already be opened for reading) 
        into vector.  If present, scanning takes place til *n* items are 
        read or until EOF. Otherwise, \ :code:`vec.scanf` reads until end of file. 
        If reading 
        til eof, a number followed 
        by a newline must be the last string in the file. (no trailing spaces 
        after the number and no extra newlines). 
        When reading til EOF, the vector grows approximately by doubling when 
        its currently allocated space is filled. To avoid the overhead of 
        memory reallocation when scanning very long vectors (e.g. > 50000 elements) 
        it is a good idea to presize the vector to a larger value than the 
        expected number of elements to be scanned. 
        Note that although the vector is resized to 
        the actual number of elements scanned, the space allocated to the 
        vector remains available for growth. See :meth:`Vector.buffer_size` . 
         
        Read from 
        column *c* of *nc* columns when data is in column format.  It numbers 
        the columns beginning from 1. 
         
        The scan takes place at the current position of the file. 
         
        Return value is number of items read. 

    .. seealso::
        :meth:`Vector.scantil`

         

----



.. method:: Vector.scantil


    Syntax:
        :code:`n = vec.scantil(fileobj, sentinel)`

        :code:`n = vec.scantil(fileobj, sentinel, c, nc)`


    Description:
        Like :meth:`Vector.scanf` but scans til it reads a value equal to the 
        sentinel. e.g. -1e15 is a possible sentinel value in many situations. 
        The vector does not include the sentinel value. The file pointer is 
        left at the character following the sentinel. 
         
        Read from 
        column *c* of *nc* columns when data is in column format.  It numbers 
        the columns beginning from 1. The scan stops when the sentinel is found in 
        any position prior to column c+1 but it is recommended that the sentinel 
        appear by itself on its own line. The file pointer is left at the 
        character following the sentinel. 
         
        The scan takes place at the current position of the file. 
         
        Return value is number of items read. 

         

----



.. method:: Vector.plot


    Syntax:
        :code:`obj = vec.plot(graphobj)`

        :code:`obj = vec.plot(graphobj, color, brush)`

        :code:`obj = vec.plot(graphobj, x_vec)`

        :code:`obj = vec.plot(graphobj, x_vec, color, brush)`

        :code:`obj = vec.plot(graphobj, x_increment)`

        :code:`obj = vec.plot(graphobj, x_increment, color, brush)`


    Description:
        Plot vector in a :class:`Graph` object.  The default is to plot the elements of the 
        vector as y values with their indices as x values.  An optional 
        argument can be used to 
        specify the x-axis.  Such an argument can be either a 
        vector, *x_vec*, in which case its values are used for x values, or 
        a scalar,  *x_increment*, in 
        which case x is incremented according to this number. 
         
        This function plots the 
        \ :code:`vec` values that exist in the vector at the time of graph flushing or window 
        resizing. The alternative is \ :code:`vec.line()` which plots the vector values 
        that exist at the time of the call to \ :code:`plot`.  It is therefore possible with 
        \ :code:`vec.line()` to produce multiple plots 
        on the same graph. 
         
        Once a vector is plotted, it is only necessary to call \ :code:`graphobj.flush()` 
        in order to display further changes to the vector.  In this way it 
        is possible to produce rather rapid line animation. 
         
        If the vector :meth:`Graph.label` is not empty it will be used as the label for 
        the line on the Graph. 
         
        Resizing a vector that has been plotted will remove it from the Graph. 
         
        The number of points plotted is the minimum of vec.size and x_vec.size 
        at the time vec.plot is called. x_vec is assumed to be an unchanging 
        Vector. 
         

    Example:

        .. code-block::
            none

            objref vec, g 
            g = new Graph() 
            g.size(0,10,-1,1) 
            vec = new Vector() 
            vec.indgen(0,10, .1) 
            vec.apply("sin") 
            vec.plot(g, .1) 
            xpanel("") 
            xbutton("run", "for i=0,vec.size()-1 { vec.rotate(1) g.flush() doNotify()}") 
            xpanel() 


    .. seealso::
        :func:`vector`

         

----



.. method:: Vector.line


    Syntax:
        :code:`obj = vec.line(graphobj)`

        :code:`obj = vec.line(graphobj, color, brush)`

        :code:`obj = vec.line(graphobj, x_vec)`

        :code:`obj = vec.line(graphobj, x_vec, color, brush)`

        :code:`obj = vec.line(graphobj, x_increment)`

        :code:`obj = vec.line(graphobj, x_increment, color, brush)`


    Description:
        Plot vector on a :func:`Graph` .  Exactly like \ :code:`.plot()` except the vector 
        is *not* plotted by reference so that the values may be changed 
        subsequently w/o disturbing the plot.  It is therefore possible to produce 
        a number of plots of the same function on the same graph, 
        without erasing any previous plot. 
         
        The line on a graph is given the :meth:`Graph.label` if the label is not empty. 
         
        The number of point plotted is the minimum of vec.size and x_vec.size . 
         

    Example:

        .. code-block::
            none

            objref vec, g 
            g = new Graph() 
            g.size(0,10,-1,1) 
            vec = new Vector() 
            vec.indgen(0,10, .1) 
            vec.apply("sin") 
            for i=0,3 { vec.line(g, .1) vec.rotate(10) } 


    .. seealso::
        :func:`family`

         

----



.. method:: Vector.ploterr


    Syntax:
        :code:`obj = vec.ploterr(graphobj, x_vec, err_vec)`

        :code:`obj = vec.ploterr(graphobj, x_vec, err_vec, size)`

        :code:`obj = vec.ploterr(graphobj, x_vec, err_vec, size, color, brush)`


    Description:
        Similar to \ :code:`vec.line()`, but plots error bars with size +/- the elements 
        of vector *err_vec*. 
         
        *size* sets the width of the seraphs on the error bars to a number 
        of printer dots. 
         
        *brush* sets the width of the plot line.  0=invisible, 
        1=minimum width, 2=1point, etc. 
         

    Example:

        .. code-block::
            none

            objref vec, xvec, errvec 
            objref g 
            g = new Graph() 
            g.size(0,100, 0,250) 
            vec = new Vector() 
            xvec = new Vector() 
            errvec = new Vector() 
             
            vec.indgen(0,200,20) 
            xvec.indgen(0,100,10) 
            errvec.copy(xvec) 
            errvec.apply("sqrt") 
            vec.ploterr(g, xvec, errvec, 10) 
            vec.mark(g, xvec, "O", 5) 

        creates a graph which has x values of 0 through 100 in increments of 10 and 
        y values of 0 through 200 in increments of 20.  At each point graphed, vertical 
        error bars are also drawn which are the +/- the length of the square root of the 
        values 0 through 100 in increments of 10.  Each error bar has seraphs which are 
        ten printer points wide. The graph is also marked with filled circles 5 printers 
        points in diameter. 

         

----



.. method:: Vector.mark


    Syntax:
        :code:`obj = vec.mark(graphobj, x_vector)`

        :code:`obj = vec.mark(graphobj, x_vector, "style")`

        :code:`obj = vec.mark(graphobj, x_vector, "style", size)`

        :code:`obj = vec.mark(graphobj, x_vector, "style", size, color, brush)`

        :code:`obj = vec.mark(graphobj, x_increment)`

        :code:`obj = vec.mark(graphobj, x_increment, "style", size, color, brush)`


    Description:
        Similar to \ :code:`vec.line`, but instead of connecting by lines, it make marks, 
        centered at the indicated position, which do not change size when 
        window is zoomed or resized. The style is a single character 
        \ :code:`|,-,+,o,O,t,T,s,S` where \ :code:`o,t,s` stand for circle, triangle, square 
        and capitalized means filled. Default size is 12 points. 

         

----



.. method:: Vector.histogram


    Syntax:
        :code:`newvect = vsrc.histogram(low, high, width)`


    Description:
        Create a histogram constructed by binning the values in \ :code:`vsrc`. 
         
        Bins run from *low* to *high* in divisions of *width*.  Data outside 
        the range is not binned. 
         
        This function returns a vector that contains the counts in each bin, so while it is 
        necessary to declare an object reference (\ :code:`objref newvect`), it is not necessary 
        to execute \ :code:`newvect = new Vector()`. 
         
        The first element of \ :code:`newvect` is 0 (\ :code:`newvect.x[0] = 0`). 
        For \ :code:`ii > 0`, \ :code:`newvect.x[ii]` equals the number of 
        items 
        in \ :code:`vsrc` whose values lie in the half open interval 
        \ :code:`[a,b)` 
        where \ :code:`b = low + ii*width` and \ :code:`a = b - width`. 
        In other words, \ :code:`newvect.x[ii]` is the number of items in 
        \ :code:`vsrc` 
        that fall in the bin just below the boundary \ :code:`b`. 
         
         

    Example:

        .. code-block::
            none

            objref interval, hist, rand 
             
            rand = new Random() 
            rand.negexp(1) 
             
            interval = new Vector(100) 
            interval.setrand(rand) // random intervals 
             
            hist = interval.histogram(0, 10, .1) 
             
            // and for a manhattan style plot ... 
            objref g, v2, v3 
            g = new Graph() 
            g.size(0,10,0,30) 
            // create an index vector with 0,0, 1,1, 2,2, 3,3, ... 
            v2 = new Vector(2*hist.size())      
            v2.indgen(.5)  
            v2.apply("int")  
            //  
            v3 = new Vector(1)  
            v3.index(hist, v2)  
            v3.rotate(-1)            // so different y's within each pair 
            v3.x[0] = 0  
            v3.plot(g, v2) 

        creates a histogram of the occurrences of random numbers 
        ranging from 0 to 10 in divisions of 0.1. 

         

----



.. method:: Vector.hist


    Syntax:
        :code:`obj = vdest.hist(vsrc, low, size, width)`


    Description:
        Similar to :func:`histogram` (but notice the different argument meanings. 
        Put a histogram in *vdest* by binning 
        the data in *vsrc*. 
        Bins run from *low* to \ :code:`low + size * width` 
        in divisions of *width*. 
        Data outside 
        the range is not binned. 

         

----



.. method:: Vector.sumgauss


    Syntax:
        :code:`newvect = vsrc.sumgauss(low, high, width, var)`

        :code:`newvect = vsrc.sumgauss(low, high, width, var, weight_vec)`


    Description:
        Create a vector which is a curve calculated by summing gaussians of 
        area 1 centered on all the points in the vector.  This has the 
        advantage over \ :code:`histogram` of not imposing arbitrary bins. *low* 
        and *high* set the range of the curve. 
        *width* determines the granularity of the 
        curve. *var* sets the variance of the gaussians. 
         
        The optional argument \ :code:`weight_vec` is a vector which should be the same 
        size as \ :code:`vec` and is used to scale or weight the gaussians (default is 
        for them all to have areas of 1 unit). 
         
        This function returns a vector, so while it is 
        necessary to declare a vector object (\ :code:`objref vectobj`), it is not necessary 
        to declare *vectobj* as a \ :code:`new Vector()`. 
         
        To plot, use \ :code:`v.indgen(low,high,width)` for the x-vector argument. 

    Example:

        .. code-block::
            none

            objref r, data, hist, x, g 
             
            r = new Random() 
            r.normal(1, 2) 
             
            data = new Vector(100) 
            data.setrand(r) 
             
            hist = data.sumgauss(-4, 6, .5, 1) 
            x = new Vector(hist.size()) 
            x.indgen(-4, 6, .5) 
             
            g = new Graph() 
            g.size(-4, 6, 0, 30) 
            hist.plot(g, x) 


         

----



.. method:: Vector.smhist


    Syntax:
        :code:`obj = vdest.smhist(vsrc, start, size, step, var)`

        :code:`obj = vdest.smhist(vsrc, start, size, step, var, weight_vec)`


    Description:
        Very similar to :func:`sumgauss` . Calculate a smooth histogram by convolving 
        the raw data set with a gaussian kernel.  The histogram begins at 
        \ :code:`varstart` and has \ :code:`varsize` values in increments of size \ :code:`varstep`. 
        \ :code:`varvar` sets the variance of the gaussians. 
        The optional argument \ :code:`weight_vec` 
        is a vector which should be the same size as \ :code:`vsrc` and is used to scale or 
        weight the number of data points at a particular value. 

         

----



.. method:: Vector.ind


    Syntax:
        :code:`newvect = vsrc.ind(vindex)`


    Description:
        Return a new vector consisting of the elements of \ :code:`vsrc` whose indices are given 
        by the elements of \ :code:`vindex`. 
         

    Example:

        .. code-block::
            none

            objref vec, vec1, vec2 
            vec = new Vector(100) 
            vec2 = new Vector() 
            vec.indgen(5) 
            vec2.indgen(49, 59, 1) 
            vec1 = vec.ind(vec2) 

        creates \ :code:`vec1` to contain the fiftieth through the sixtieth elements of \ :code:`vec2` 
        which would have the values 245 through 295 in increments of 5. 
         

         

----



.. method:: Vector.addrand


    Syntax:
        :code:`obj = vsrcdest.addrand(randobj)`

        :code:`obj = vsrcdest.addrand(randobj, start, end)`


    Description:
        Adds random values to the elements of the vector by sampling from the 
        same distribution as last picked in the Random object *randobj*. 

    Example:

        .. code-block::
            none

            objref vec, g, r 
            vec = new Vector(50) 
            g = new Graph() 
            g.size(0,50,0,100) 
            r = new Random() 
            r.poisson(.2) 
            vec.plot(g) 
             
            proc race() {local i 
                    vec.fill(0) 
                    for i=1,300 { 
                            vec.addrand(r) 
                            g.flush() 
                            doNotify() 
                    } 
            } 
             
            race()  


         

----



.. method:: Vector.setrand


    Syntax:
        :code:`obj = vdest.setrand(randobj)`

        :code:`obj = vdest.setrand(randobj, start, end)`


    Description:
        Sets random values for the elements of the vector by sampling from the 
        same distribution as last picked in *randobj*. 

         

----



.. method:: Vector.sin


    Syntax:
        :code:`obj = vdest.sin(freq, phase)`

        :code:`obj = vdest.sin(freq, phase, dt)`


    Description:
        Generate a sin function in vector \ :code:`vec` with frequency *freq* hz, phase 
        *phase* in radians.  *dt* is assumed to be 1 msec unless specified. 

         

----



.. method:: Vector.apply


    Syntax:
        :code:`obj = vsrcdest.apply("func")`

        :code:`obj = vsrcdest.apply("func", start, end)`


    Description:
        Apply a hoc function to each of the elements in the vector. 
        The function can be any function that is accessible in oc.  It 
        must take only one scalar argument and return a scalar. 
        Note that the function name must be in quotes and that the parentheses 
        are omitted. 

    Example:

        .. code-block::
            none

            vec.apply("sin", 0, 9) 

        applies the sin function to the first ten elements of the vector \ :code:`vec`. 

         

----



.. method:: Vector.reduce


    Syntax:
        :code:`x = vsrc.reduce("func")`

        :code:`x = vsrc.reduce("func", base)`

        :code:`x = vsrc.reduce("func", base, start, end)`


    Description:
        Pass all elements of a vector through a function and return the sum of 
        the results.  Use *base* to initialize the value x. 
        Note that the function name must be in quotes and that the parentheses 
        are omitted. 

    Example:

        .. code-block::
            none

            objref vec 
            vec = new Vector() 
            vec.indgen(0, 10, 2) 
            func sq(){ 
            	return $1*$1 
            } 
            vec.reduce("sq", 100) 

        returns the value 320. 
         
        100 + 0*0 + 2*2 + 4*4 + 6*6 + 8*8 + 10*10 = 320 
         

         

----



.. method:: Vector.floor


    Syntax:
        :code:`vec.floor()`


    Description:
        Rounds toward negative infinity. Note that :func:`float_epsilon` is not 
        used in this calculation. 

         
         

----



.. method:: Vector.to_python


    Syntax:
        :code:`pythonlist = vec.to_python()`

        :code:`pythonlist = vec.to_python(pythonlist)`

        :code:`numpyarray = vec.to_python(numpyarray)`


    Description:
        Copy the vector elements from the hoc vector to a pythonlist or 
        1-d numpyarray. If the arg exists the pythonobject must have the same 
        size as the hoc vector. 

         

----



.. method:: Vector.from_python


    Syntax:
        :code:`vec = vec.from_python(pythonlist)`

        :code:`vec = vec.from_python(numpyarray)`


    Description:
        Copy the python list elements into the hoc vector. The elements must be 
        numbers that are convertable to doubles. 
        Copy the numpy 1-d array elements into the hoc vector. 
        The hoc vector is resized. 


