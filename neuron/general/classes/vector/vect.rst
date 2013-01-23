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
        containing a :code:`double x[]` variable. Individual elements of this array can 
        be manipulated with the normal :samp:`{objref}.x[{index}]` notation. 
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
        v1 = v2*s2 + v3*s3 + v4*s4 could be written 

        .. code-block::
            none

            	v1 = v2.c.mul(s2).add(v3.c.mul(s3)).add(v4.c.mul(s4)) 

        but if the expressions get too complex it is probably clearer to employ 
        temporary objects to break the process into several separate expressions. 
         

    Example:

        .. code-block::
            none

            objref vec
            vec = new Vector(20,5)

        will create a vector with 20 indices, each having the value of 5. 

        .. code-block::
            none

            objref vec1
            vec1 = new Vector()

        will create a vector with 1 index which has value of 0. It is seldom 
        necessary to specify a size for a new vector since most operations, if necessary, 
        increase or decrease the number of available elements as needed. 
         

    .. seealso::
        :ref:`double <keyword_double>`,	:data:`Vector.x`, :meth:`Vector.resize`

         

----



.. data:: Vector.x


    Syntax:
        :code:`vec.x[index]`


    Description:
        Elements of a vector can be accessed with :code:`vec.x[index]` notation. 
        Vector indices range from 0 to Vector.size()-1. 
        This 
        notation is superior to the older :code:`vec.get()` and :code:`vec.set()` notations for 
        three reasons: 
         
        1.  It performs the roles of both 
            :code:`vec.get` and :code:`vec.set` with a syntax that is consistent with the normal 
            syntax for a :code:`double` array inside of an object. 
        2.  It can be viewed by a field editor (since it can appear on the left hand 
            side of an assignment statement). 
        3.  You can take its  address for functions which require that kind of argument. 

    Example:
        :code:`print vec.x[0]` prints the value of the 0th (first) element. 
         
        :code:`vec.x[i] = 3` sets the i'th element to 3. 
         

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
        :code:`vec.x[-1]` returns the value of the first element of the vector, just as 
        would :code:`vec.x[0]`. 
         
        :code:`vec.x(i)` returns the value of index *i* just as does :code:`vec.x[i]`. 

         

----



.. method:: Vector.size


    Syntax:
        :code:`size = vec.size()`


    Description:
        Return the number of elements in the vector. The last element has the index: 
        :code:`vec.size() - 1`. Most explicit for loops over a vector can take the form: 

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
        (see note in :meth:`Vector.size`). 
        For example, resizing vectors that have been plotted will remove that vector 
        from the plot list. Other functions may not be so forgiving and result in 
        a memory error (segmentation violation or unhandled exception). 

    Example:

        .. code-block::
            none

            objref vec 
            vec = new Vector(20,5) 
            vec.resize(30)
        
        Appends 10 elements, each having a value of 0, to :code:`vec`. 

        .. code-block::
            none

            :code:`vec.resize(10)` 

        removes the last 20 elements from the  :code:`vec`.The values of the first 
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
        is superseded by the :code:`vec.x[]` notation but is retained for backward 
        compatibility. 

         

----



.. method:: Vector.set


    Syntax:
        :code:`obj = vsrcdest.set(index,value)`


    Description:
        Set vector element index to value.  This function is superseded by 
        the :code:`vec.x[i] = expr` notation but is retained for backward 
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
        Transfers take place on exit from :code:`finitialize()` and on exit from :code:`fadvance()`. 
        At the end of :code:`finitialize()`, :code:`v.x[0] = var`. At the end of :code:`fadvance`, 
        *var* will be saved if :code:`t` (after being incremented by :code:`fadvance`) 
        is equal or greater than the associated time of the 
        next index. The system maintains a set of record vectors and the vector will 
        be removed from the list if the vector or var is destroyed. 
        The vector is automatically increased in size by 100 elements at a time 
        if more space is required, so efficiency will be slightly improved if one 
        creates vectors with sufficient size to hold the entire stream, and plots will 
        be more persistent (recall that resizing may cause reallocation of memory 
        to hold elements and this will make pointers invalid). 
         
        The record semantics can be thought of as:
 
        :code:`var(t) -> v.x[index]` 
         
        The default relationship between :code:`index` and 
        :code:`t` is :code:`t = index*dt`. 
 
        In the second form, :code:`t = index*Dt`. 
 
        In the third form, :code:`t = tvec.x[index]`. 
         
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
        record/play behavior is reasonable but surprising if :data:`dt` is greater than 
        :code:`Dt`. Things work best if :code:`Dt` happens to be a multiple of :data:`dt`. All combinations 
        of record ; play ; :code:`Dt =>< dt` ; and tvec sequences 
        have not been tested. 

    Example:
        See :file:`tests/nrniv/vrecord.hoc` for examples of usage. 
         
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
        :code:`dv = new Vector()` 

    .. seealso::
        :func:`finitialize`, :func:`fadvance`, :func:`play`, :data:`t`, :func:`play_remove`

         

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
        The :code:`vsrc` vector values are assigned to the "*var*" variable during 
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
        :code:`v.x[index] -> var(t)` where t(index) is Dt*index or tvec.x[index] 
        The discrete event delivery system is used to determine the precise 
        time at which values are copied from vsrc to var. Note that for variable 
        step methods, unless continuity is specifically requested, the function 
        is a step function. Also, for the local variable dt method, var MUST be 
        associated with the cell that contains the currently accessed section 
        (but see the paragraph below about the use of a point_process_object 
        inserted as the first arg). 
         
        For the fixed step method 
        transfers take place on entry to :func:`finitialize` and  on entry to :func:`fadvance`. 
        At the beginning of :func:`finitialize`, :code:`var = v.x[0]`. On :func:`fadvance` a transfer will 
        take place if t will be (after the :code:`fadvance` increment) equal 
        or greater than the associated time of the next index. For the variable step 
        methods, transfers take place exactly at the times specified by the Dt 
        or tvec arguments. 
         
        The system maintains a set of play vectors and the vector will be removed 
        from the list if the vector or var is destroyed. 
        If the end of the vector is reached, no further transfers are made (:code:`var` becomes 
        constant) 
         
        Note well: for the fixed step method, 
        if :code:`fadvance` exits with time equal to :code:`t` (ie enters at time t-dt), 
        then on entry to :code:`fadvance`, *var* is set equal to the value of 
        the vector at the index 
        appropriate to time t. Execute tests/nrniv/vrecord.hoc to see what this implies 
        during a simulation. ie the value of var from :code:`t-dt` to t played into by 
        a vector is equal to the value of the vector at :code:`index(t)`. If the vector 
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
        The values are appended to the end of the :code:`vsrcdest` vector. 

    Example:

        .. code-block::
            none

            objref vec, vec1, vec2 
            vec = new Vector (10,4) 
            vec1 = new Vector (10,5) 
            vec2 = new Vector (10,6) 
            vec.append(vec1, vec2, 7, 8, 9) 

        turns :code:`vec` into a 33 element vector, whose first ten elements = 4, whose 
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
         
        :code:`obj.insrt(obj.size, ...)` is equivalent to :code:`obj.append(...)` 

         

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
        of its elements (to within :data:`float_epsilon`). A return value of 1 signifies true; 0 signifies false. 

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
        source elements (beginning at *src*:code:`.x[0]`) 
        are copied to  *vdest* beginning at *dest*:code:`.x[dest_start]`, 
        *Src_start* and *src_end* here refer to indices of *vsrcx*, 
        not *vdest*.  If *vdest* is too small for the size required by *vsrc* and the 
        arguments, then it is resized to hold the data. 
        If the *dest* is larger than required AND there is more than one 
        argument the *dest* is NOT resized. 
        One may use -1 for the 
        src_end argument to specify the entire size (instead of the 
        tedious :code:`src.size()-1`) 
         
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
 
        .. code-block::
            none
        
 
            objref v1, v2 
            v1 = new Vector(30) 
            v1.indgen() 
            v1.printf() 
            @code... 
            v2 = new Vector() 
            v2.copy(v1, 0, 1, -1, 1, 2) 
            v2.printf() 

        To merge or shuffle two vectors into a third, use:
 
        .. code-block::
            none
            
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

        turns :code:`vec` from a 100 element into a 54 element vector. 
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
        the label. For a complete copy including the label use :meth:`Vector.cl`. 
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
         
        It merely avoids the necessity of a :code:`vdest = new Vector()` command and 
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

        creates :code:`vec1` with 9 elements which correspond to the values at indices 
        2 - 10 in :code:`vec`.  The contents of :code:`vec1` would then be, in order: 14, 16, 18, 
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
        :code:`vdest` is vector consisting of those elements of the given vector, :code:`vsource` 
        that match the condition opstring. 
         
        Opstring is a string matching one of these (all comparisons 
        are with respect to :data:`float_epsilon` ): :code:`"=="`, :code:`"!="`, :code:`">"`, :code:`"<"`, :code:`">="`, :code:`"<="`

        Op2string requires two numbers defining open/closed ranges and matches one 
        of these: :code:`"[]"`, :code:`"[)"`, :code:`"(]"`, :code:`"()"`
         

    Example:

        .. code-block::
            none

            vec = new Vector(25) 
            vec1 = new Vector() 
            vec.indgen(10) 
            vec1.where(vec, ">=", 50) 

        creates :code:`vec1` with 20 elements ranging in value from 50 to 240 in 
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

        creates :code:`vec1` with random elements gotten from :code:`vec` which have values 
        greater than 15.  The new elements in vec1 will be ordered 
        according to the order of their appearance in :code:`vec`. 

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
         
        :code:`vdest` is a vector consisting of the indices of those elements of 
        the source vector that match the condition opstring. 
         
        Opstring is a string matching one of these: :code:`"=="`, :code:`"!="`, :code:`">"`, :code:`"<"`, :code:`">="`, :code:`"<="`


        Op2string is a string matching one of these: :code:`"[]"`, :code:`"[)"`, :code:`"(]"`, :code:`"()"`

         
        Comparisons are relative to the :data:`float_epsilon` global variable. 
         

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
        Write the vector :code:`vec` to an open *fileobj* of type :class:`File` in 
        machine dependent binary format. 
        You must keep track of the vector's 
        size for later reading, so it is recommended that you store the size of the 
        vector as the first element of the file. 
         
        It is almost always better to use :func:`vwrite` since it stores the size 
        of the vector automatically and is more portable since the corresponding 
        vread will take care of machine dependent binary byte ordering differences. 
         
        Return value is the number of items. (0 if error) 
         
        :func:`fread` is used to read a file containing numbers stored by :code:`fwrite` but 
        must have the same size. 

         

----



.. method:: Vector.fread


    Syntax:
        :code:`n = vdest.fread(fileobj)`

        :code:`n = vdest.fread(fileobj, n)`

        :code:`n = vdest.fread(fileobj, n, precision)`


    Description:
        Read the elements of a vector from the file in binary as written by :code:`fwrite.` 
        If *n* is present, the vector is resized before reading. Note that 
        files created with fwrite cannot be fread on a machine with different 
        byte ordering. E.g. spark and intel cpus have different byte ordering. 
         
        It is almost always better to use :code:`vwrite` in combination with :code:`vread`. 
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
        :class:`File`. 
        :meth:`~Vector.vwrite` is easier to use than :code:`fwrite()` 
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

         
        .. warning::
        
            These are useful primarily for storage of data: exact 
            values will not necessarily be maintained due to the conversion 
            process.
         
        Return value is 1. Only if the type field is invalid will the return 
        value be 0. 

         

----



.. method:: Vector.vread


    Syntax:
        :code:`n = vec.vread(fileobj)`


    Description:
        Read vector from binary format file written with :code:`vwrite()`. 
        Size and data type have 
        been stored by :code:`vwrite()` to allow correct retrieval syntax, byte ordering, and 
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
        (if :code:`fileobj` is present).  *Start* and *end* enable you to specify 
        which particular set of indexed values to print. 
        Use :code:`format_string` for formatting the output of each element. 
        This string must contain exactly one :code:`%f`, :code:`%g`, or :code:`%e`, 
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
        Read ascii values from a :class:`File` instance (must already be opened for reading) 
        into vector.  If present, scanning takes place til *n* items are 
        read or until EOF. Otherwise, :code:`vec.scanf` reads until end of file. 
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
        :code:`vec` values that exist in the vector at the time of graph flushing or window 
        resizing. The alternative is :code:`vec.line()` which plots the vector values 
        that exist at the time of the call to :code:`plot`.  It is therefore possible with 
        :code:`vec.line()` to produce multiple plots 
        on the same graph. 
         
        Once a vector is plotted, it is only necessary to call :code:`graphobj.flush()` 
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
        :meth:`Graph.vector`

         

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
        Plot vector on a :class:`Graph`.  Exactly like :code:`.plot()` except the vector 
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
        :meth:`Graph.family`

         

----



.. method:: Vector.ploterr


    Syntax:
        :code:`obj = vec.ploterr(graphobj, x_vec, err_vec)`

        :code:`obj = vec.ploterr(graphobj, x_vec, err_vec, size)`

        :code:`obj = vec.ploterr(graphobj, x_vec, err_vec, size, color, brush)`


    Description:
        Similar to :code:`vec.line()`, but plots error bars with size +/- the elements 
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
        Similar to :code:`vec.line`, but instead of connecting by lines, it make marks, 
        centered at the indicated position, which do not change size when 
        window is zoomed or resized. The style is a single character 
        :code:`|,-,+,o,O,t,T,s,S` where :code:`o,t,s` stand for circle, triangle, square 
        and capitalized means filled. Default size is 12 points. 

         

----



.. method:: Vector.histogram


    Syntax:
        :code:`newvect = vsrc.histogram(low, high, width)`


    Description:
        Create a histogram constructed by binning the values in :code:`vsrc`. 
         
        Bins run from *low* to *high* in divisions of *width*.  Data outside 
        the range is not binned. 
         
        This function returns a vector that contains the counts in each bin, so while it is 
        necessary to declare an object reference (:code:`objref newvect`), it is not necessary 
        to execute :code:`newvect = new Vector()`. 
         
        The first element of :code:`newvect` is 0 (:code:`newvect.x[0] = 0`). 
        For :code:`ii > 0`, :code:`newvect.x[ii]` equals the number of 
        items 
        in :code:`vsrc` whose values lie in the half open interval 
        :code:`[a,b)` 
        where :code:`b = low + ii*width` and :code:`a = b - width`. 
        In other words, :code:`newvect.x[ii]` is the number of items in 
        :code:`vsrc` 
        that fall in the bin just below the boundary :code:`b`. 
         
         

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
        Bins run from *low* to :code:`low + size * width` 
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
        advantage over :code:`histogram` of not imposing arbitrary bins. *low* 
        and *high* set the range of the curve. 
        *width* determines the granularity of the 
        curve. *var* sets the variance of the gaussians. 
         
        The optional argument :code:`weight_vec` is a vector which should be the same 
        size as :code:`vec` and is used to scale or weight the gaussians (default is 
        for them all to have areas of 1 unit). 
         
        This function returns a vector, so while it is 
        necessary to declare a vector object (:code:`objref vectobj`), it is not necessary 
        to declare *vectobj* as a :code:`new Vector()`. 
         
        To plot, use :code:`v.indgen(low,high,width)` for the x-vector argument. 

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
        :code:`varstart` and has :code:`varsize` values in increments of size :code:`varstep`. 
        :code:`varvar` sets the variance of the gaussians. 
        The optional argument :code:`weight_vec` 
        is a vector which should be the same size as :code:`vsrc` and is used to scale or 
        weight the number of data points at a particular value. 

         

----



.. method:: Vector.ind


    Syntax:
        :code:`newvect = vsrc.ind(vindex)`


    Description:
        Return a new vector consisting of the elements of :code:`vsrc` whose indices are given 
        by the elements of :code:`vindex`. 
         

    Example:

        .. code-block::
            none

            objref vec, vec1, vec2 
            vec = new Vector(100) 
            vec2 = new Vector() 
            vec.indgen(5) 
            vec2.indgen(49, 59, 1) 
            vec1 = vec.ind(vec2) 

        creates :code:`vec1` to contain the fiftieth through the sixtieth elements of :code:`vec2` 
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
        Generate a sin function in vector :code:`vec` with frequency *freq* hz, phase 
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

        applies the sin function to the first ten elements of the vector :code:`vec`. 

         

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
        Rounds toward negative infinity. Note that :data:`float_epsilon` is not 
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


----


.. method:: Vector.fit


    Syntax:
        :code:`error = data_vec.fit(fit_vec,"fcn",indep_vec,&p1,[&p2],...,[&pN])`


    Description:
        Use a simplex algorithm to find parameters *p1* through *pN* such to 
        minimize the mean squared error between the "data" contained in 
        :code:`data_vec` and the approximation generated by the user-supplied "*fcn*" 
        applied to the elements of :code:`indep_vec`. 
         
        *fcn* must take one argument which is the main independent variable 
        followed by one or more arguments which are tunable parameters which 
        will be optimized.  Thus the arguments to .fit following "*fcn*" should 
        be completely analogous to the arguments to fcn itself.  The 
        difference is that the args to fcn must all be scalars while the 
        corresponding args to .fit will be a vector object (for the 
        independent variable) and pointers to scalars (for the remaining 
        parameters). 
         
        The results of a call to .fit are three-fold.  First, the parameters 
        of best fit are returned by setting the values of the variables *p1* to 
        *pN* (possible because they are passed as pointers).  Second, the values 
        of the vector fit_vec are set to the fitted function.  If :code:`fit_vec` is 
        not passed with the same size as :code:`indep_vec` and :code:`data_vec`, it is resized 
        accordingly.  Third, the mean squared error between the fitted 
        function and the data is returned by :code:`.fit`.  The :code:`.fit()` call may be 
        reiterated several times until the error has reached an acceptable 
        level. 
         
        Care must be taken in selecting an initial set of parameter values. 
        Although you need not be too close, wild discrepancies will cause the 
        simplex algorithm to give up.  Values of 0 are to be avoided.  Trial 
        and error is sometimes necessary. 
         
        Because calls to hoc have a high overhead, this procedure can be 
        rather slow.  Several commonly-used functions are provided directly 
        in c code and will work much faster.  In each case, if the name below 
        is used, the builtin function will be used and the user is expected to 
        provide the correct number of arguments (here denoted :code:`a,b,c`...). 

        .. code-block::
            none

            "exp1": y = a * exp(-x/b)   
            "exp2": y = a * exp(-x/b) + c * exp (-x/d) 
            "charging": y = a * (1-exp(-x/b)) + c * (1-exp(-x/d)) 
            "line": y = a * x + b 
            "quad": y = a * x^2 + b*x + c 


    .. warning::
        This function is not very useful for fitting the results of simulation runs 
        due to its argument organization. For that purpose the :func:`fit_praxis` syntax 
        is more suitable. This function should become a top-level function which 
        merely takes a user error function name and a parameter list. 
         
        An alternative implementation of the simplex fitting algorithm is in 
        the scopmath library. 

    .. seealso::
        :func:`fit_praxis`

    Example:
        The :menuselection:`NEURON Main Menu --> Miscellaneous --> Parameterized Function` widget uses this function 
        and is implemented in :file:`nrn/lib/hoc/funfit.hoc`
         
        The following example demonstrates the strategy used by the simplex 
        fitting algorithm to search for a minimum. The location of the parameter 
        values is plotted on each call to the function. 
        The sample function has a minimum at the point (1, .5) 
         

        .. code-block::
            none

            objref g, dvec, fvec, ivec 
            g = new Graph() 
            g.size(0,3,0,3) 
             
            func fun() {local f 
                    if ($1 == 0) { 
                            g.line($2, $3) 
                            g.flush() 
                            print $1, $2, $3 
                    } 
                    return ($2 - 1)^2 +($3-.5)^2 
            } 
             
             
            dvec = new Vector(2) 
            fvec = new Vector(2) 
            fvec.fill(1) 
            ivec = new Vector(2) 
            ivec.indgen() 
             
            a = 2 
            b = 1 
            g.beginline() 
            error = dvec.fit(fvec, "fun", ivec, &a, &b) 
            print a, b, error 

         

         


----

.. _vect2:

.. method:: Vector.interpolate


    Syntax:
        :code:`obj = ysrcdest.interpolate(xdest, xsrc)`

        :code:`obj = ydest.interpolate(xdest, xsrc, ysrc)`


    Description:
        Linearly interpolate points from (xsrc,ysrc) to (xdest,ydest) 
        In the second form, xsrc and ysrc remain unchanged. 
        Destination points outside the domain of xsrc are set to 
        :code:`ysrc[0]` or :code:`ysrc[ysrc.size-1]`

    Example:

         

        .. code-block::
            none
                
            objref g 
            g = new Graph() 
            g.size(0,10,0,100) 

            //... 
            objref xs, ys, xd, yd 
            xs = new Vector(10) 
            xs.indgen() 
            ys = xs.c.mul(xs) 
            ys.line(g, xs, 1, 0) // black reference line 
             
            xd = new Vector() 
             
            xd.indgen(-.5, 10.5, .1) 
            yd = ys.c.interpolate(xd, xs) 
            yd.line(g, xd, 3, 0) // blue more points than reference 
             
            xd.indgen(-.5, 13, 3) 
            yd = ys.c.interpolate(xd, xs) 
            yd.line(g, xd, 2, 0) // red fewer points than reference 


         

----



.. method:: Vector.deriv


    Syntax:
        :code:`obj = vdest.deriv(vsrc)`

        :code:`obj = vdest.deriv(vsrc, dx)`

        :code:`obj = vdest.deriv(vsrc, dx, method)`

        :code:`obj = vsrcdest.deriv()`

        :code:`obj = vsrcdest.deriv(dx)`

        :code:`obj = vsrcdest.deriv(dx, method)`


    Description:
        The numerical Euler derivative or the central difference derivative of :code:`vec` 
        is placed in :code:`vdest`. 
        The variable *dx* gives the increment of the independent variable 
        between successive elements of :code:`vec`. 


        *method* = 1 = Euler derivative: 
            :code:`vec1[i] = (vec[i+1] - vec[i])/dx` 
 
            Each time this method is used, 
            the first element 
            of :code:`vec` is lost since *i* cannot equal -1.  Therefore, since the 
            :code:`integral` function performs an Euler 
            integration, the integral of :code:`vec1` will reproduce :code:`vec` minus the first 
            element. 

        *method* = 2 = Central difference derivative: 
            :code:`vec1[i] = ((vec[i+1]-vec[i-1])/2)/dx` 
 
            This method produces an Euler derivative for the first and last 
            elements of :code:`vec1`.  The central difference method maintains the 
            same number of elements in :code:`vec1` 
            as were in :code:`vec` and is a more accurate method than the Euler method. 
            A vector differentiated by this method cannot, however, be integrated 
            to reproduce the original :code:`vec`. 

         

    Example:

        .. code-block::
            none

            objref vec, vec1 
            vec = new Vector() 
            vec1 = new Vector() 
            vec.indgen(0, 5, 1) 
            func sq(){ 
            	return $1*$1 
            } 
            vec.apply("sq") 
            vec1.deriv(vec, 0.1) 

        creates :code:`vec1` with elements: 

        .. code-block::
            none

            10	20	 
            40	60	 
            80	90 

        Since *dx*\ =0.1, and there are eleven elements including 0, 
        the entire function exists between the values of 0 and 1, and the derivative 
        values are large compared to the function values. With *dx*\ =1,the vector 
        :code:`vec1` would consist of the following elements: 

        .. code-block::
            none

            1	2	 
            4	6	 
            8	9 

         
        The Euler method vs. the Central difference method:
 
        Beginning with the vector :code:`vec`: 

        .. code-block::
            none

            0	1	 
            4	9	 
            16	25 

        :code:`vec1.deriv(vec, 1, 1)` (Euler) would go about 
        producing :code:`vec1` by the following method: 

        .. code-block::
            none

            1-0   = 1	4-1  = 3		 
            9-4   = 5	16-9 = 7	 
            25-16 = 9 

        whereas :code:`vec1.deriv(vec, 1, 2)` (Central difference) would go about 
        producing :code:`vec1` as such: 

        .. code-block::
            none

            1-0      = 1		(4-0)/2  = 2	 
            (9-1)/2  = 4		(16-4)/2 = 6	 
            (25-9)/2 = 8		25-16    = 9 


         

----



.. method:: Vector.integral


    Syntax:
        :code:`obj = vdest.integral(vsrc)`

        :code:`obj = vdest.integral(vsrc, dx)`

        :code:`obj = vsrcdest.integral()`

        :code:`obj = vsrcdest.integral(dx)`


    Description:
        Places a numerical Euler integral of the vsrc elements in vdest. 
        *dx* sets the size of the discretization. 
         
        :code:`vdest[i+1] = vdest[i] + vsrc[i+1]` and the first element of :code:`vdest` is always 
        equal to the first element of :code:`vsrc`. 

    Example:

        .. code-block::
            none

            objref vec, vec1 
            vec = new Vector() 
            vec1 = new Vector() 
            vec.indgen(0, 5, 1)	//vec will have 6 values from 0 to 5, with increment=1 
            vec.apply("sq")		//sq() squares an element  
            			//and is defined in the example for .deriv 
            vec1.integral(vec, 1)	//Euler integral of vec elements approximating 
            			//an x-squared function, dx = 0.1 
            vec1.printf() 

        will print the following elements in :code:`vec1` to the screen: 

        .. code-block::
            none

            0	1	5	 
            14	30	55 

        In order to make the integral values more accurate, it is necessary to increase 
        the size of the vector and to decrease the size of *dx*. 

        .. code-block::
            none

            objref vec2 
            vec2 = new Vector(6) 
            vec.indgen(0, 5.1, 0.1)	//vec will have 51 values from 0 to 5, with increment=0.1 
            vec.apply("sq")		//sq() squares an element  
            			//and is defined in the example for .deriv 
            vec1.integral(vec, 0.1)	//Euler integral of vec elements approximating 
            			//an x-squared function, dx = 0.1 
            for i=0,5{vec2.x[i] = vec1.x[i*10]}  //put the value of every 10th index in vec2 
            vec2.printf() 

        will print the following elements in :code:`vec2` (which are the elements of 
        :code:`vec1` corresponding to the integers 0-5) to the screen: 

        .. code-block::
            none

            0	0.385	2.87 
            9.455	22.14	42.925 

        The integration naturally becomes more accurate as 
        *dx* is reduced and the size of the vector is increased.  If the vector 
        is taken to 501 elements from 0-5 and *dx* is made to equal 0.01, the integrals 
        of the integers 0-5 yield the following (compared to their continuous values 
        on their right). 

        .. code-block::
            none

            0.00000 -- 0.00000	0.33835 --  0.33333	2.6867  --  2.6666 
            9.04505 -- 9.00000	21.4134 -- 21.3333	41.7917 -- 41.6666 


         

----



.. method:: Vector.median


    Syntax:
        :code:`median = vsrc.median()`


    Description:
        Find the median value of :code:`vec`. 

         

----



.. method:: Vector.medfltr


    Syntax:
        :code:`obj = vdest.medfltr(vsrc)`

        :code:`obj = vdest.medfltr(vsrc, points)`

        :code:`obj = vsrcdest.medfltr()`

        :code:`obj = vsrcdest.medfltr( points)`


    Description:
        Apply a median filter to vsrc, producing a smoothed version in vdest. 
        Each point is replaced with the median value of the *points* on 
        either side. 
        This is typically used for eliminating spikes from data. 

         

----



.. method:: Vector.sort


    Syntax:
        :code:`obj = vsrcdest.sort()`


    Description:
        Sort the elements of :code:`vec1` in place, putting them in numerical order. 

         

----



.. method:: Vector.sortindex


    Syntax:
        :code:`vdest = vsrc.sortindex()`

        :code:`vdest = vsrc.sortindex(vdest)`


    Description:
        Return a new vector of indices which sort the vsrc elements in numerical 
        order. That is vsrc.index(vsrc.sortindex) is equivalent to vsrc.sort(). 
        If vdest is present, use that as the destination vector for the indices. 
        This, if it is large enough, avoids the destruct/construct of vdest. 

    Example:

        .. code-block::
            none

            objref a, r, si 
            r = new Random() 
            r.uniform(0,100) 
            a = new Vector(10) 
            a.setrand(r) 
            a.printf 
             
            si = a.sortindex 
            si.printf 
            a.index(si).printf 

         

         

----



.. method:: Vector.reverse


    Syntax:
        :code:`obj = vsrcdest.reverse()`


    Description:
        Reverses the elements of :code:`vec` in place. 

         

----



.. method:: Vector.rotate


    Syntax:
        :code:`obj = vsrcdest.rotate(value)`

        :code:`obj = vsrcdest.rotate(value, 0)`


    Description:
        A negative *value* will move elements to the left.  A positive argument 
        will move elements to the right.  In both cases, the elements shifted off one 
        end of the vector will reappear at the other end. 
        If a 2nd arg is present, 0 values get shifted in and elements shifted off 
        one end are lost. 

    Example:

        .. code-block::
            none

            vec.indgen(1, 10, 1) 
            vec.rotate(3) 

        orders the elements of :code:`vec` as follows: 

        .. code-block::
            none

            8  9  10  1  2  3  4  5  6  7 

        whereas, 

        .. code-block::
            none

            vec.indgen(1, 10, 1) 
            vec.rotate(-3) 

        orders the elements of :code:`vec` as follows: 

        .. code-block::
            none

            4  5  6  7  8  9  10  1  2  3 


        .. code-block::
            none

            objref vec 
            vec = new Vector() 
            vec.indgen(1,5,1) 
            vec.printf 
            vec.c.rotate(2).printf 
            vec.c.rotate(2, 0).printf 
            vec.c.rotate(-2).printf 
            vec.c.rotate(-2, 0).printf 


         

----



.. method:: Vector.rebin


    Syntax:
        :code:`obj = vdest.rebin(vsrc,factor)`

        :code:`obj = vsrcdest.rebin(factor)`


    Description:
        Compresses length of vector :code:`vsrc` by an integer *factor*.  The sum of 
        elements is conserved, unless the *factor* produces a remainder, 
        in which case the remainder values are truncated from :code:`vdest`. 

    Example:

        .. code-block::
            none

            vec.indgen(1, 10, 1) 
            vec1.rebin(vec, 2) 

        produces :code:`vec1`: 

        .. code-block::
            none

            3  7  11  15  19 

        where each pair of :code:`vec ` elements is added together into one element. 
         
        But, 

        .. code-block::
            none

            vec.indgen(1, 10, 1) 
            vec1.rebin(vec, 3) 

        adds trios :code:`vec` elements and gets rid of the value 10, producing 
        :code:`vec1`: 

        .. code-block::
            none

            6  15  24 


         

----



.. method:: Vector.pow


    Syntax:
        :code:`obj = vdest.pow(vsrc, power)`

        :code:`obj = vsrcdest.pow(power)`


    Description:
        Raise each element to some power. A power of -1, 0, .5, 1, or 2 
        are efficient. 

         

----



.. method:: Vector.sqrt


    Syntax:
        :code:`obj = vdest.sqrt(vsrc)`

        :code:`obj = vsrcdest.sqrt()`


    Description:
        Take the square root of each element. No domain checking. 

         

----



.. method:: Vector.log


    Syntax:
        :code:`obj = vdest.log(vsrc)`

        :code:`obj = vsrcdest.log()`


    Description:
        Take the natural log of each element. No domain checking. 

         

----



.. method:: Vector.log10


    Syntax:
        :code:`obj = vdest.log10(vsrc)`

        :code:`obj = vsrcdest.log10()`


    Description:
        Take the logarithm to the base 10 of each element. No domain checking. 

         

----



.. method:: Vector.tanh


    Syntax:
        :code:`obj = vdest.tanh(vsrc)`

        :code:`obj = vsrcdest.tanh()`


    Description:
        Take the hyperbolic tangent of each element. 

         

----



.. method:: Vector.abs


    Syntax:
        :code:`obj = vdest.abs(vsrc)`

        :code:`obj = vsrcdest.abs()`


    Description:
        Take the absolute value of each element. 

    Example:

        .. code-block::
            none

            objref v1 
            v1 = new Vector() 
            v1.indgen(-.5, .5, .1) 
            v1.printf() 
            v1.abs.printf() 


    .. seealso::
        :func:`abs`

         

----



.. method:: Vector.index


    Syntax:
        :code:`obj = vdest.index(vsrc,  indices)`


    Description:
        The values of the vector :code:`vsrc` indexed by the vector *indices* are collected 
        into :code:`vdest`. 
         

    Example:

        .. code-block::
            none

            objref vec, vec1, vec2, vec3 
            vec = new Vector() 
            vec1 = new Vector() 
            vec2 = new Vector() 
            vec3 = new Vector(6) 
            vec.indgen(0, 5.1, 0.1)	//vec will have 51 values from 0 to 5, with increment=0.1 
            vec1.integral(vec, 0.1)	//Euler integral of vec elements approximating 
            			//an x-squared function, dx = 0.1 
            vec2.indgen(0, 50,10) 
            vec3.index(vec1, vec2)  //put the value of every 10th index in vec2 

        makes :code:`vec3` with six elements corresponding to the integrated integers from 
        :code:`vec`. 

         

----



.. method:: Vector.min


    Syntax:
        :code:`x = vec.min()`

        :code:`x = vec.min(start, end)`


    Description:
        Return the minimum value. 

         

----



.. method:: Vector.min_ind


    Syntax:
        :code:`i = vec.min_ind()`

        :code:`i = vec.min_ind(start, end)`


    Description:
        Return the index of the minimum value. 

         

----



.. method:: Vector.max


    Syntax:
        :code:`x = vec.max()`

        :code:`x = vec.max(start, end)`


    Description:
        Return the maximum value. 

         

----



.. method:: Vector.max_ind


    Syntax:
        :code:`i = vec.max_ind()`

        :code:`i = vec.max_ind(start, end)`


    Description:
        Return the index of the maximum value. 

         

----



.. method:: Vector.sum


    Syntax:
        :code:`x = vec.sum()`

        :code:`x = vec.sum(start, end)`


    Description:
        Return the sum of element values. 

         

----



.. method:: Vector.sumsq


    Syntax:
        :code:`x = vec.sumsq()`

        :code:`x = vec.sumsq(start, end)`


    Description:
        Return the sum of squared element values. 

         

----



.. method:: Vector.mean


    Syntax:
        :code:`x =  vec.mean()`

        :code:`x =  vec.mean(start, end)`


    Description:
        Return the mean of element values. 

         

----



.. method:: Vector.var


    Syntax:
        :code:`x = vec.var()`

        :code:`x = vec.var(start, end)`


    Description:
        Return the variance of element values. 

         

----



.. method:: Vector.stdev


    Syntax:
        :code:`vec.stdev()`

        :code:`vec.stdev(start,end)`


    Description:
        Return the standard deviation of the element values. 

         

----



.. method:: Vector.stderr


    Syntax:
        :code:`x = vec.stderr()`

        :code:`x = vec.stderr(start, end)`


    Description:
        Return the standard error of the mean (SEM) of the element values. 

         

----



.. method:: Vector.dot


    Syntax:
        :code:`x = vec.dot(vec1)`


    Description:
        Return the dot (inner) product of :code:`vec` and *vec1*. 

         

----



.. method:: Vector.mag


    Syntax:
        :code:`x = vec.mag()`


    Description:
        Return the vector length or magnitude. 

         

----



.. method:: Vector.add


    Syntax:
        :code:`obj = vsrcdest.add(scalar)`

        :code:`obj = vsrcdest.add(vec1)`


    Description:
        Add either a scalar to each element of the vector or add the corresponding 
        elements of *vec1* to the elements of :code:`vsrcdest`. 
        :code:`vsrcdest` and *vec1* must have the same size. 

         

----



.. method:: Vector.sub


    Syntax:
        :code:`obj = vsrcdest.sub(scalar)`

        :code:`obj = vsrcdest.sub(vec1)`


    Description:
        Subtract either a scalar from each element of the vector or subtract the 
        corresponding elements of *vec1* from the elements of :code:`vsrcdest`. 
        :code:`vsrcdest` and *vec1* must have the same size. 

         

----



.. method:: Vector.mul


    Syntax:
        :code:`obj = vsrcdest.mul(scalar)`

        :code:`obj = vsrcdest.mul(vec1)`


    Description:
        Multiply each element of :code:`vsrcdest` either by either a scalar or the 
        corresponding elements of *vec1*.  :code:`vsrcdest` 
        and *vec1* must have the same size. 

         

----



.. method:: Vector.div


    Syntax:
        :code:`obj = vsrcdest.div(scalar)`

        :code:`obj = vsrcdest.div(vec1)`


    Description:
        Divide each element of :code:`vsrcdest` either by a scalar or by the 
        corresponding elements of *vec1*.  :code:`vsrcdest` 
        and *vec1* must have the same size. 

         

----



.. method:: Vector.scale


    Syntax:
        :code:`scale = vsrcdest.scale(low, high)`


    Description:
        Scale values of the elements of a vector to lie within the given range. 
        Return the scale factor used. 

         

----



.. method:: Vector.eq


    Syntax:
        :code:`boolean = vec.eq(vec1)`


    Description:
        Test equality of vectors.  Returns 1 if all elements of vec == 
        corresponding elements of *vec1* (to within :data:`float_epsilon`). 
        Otherwise it returns 0. 

         

----



.. method:: Vector.meansqerr


    Syntax:
        :code:`x = vec.meansqerr(vec1)`

        :code:`x = vec.meansqerr(vec1, weight_vec)`


    Description:
        Return the mean squared error between values of the elements of :code:`vec` and 
        the corresponding elements of *vec1*.  :code:`vec` and *vec1* must have the 
        same size. 
         
        If the second vector arg is present, it also must have the same size and the 
        return value is sum of :code:`w[i]*(v1[i] - v2[i])^2 / size`

         



Fourier
~~~~~~~

The following routines are based on the fast fourier transform (FFT) 
and are implemented using code from Numerical Recipes in C (2nd ed.) 
Refer to this source for further information. 
         



.. method:: Vector.correl


    Syntax:
        :code:`obj = vdest.correl(src)`

        :code:`obj = vdest.correl(src, vec2)`


    Description:
        Compute the cross-correlation function of *src* and *vec2* (or the 
        autocorrelation of *src* if *vec2* is not present). 

         

----



.. method:: Vector.convlv


    Syntax:
        :code:`obj = vdest.convlv(src,filter)`

        :code:`obj = vdest.convlv(src,filter, sign)`


    Description:
        Compute the convolution of *src* with *filter*.  If <sign>=-1 then 
        compute the deconvolution. 
        Assumes filter is given in "wrap-around" order, with countup 
        :code:`t=0..t=n/2` followed by countdown :code:`t=n..t=n/2`.  The size of *filter* 
        should be an odd <= the size of *v1*>. 

    Example:

        .. code-block::
            none

            objref v1, v2, v3 
            v1 = new Vector(16) 
            v2 = new Vector(16) 
            v3 = new Vector() 
            v1.x[5] = v1.x[6] = 1 
            v2.x[3] = v2.x[4] = 3 
            v3.convlv(v1, v2) 
            v1.printf() 
            v2.printf() 
            v3.printf() 


         

----



.. method:: Vector.spctrm


    Syntax:
        :code:`obj = vdest.spctrm(vsrc)`


    Description:
        Return the power spectral density function of vsrc. 

         

----



.. method:: Vector.filter


    Syntax:
        :code:`obj = vdest.filter(src,filter)`

        :code:`obj = vsrcdest.filter(filter)`


    Description:
        Digital filter implemented by taking the inverse fft of 
        *filter* and convolving it with *vec1*.  *vec* and *vec1* 
        are in the time 
        domain and *filter* is in the frequency domain. 

         

----



.. method:: Vector.fft


    Syntax:
        :code:`obj = vdest.fft(vsrc, sign)`

        :code:`obj = vsrcdest.fft(sign)`


    Description:
        Compute the fast fourier transform of the source data vector.  If 
        *sign*\ =-1 then compute the inverse fft. 
         
        If vsrc.\ :meth:`~Vector.size` is not an integral power of 2, it is padded with 0's to 
        the next power of 2 size. 
         
        The complex frequency domain is represented in the vector as pairs of 
        numbers --- except for the first two numbers. 
        vec.x[0] is the amplitude of the 0 frequency cosine (constant) 
        and vec.x[1] is the amplitude of the highest (N/2) frequency cosine 
        (ie. alternating 1,-1's in the time domain) 
        vec.x[2, 3] is the amplitude of the cos(2*PI*i/n), sin(2*PI*i/n) components 
        (ie. one whole wave in the time domain) 
        vec.x[n-2, n-1] is the amplitude of the cos(PI*(n-1)*i/n), sin(PI*(n-1)*i/n) 
        components. The following example of a pure time domain sine wave 
        sampled at 16 points should be played with to see where 
        the specified frequency appears in the frequency domain vector (note that if the 
        frequency is greater than 8, aliasing will occur, ie sampling makes it appear 
        as a lower frequency) 
        Also note that the forward transform does not produce the amplitudes of 
        the frequency components that goes up to make the time domain function but 
        instead each element is the integral of the product of the time domain 
        function and a specific pure frequency. Thus the 0 and highest frequency 
        cosine are N times the amplitudes and all others are N/2 times the amplitudes. 
         
        .. code-block::
            none
         
            objref box, g1, g2, g3 
            objref v1, v2, v3 
             
            proc setup_gui() { 
            box = new VBox() 
            box.intercept(1) 
            xpanel("", 1) 
            xradiobutton("sin   ", "c=0  p()") 
            xradiobutton("cos   ", "c=1  p()") 
            xvalue("freq (waves/domain)", "f", 1, "p()") 
            xpanel() 
            g1 = new Graph() 
            g2 = new Graph() 
            g3 = new Graph() 
            box.intercept(0) 
            box.map() 
            g1.size(0,N, -1, 1) 
            g2.size(0,N, -N, N) 
            g3.size(0,N, -N, N) 
            } 
            @code...	//define a gui for this example 
             
            N=16	// should be power of 2 
            c=1	// 0 -> sin   1 -> cos 
            f=1	// waves per domain, max is N/2 
            setup_gui() // construct the gui for this example 
             
            proc p() { 
            v1 = new Vector(N) 
            v1.sin(f, c*PI/2, 1000/N) 
            v1.plot(g1) 
             
            v2 = new Vector() 
            v2.fft(v1, 1)		// forward 
            v2.plot(g2) 
             
            v3 = new Vector() 
            v3.fft(v2, -1)		// inverse 
            v3.plot(g3)		// amplitude N/2 times the original 
            } 
             
            p() 

         
        The inverse fft is mathematically almost identical 
        to the forward transform but often 
        has a different operational interpretation. In this 
        case the result is a time domain function which is merely the sum 
        of all the pure sinusoids weighted by the (complex) frequency function 
        (although, remember, points 0 and 1 in the frequency domain are special, 
        being the constant and the highest alternating cosine, respectively). 
        The example below shows the index of a particular frequency and phase 
        as well as the time domain pattern. Note that index 1 is for the higest 
        frequency cosine instead of the 0 frequency sin. 
         
        Because the frequency domain representation is something only a programmer 
        could love, and because one might wish to plot the real and imaginary 
        frequency spectra, one might wish to encapsulate the fft in a function 
        which uses a more convenient representation. 
         
        Below is an alternative FFT function where the frequency 
        values are spectrum amplitudes (no need to divide anything by N) 
        and the real and complex frequency components are 
        stored in separate vectors (of length N/2 + 1). 
         
        Consider the functions 

        .. code-block::
            none
            
            FFT(1, vt_src, vfr_dest, vfi_dest)
            FFT(-1, vt_dest, vfr_src, vfi_src)
         
        The forward transform (first arg = 1) requires 
        a time domain source vector with a length of N = 2^n where n is some positive 
        integer. The resultant real (cosine amplitudes) and imaginary (sine amplitudes) 
        frequency components are stored in the N/2 + 1 
        locations of the vfr_dest and vfi_dest vectors respectively (Note: 
        vfi_dest.x[0] and vfi_dest.x[N/2] are always set to 0. The index i in the 
        frequency domain is the number of full pure sinusoid waves in the time domain. 
        ie. if the time domain has length T then the frequency of the i'th component 
        is i/T. 
         
        The inverse transform (first arg = -1) requires two freqency domain 
        source vectors for the cosine and sine amplitudes. The size of these 
        vectors must be N/2+1 where N is a power of 2. The resultant time domain 
        vector will have a size of N. 
         
        If the source vectors are not a power of 2, then the vectors are padded 
        with 0's til vtsrc is 2^n or vfr_src is 2^n + 1. The destination vectors 
        are resized if necessary. 
         
        This function has the property that the sequence 

        .. code-block::
            none

            FFT(1, vt, vfr, vfi) 
            FFT(-1, vt, vfr, vfi) 

        leaves vt unchanged. Reversal of the order would leave vfr and vfi unchanged. 
         
        The implementation is:
 

        .. code-block::
            none

            proc FFT() {local n, x 
                    if ($1 == 1) { // forward 
                            $o3.fft($o2, 1) 
                            n = $o3.size() 
                            $o3.div(n/2) 
                            $o3.x[0] /= 2	// makes the spectrum appear discontinuous 
                            $o3.x[1] /= 2	// but the amplitudes are intuitive 
             
                            $o4.copy($o3, 0, 1, -1, 1, 2)   // odd elements 
                            $o3.copy($o3, 0, 0, -1, 1, 2)   // even elements 
                            $o3.resize(n/2+1) 
                            $o4.resize(n/2+1) 
                            $o3.x[n/2] = $o4.x[0]           //highest cos started in o3.x[1 
                            $o4.x[0] = $o4.x[n/2] = 0       // weights for sin(0*i)and sin(PI*i) 
            	}else{ // inverse 
                            // shuffle o3 and o4 into o2 
                            n = $o3.size() 
                            $o2.copy($o3, 0, 0, n-2, 2, 1) 
                            $o2.x[1] = $o3.x[n-1] 
                            $o2.copy($o4, 3, 1, n-2, 2, 1) 
                            $o2.x[0] *= 2 
                            $o2.x[1] *= 2  
                            $o2.fft($o2, -1) 
                    } 
            } 

        If you load the previous example so that FFT is defined, the following 
        example shows the cosine and sine spectra of a pulse. 
 
        .. code-block::
            none
 
            objref v1, v2, v3, v4 
            objref box, g1, g2, g3, g4, b1 
             
            proc setup_gui() { 
            box = new VBox() 
            box.intercept(1) 
            xpanel("") 
            xvalue("delay (points)", "delay", 1, "p()") 
            xvalue("duration (points)", "duration", 1, "p()") 
            xpanel() 
            g1 = new Graph() 
            b1 = new HBox() 
            b1.intercept(1) 
            g2 = new Graph() 
            g3 = new Graph() 
            b1.intercept(0) 
            b1.map() 
            g4 = new Graph() 
            box.intercept(0) 
            box.map() 
            g1.size(0,N, -1, 1) 
            g2.size(0,N/2, -1, 1) 
            g3.size(0,N/2, -1, 1) 
            g4.size(0,N, -1, 1) 
            } 
            @code... 
            N=128 
            delay = 0 
            duration = N/2 
            setup_gui() 
            proc p() { 
            v1 = new Vector(N) 
            v1.fill(1, delay, delay+duration-1) 
            v1.plot(g1) 
             
            v2 = new Vector() 
            v3 = new Vector() 
            FFT(1, v1, v2, v3) 
            v2.plot(g2) 
            v3.plot(g3) 
             
            v4 = new Vector() 
            FFT(-1, v4, v2, v3) 
            v4.plot(g4) 
            } 
            p() 
             


    .. seealso::
        :func:`fft`, :func:`spctrm`

.. method:: Vector.trigavg


    Syntax:
        :code:`v1.trigavg(data,trigger,pre,post)`


    Description:
        Perform an event-triggered average of <*data*> using times given by 
        <*trigger*>. The duration of the average is from -<*pre*> to <*post*>. 
        This is useful, for example, in calculating a spike triggered stimulus 
        average. 

         

----



.. method:: Vector.spikebin


    Syntax:
        :code:`v.spikebin(data,thresh)`


    Description:
        Used to make a binary version of a spike train.  <*data*> is a vector 
        of membrane potential.  <*thresh*> is the voltage threshold for spike 
        detection.  <*v*> is set to all zeros except at the onset of spikes 
        (the first dt which the spike crosses threshold) 

         

----



.. method:: Vector.psth


    Syntax:
        :code:`vmeanfreq = vdest.psth(vsrchist,dt,trials,size)`


    Description:
        The name of this function is somewhat misleading, since its 
        input, vsrchist, is a finely-binned post-stimulus time histogram, 
        and its output, vdest, is an array whose elements are the mean 
        frequencies f_mean[i] that correspond to each bin of vsrchist. 
         
        For bin i, the corresponding mean frequency f_mean[i] is 
        determined by centering an adaptive square window on i and 
        widening the window until the number of spikes under the 
        window equals size.  Then f_mean[i] is calculated as 
         
        :code:`f_mean[i] = N[i] / (m dt trials)` 
         
        where 

        .. code-block::
            none

              f_mean[i] is in spikes per _second_ (Hz). 
              N[i] = total number of events in the window 
                       centered on bin i 
              m = total number of bins in the window 
                       centered on bin i 
              dt = binwidth of vsrchist in _milliseconds_ 
                       (so m dt is the width of the window in milliseconds) 
              trials = an integer scale factor 

         
        trials is used to adjust for the number of traces that were 
        superimposed to compute the elements of vsrchist.  In other words, 
        suppose the elements of vsrchist were computed by adding up the 
        number of spikes in n traces 

        .. math::
        
            v1.x[i] = \sum_{j=1}^n {\text{number of spikes in bin i of trace j}}

        Then trials would be assigned the value n.  Of course, if 
        the elements of vsrchist are divided by n before calling psth(), 
        then trials should be set to 1. 
         
        Acknowledgment: 
        The documentation and example for psth was prepared by Ted Carnevale. 

    .. warning::
        The total number of spikes in vsrchist must be greater than size. 

    Example:


        .. code-block::
            none

            objref g1, g2, b 
            b = new VBox() 
            b.intercept(1) 
            g1 = new Graph() 
            g1.size(0,200,0,10) 
            g2 = new Graph() 
            g2.size(0,200,0,10) 
            b.intercept(0) 
            b.map("psth and mean freq") 

            VECSIZE = 200 
            MINSUM = 50 
            DT = 1000	// ms per bin of v1 (vsrchist) 
            TRIALS = 1 
             
            objref v1, v2 
            v1 = new Vector(VECSIZE) 
               
            objref r 
            r = new Random() 
                        
               
            for (ii=0; ii<VECSIZE; ii+=1) { 
            	v1.x[ii] = int(r.uniform(0,10)) 
            } 
            v1.plot(g1) 
             
            v2 = new Vector() 
            v2.psth(v1,DT,TRIALS,MINSUM) 
            v2.plot(g2) 


         

----



.. method:: Vector.inf


    Syntax:
        :code:`v.inf(i,dt,gl,el,cm,th,res,[ref])`


    Description:
        Simulate a leaky integrate and fire neuron.  <*i*> is a vector containing 
        the input.  <*dt*> is the timestep.  <*gl*> and <*el*> are the conductance 
        and reversal potential of the leak term <*cm*> is capacitance.  <*th*> 
        is the threshold voltage and <*res*> is the reset voltage. <*ref*>, if 
        present sets the duration of ab absolute refractory period. 
         
        N.b. Currently working with forward Euler integration, which may give 
        spurious results. 

         
         

----



.. method:: Vector.resample


    Syntax:
        :code:`v1.resample(v2,rate)`


    Description:
        Resamples the vector at another rate -- integers work best. 

    .. seealso::
        :func:`copy`




