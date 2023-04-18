
.. _hoc_geometry:


.. _hoc_geometry_Section:
         
Conceptual Overview of Sections
-------------------------------

Sections are unbranched lengths of continuous cable connected together to form 
a neuron. Sections can be connected to form 
any tree-shaped structure but loops are not permitted. (You may, however, 
develop membrane mechanisms, such as electrical gap junctions 
which do not have the loop restriction. But be aware that the electrical 
current flows through such connections are calculated by a modified euler 
method instead of the more numerically robust fully implicit/crank-nicholson 
methods) 
 
Do not confuse sections with segments. Sections are divided into segments 
of equal length for numerical simulation purposes (see :hoc:data:`nseg`).
NEURON uses segments to represent the electrical circuit shown below. 

.. code-block::
    none

     
              Ra 
    o/`--o--'\/\/`--o--'\/\/`--o--'\/\/`--o--'\o v 
         |          |          |          | 
        ---        ---        ---        --- 
       |   |      |   |      |   |      |   | 
        ---        ---        ---        --- 
         |          |          |          | 
    -------------------------------------------- ground 
     

Such segments are similar to 
compartments in compartmental modeling programs. 
         

----



.. _hoc_geometry_geometry:

Geometry
~~~~~~~~

Section geometry is used to compute the area and axial resistance of each segment. 
 
There are two ways to specify section geometry: 

1) The stylized method simply specifies parameters for length and diameter. 
2) The 3-D method specifies 
   a section's shape, orientation, and location in three dimensions. 
 
Choose the stylized method if the notions of cable length and diameter 
are authoritative and where 3-d shape is irrelevant. For plotting purposes, 
length and diameter will be used to generate 3-d info automatically for 
a stylized straight cylinder. (see :hoc:func:`define_shape`)
 
Choose the 3-D method if the shape comes from 3-d reconstruction data 
or if your 3-d visualization is paramount. This method makes the 3-d info 
authoritative and automatically 
determines the abstract cable's length and diameter. 
With this method, you may change a section's length/diameter only by 
changing its 3-d info. (but see :hoc:func:`pt3dconst`)
 
Stylized specification of geometry
==================================

For simulations one needs to specify L, nseg, diam, Ra, and connectivity. 


L 
    For each section, L is the length of the entire section in microns. 

nseg 
    The section is divided into nseg compartments of length L/nseg. 
    Membrane potential will be computed at the ends of the section and the 
    middle of each compartment. 

diam 
    The diameter in microns. 
    Note that diam is a range variable and 
    therefore must be respecified whenever :hoc:data:`nseg` is changed.

Ra 
    Axial resistivity in ohm-cm. 

connectivity 
    This is established with the :ref:`connect <hoc_keyword_connect>` command and defines the
    parent of the section, which end of the section 
    is attached to the parent, and where on the parent the 
    attachment takes place. To avoid confusion, it is best to attach the 
    0 end of a section to the 1 end of its parent. 

 
In the stylized specification, the shape model used for a section is 
a sequence of right circular cylinders of length, L/nseg, with diameter 
given by the diam range variable at the center of each segment. 
The area of a segment is PI*diam*L/nseg (micron2) and the half-segment axial 
resistance is \ ``.01*Ra*(L/2/nseg)/(PI*(diam/2)^2)``. The .01 factor is necessary 
to convert ohm-cm micron/micron2 to MegOhms. Ends of cylinders are not 
counted in the area and, in fact, the areas are very close to those of 
truncated cones as long as the diameter does not change too much. 


.. code-block::
    none

    forall delete_section() 
    create a 
    access a 
    nseg = 11 
    Ra = 100 
    L=1000 
    diam(0:1)=11:100 
    for (x) print x, diam(x), area(x), PI*diam(x)*L/nseg, ri(x), .01*Ra*(L/2/nseg)/(PI*(diam(x)/2)^2) 

Note that the area (and length) of the 0,1 terminal ends is equal to 0 
and the axial resistance 
is the sum of the adjacent half-segment resistances between segment and 
parent segment. Such, niceties allow the spatial discretization error to 
be proportional to \ ``(1/nseg)^2``. However, for second order correctness, 
all point processes must be located at the center of the segments or at the 
ends and all branches should be connected at the ends or centers of segments. 
Note that if one increases nseg by a factor of 3, old centers are preserved. 
 
For single compartment simulations it is most convenient to choose 
a membrane area of 100 micron2 so that point process currents (nanoamps) 
are equivalent to density currents (milliamps/cm2). 
 
Also note that a single compartment of length = diameter has the same 
effective area as that of a sphere of the same diameter. 
     

Example:
    The following example demonstrates the automatic 3-d shape construction. 
    The root section "a" is drawn with its 0 end (left) at the origin and is colored 
    red. 
     
    Sections connected to its 1 end (sections b, c, d) 
    get drawn from left to right. Sections 
    descended from the 0 end (section e) of the root get drawn from right to left. 
     
    Especially note the diameter pattern of section c whose "1" end is connected 
    to the "b" parent. You don't have to understand this if you always connect 
    the "0" end to the parent. 
     


    .. code-block::
        none

        forall delete_section() 
        create a, b, c, d, e 
        connect b(0), a(1) 
        connect c(1), b(1) 
        connect d(0), b(1) 
        connect e(0), a(0) 
        forall nseg=21 
        forall L=100 
        forall diam(0:1) = 10:40 
         
        objref s 
        s = new Shape() 
        s.show(0) 
        a s.color(2) 
        topology() 
        finitialize() 
        forall { 
        	print secname() 
        	for i=0,n3d()-1 print i, x3d(i), y3d(i), z3d(i), diam3d(i) 
        } 

     
    If you change the diameter or length, the Shape instances are 
    automatically redrawn or when :hoc:func:`doNotify` is called.
    Segment area and axial resistance will be automatically recomputed prior 
    to their use. 
     
    Under some circumstances, involving nonlinearly varying diameters across 
    a section, 
    at first sight surprising results can occur 
    when the stylized method is used and a Shape instance is created. 
    This is because under a define_shape() with no pre-existing 
    3-d points in a section, a number of 3-d points is created equal to 
    the number of segments plus the end areas. When 3-d points exist, 
    they determine the calculation of L, diam, area, and ri. Thus diam 
    can change slightly merely due to shape creation. When 
    L and diam are changed, there is first a change to the 3-d points and 
    then L and diam are updated to reflect the actual values of these 
    3-d points. Due to multiple interpolation effects, specifying a nonlinearly 
    varying diam will, in general, not give exactly the same diameter values as the 
    case where no 3-d information exists. This effect is illustrated in the 
    following example 



    .. code-block::
        none

        forall delete_section() 
        objref s 
        proc pr() { 
        pt3dclear() 
        nseg = $1 
        diam = 10 
        diam(.34:.66) = 20:20 
        define_shape() 
        for(x) print x*L, diam(x), area(x), ri(x) 
        } 
         
        xpanel("change nseg") 
        xradiobutton("nseg = 3", "pr(3)") 
        xradiobutton("nseg = 11", "pr(11)") 
        xradiobutton("nseg = 101", "pr(101)") 
        xpanel() 


        create a 
        access a  
        nseg=3 
        {Ra=100 L=100} 
         
         
        diam=10 
        diam(.34:.66) = 20:20 
         
        for(x) print x*L, diam(x), area(x), ri(x) 
         
        s = new Shape() 
        s.show(0) 
         
        for i=0, n3d()-1 print i, arc3d(i), diam3d(i) 
        print "L=", L 
        for(x) print x*L, diam(x), area(x), ri(x) 
         

The difference is that the 3-d points define a series of truncated cones 
instead of a series of right circular cylinders. The difference is reduced 
with larger nseg. With the stylized method, abrupt 
changes in diameter should only take place at the 
boundaries of sections if you wish to view shape and also make use of 
the fewest possible number of segments. But remember, end area of the 
abrupt changes is not calculated. For that, you need an explicit pair 
of 3-d points with the same location and different diameters. 
     
3-D specification of geometry
=============================
3-d information for a section is kept in a list of (x,y,z,diam) "points". 
The first point is associated with the end of the section that is connected 
to the parent (NB: Not necessarily the 0 end) and the 
last point is associated with the opposite end. There must be at least two 
points and they should be ordered in terms of monotonically increasing 
arc length. 
 
The root section is treated as the origin of the cell with respect to 
3-d position.  When any section's 3-d shape or length changes, all the 
sections in the child trees have their 3-d information translated to 
correspond to the new position.  So, assuming the soma is the root 
section, to translate an entire cell to another location it suffices to 
change only the location of the soma.  It will avoid confusion if, 
except for good reason, one attaches only the 0 end of a child section 
to a parent.  This will ensure that the diam(x) as x ranges from 0 to 1 
has the same sense as diam3d(i) as i ranges from 0 to n3d()-1. 
 
The shape model used for a section  when the pt3d list is non-empty 
is that of a sequence of truncated cones in which the pt3d points define 
the location and diameter of the ends. From this sequence of points, 
the effective area, diameter, and resistance is computed for each segment 
via a trapezoidal integration across the segment length. This takes 
into account the extra area due to \ ``sqrt(dx^2 + dy^2)`` for fast changing 
diameters (even degenerate cones of 0 length can be specified, ie. two 
points with same coordinates but different diameters) 
but no attempt is made to deal with centroid curvature effects 
on the area. Note that the number of 3d points used to describe a shape 
has nothing to do with nseg and does not affect simulation speed. 
(Although, of course, it does affect how fast one can draw the shape) 
 

Example:
    The following illustrates the notion of the 3-d points as describing 
    a sequence of cones. Note that the segment area and resistance is 
    different than the 
    simplistic calculation used in the stylized method. In this case 
    the area of the segment has very little to do 
    with the diameter of the center of the segment. 
    



    .. code-block::
        none

        forall delete_section() 

        create a 
        access a 
        Ra=100 
        nseg = 11 
        pt3dclear() 
        for i=0,30 { 
        	x = PI*i/30 
        	pt3dadd(200*sin(x), 200*cos(x), 0, 100*sin(4*x)) 
        } 
        objref s 
        s = new Shape() 
        s.show(0) 
        print L 
        for (x) print x, diam(x), area(x), PI*diam(x)*L/nseg, ri(x), .01*Ra*(L/2/nseg)/(PI*(diam(x)/2)^2) 

    Note that at one point the diameter is numerically 0 and 
    the axial resistance becomes 
    essentially infinite thus decoupling the adjacent segments. Take care to 
    avoid constructing spheres with a beginning and ending diameter of 0. 
    No current 
    would flow from the end to a connecting section. The end diameter should be 
    the diameter of the end of the connecting section. 
     
    The following loads the pyramidal cell 3-d reconstruction from the demo 
    directory of your neuron system. 
    Notice that you can modify the length only if the pt3dconst mode is 1. 


    .. code-block::
        none

        forall delete_section() 

        xopen("$(NEURONHOME)/demo/pyramid.nrn") 
        mode = 1 
        pt3dconst(mode) 
        objref s 
        s = new Shape() 
        s.action("dendrite_1[8] s.select()") 
         
        dendrite_1[8] s.color(2) 
         
        xpanel("Change Length") 
        xvalue("dendrite_1[8].L", "dendrite_1[8].L", 1) 
        xcheckbox("Can't change length", &mode, "pt3dconst(mode)") 
        xpanel() 


.. seealso::
    :hoc:func:`pt3dclear`, :hoc:func:`pt3dadd`, :hoc:func:`pt3dconst`, :hoc:func:`pt3dstyle`, :hoc:func:`n3d`, :hoc:func:`x3d`, :hoc:func:`y3d`, :hoc:func:`z3d`, :hoc:func:`diam3d`, :hoc:func:`arc3d`
    :hoc:func:`getSpineArea`, :hoc:func:`setSpineArea`, :hoc:func:`spine3d`

     

.. seealso::
    :hoc:func:`define_shape`, :hoc:func:`pt3dconst`

 
If 3-D shape is not an issue it is sufficient to specify the section variables 
L (length in microns),  Ra (axial resistivity in ohm-cm), and the range variable 
diam (diameter in microns). 
 
A list of 3-D points with corresponding diameters describes the geometry 
of a given section. 
     

----


Defining the 3D Shape
---------------------



.. hoc:function:: pt3dclear


    Syntax:
        ``buffersize =  pt3dclear()``

        ``buffersize =  pt3dclear(buffersize)``


    Description:
        Destroy the 3d location info in the currently accessed section. 
        With an argument, that amount of space is allocated for storage of 
        3-d points in that section. 

         

----



.. hoc:function:: pt3dadd


    Syntax:
        ``pt3dadd(x,y,z,d)``


    Description:
         
        Add the 3d location and diameter point at the end of the current pt3d 
        list. Assume that successive additions increase the arc length 
        monotonically. When pt3d points exist in a section they are used 
        to compute *diam* and *L*. When *diam* or *L* are changed and \ ``pt3dconst()==0`` 
        the 3-d info is changed to be consistent with the new values of 
        *L* and *diam*. (Note: When *L* is changed, \ ``diam_shape()`` should be executed 
        to adjust the 3-d info so that branches appear connected.) 
        The existence of a spine at this point is signaled 
        by a negative value for *d*. 

         

----



.. hoc:function:: pt3dconst


    Syntax:
        ``pt3dconst(0)``

        ``pt3dconst(1)``


    Description:
        If \ ``pt3dconst`` is set at 0, newly assigned values for *d* and *L* will 
        automatically update pre-existing 3d information. 
        \ ``pt3dconst`` returns its previous state on each call. Its original value is 0. 
         
        Note that the *diam* information transferred to the 3d point information 
        comes from the current diameter of the segments and does not change 
        the number of 3d points.  Thus if there are a lot of 3d points the 
        shape will appear as a string of uniform diameter cylinders each of 
        length L/nseg. ie. after transfer \ ``diam3d(i) == diam(arc3d(i))``. 
        Then, after a call to an internal function such as \ ``area()`` or 
        \ ``finitialize()``, the 3d point info will be used to determine the values 
        of the segment diameters. 
         
        Because of the three separate interpolations: 
        hoc range spec -> segment diameter -> 3d point diam -> segment diameter, 
        the final values of the segment diameter may be different from the 
        case where 3d info does not exist. 
         
        Because of the surprises noted above, when using 3d points 
        consider treating them as the authoritative diameter info and set 
        \ ``pt3dconst(1)``. 
         
        3d points are automatically generated when one uses 
        the nrniv Shape class. If you want the flexibility of being able 
        to specify 3d diameter using range variable notation 
        (eg diam(0:1) = 10:20) you will need to experiment with \ ``nseg`` and 
        \ ``n3d()`` in order to understand the exact consequences of interpolation. 

    .. seealso::
        :hoc:func:`pt3dstyle`

         

----



.. hoc:function:: pt3dstyle


    Syntax:
        ``style = pt3dstyle()``

        ``style = pt3dstyle(0)``

        ``style = pt3dstyle(1, x, y, z)``

        ``style = pt3dstyle(1, &x, &y, &z)``


    Description:
        With no args, returns 1 if using a logical connection point. 
         
        With a first arg of 0, then style is NO logical connection point 
        and (with :hoc:func:`pt3dconst` == 0 and define_shape is executed)
        the 3-d location info is translated so the first 3-d point coincides with 
        the parent connection location. This is the classical and default behavior. 
         
        With a first arg of 1 and x,y,z value arguments, those values are used 
        to define a logical connection point relative to the first 3-d point. 
        When :hoc:func:`pt3dconst` == 0 and define_shape is executed, the 3-d location
        info is translated so that the logical connection point coincides 
        with the parent connection location. Note that logical connection points 
        have absolutely no effect on the electrical properties of the structure since 
        they do not affect the length or area of a section. 
        They are useful mostly for accurate visualization of a dendrite connected 
        to the large diameter edge of a soma that happens to be far from the 
        soma centroid. The logical connection point should be set to the location 
        of the parent centroid connection, i.e. most often the 0.5 location 
        of the soma. Note, that under translation and scaling, 
        the relative position between 
        the logical connection point and the first 3-d point is preserved. 
         
        With a first arg of 1 and x,y,z reference arguments, the x,y,z variables 
        are assigned the values of the logical connection point (if the style 
        in fact was 1). 

    .. seealso::
        :hoc:func:`pt3dconst`, :hoc:func:`define_shape`

         

----



.. hoc:function:: pt3dinsert


    Syntax:
        ``pt3dinsert(i, x, y, z, diam)``


    Description:
        Insert the point (so it becomes the i'th point). If i is equal to 
        :hoc:func:`n3d` the point is appended (equivalent to :hoc:func:`pt3dadd`).

         

----



.. hoc:function:: pt3dremove


    Syntax:
        ``pt3dremove(i)``


    Description:
        Remove the i'th point. 

         

----



.. hoc:function:: pt3dchange


    Syntax:
        ``pt3dchange(i, x, y, z, diam)``

        ``pt3dchange(i, diam)``


    Description:
        Change the i'th 3-d point info. If only two args then the second arg 
        is the diameter and the location is unchanged. 

        .. code-block::
            none

            pt3dchange(5, x3d(5), y3d(5), z3d(5), (spine3d(5)+1)/2 * diam3d(5)) 

        leaves the pt3d info unchanged. 

         

----


Reading 3D Data from NEURON
---------------------------

.. hoc:function:: n3d


    Syntax:
        ``n3d()``


    Description:
        Return the number of 3d locations stored in the currently accessed section. 

         

----



.. hoc:function:: x3d


    Syntax:
        ``x3d(i)``


    Description:
        Returns the x coordinate of the ith point in the 3-d list of the 
        currently accessed section. 

    .. seealso::
        :hoc:func:`y3d`, :hoc:func:`z3d`, :hoc:func:`arc3d`, :hoc:func:`diam3d`


----



.. hoc:function:: y3d


    Syntax:
        ``y3d(i)``


    .. seealso::
        :hoc:func:`x3d`


----



.. hoc:function:: z3d


    Syntax:
        ``z3d(i)``


    .. seealso::
        :hoc:func:`x3d`

         

----



.. hoc:function:: diam3d


    Syntax:
        ``diam3d(i)``


    Description:
        Returns the diameter of the ith 3d point of the currently accessed 
        section. 
        \ ``diam3d(i)`` will always be positive even 
        if there is a spine at the ith point. 

    .. seealso::
        :hoc:func:`spine3d`


----



.. hoc:function:: arc3d


    Syntax:
        ``arc3d(i)``


    Description:
        This is the arc length position of the ith point in the 3d list. 
        \ ``arc3d(n3d()-1) == L`` 

         

----



.. hoc:function:: spine3d


    Syntax:
        ``spine3d(i)``


    Description:
        Return 0 or 1 depending on whether a spine exists at this point. 

         

----



.. hoc:function:: setSpineArea


    Syntax:
        ``setSpineArea(area)``


    Description:
        The area of an average spine in um2. \ ``setSpineArea`` merely adds to 
        the total area of a segment. 

         

----



.. hoc:function:: getSpineArea


    Syntax:
        ``getSpineArea()``


    Description:
        Return the area of the average spine. 

         

----



.. hoc:function:: define_shape


    Syntax:
        ``define_shape()``


    Description:
        Fill in empty pt3d information with a naive algorithm based on current 
        values for *L* and *diam*. Sections that already have pt3d info are 
        translated to ensure that their first point is at the same location 
        as the parent. But see :hoc:func:`pt3dstyle` with regard to the use of
        a logical connection point if the translation ruins the 
        visualization. 
         
        Note: This may not work right when a branch is connected to 
        the interior of a parent section \ ``0 < x < 1``, 
        rather only when it is connected to the parent at 0 or 1. 

         

----



.. hoc:function:: area


    Syntax:
        ``area(x)``


    Description:
        Return the area (in square microns) of the segment which contains *x*. 
         
        \ ``area(0)`` and \ ``area(1)`` = 0 

         

----



.. hoc:function:: ri


    Syntax:
        ``ri(x)``


    Description:
        Return the resistance (in megohms) between the center of the segment containing x 
        and its parent segment. This can be used to compute axial current 
        given the voltage at two adjacent points. If there is no parent 
        the "infinite" resistance returned is 1e30. 
         

    Example:

        .. code-block::
            none

            for (x) print x, area(x), ri(x) 

        will print the arc length, the segment area at that arc length, and the resistance along that length 
        for the currently accessed section. 

         
         

----



.. hoc:function:: distance


    Syntax:
        ``distance() or distance(0, x)``

        ``len = distance(x) or len = distance(1, x)``



    Description:
        Compute the path distance between two points on a neuron. 
        If a continuous path does not exist the return value is 1e20. 
         


        \ ``distance()`` with no arguments 
            specifies the origin as location 0 
            of the currently accessed section. 

        \ ``distance(x) (0<=x<=1)`` 
            returns the distance (in microns) from the origin to 
            this point on the currently accessed section. 

         
        To overcome the 
        old initialization restriction, distance(0, x) can be used to set the 
        origin. Note that distance is measured from the centers of 
        segments. 

    Example:
        .. code-block::
            none

            create a, b
            connect b(0), a(1)
            a { L = 1000  nseg = 5 }
            b { L = 200   nseg = 5 }
            
            { a distance(0, 0.5) } // origin is center of a
            b print distance(0.0) // 500
            b print distance(0.5) // 600
            b print distance(1.0) // 700

    .. warning::
        When subtrees are connected by :hoc:meth:`ParallelContext.multisplit` , the
        distance function returns 1e20 if the path spans the split location. 

    .. seealso::
        :hoc:class:`RangeVarPlot`

         
         

----



.. hoc:data:: diam_changed


    Syntax:
        ``diam_changed``


    Description:
        Signals the system that the coefficient matrix needs to be 
        recalculated. 
         
        This is not needed since \ ``Ra`` is now a section variable 
        and automatically sets diam_changed whenever any sections Ra is 
        changed. 
        Changing diam or any pt3d value will cause it to be set automatically. 

         
         

----



.. hoc:data:: L

        Length of a section in microns. 
         

----



.. hoc:data:: diam

        Diameter range variable of a section in microns. 
         

----



.. hoc:data:: Ra


    Syntax:
        ``Ra``


    Description:
        Axial resistivity in ohm-cm. This used to be a global variable 
        so that it was the same for all sections. Now, it is a section 
        variable and must be set individually for each section. A simple 
        way to set its value is 
        \ ``forall Ra=35.4`` 
         
        Prior to 1/6/95 the default value for Ra was 34.5. Presently it is 
        35.4. 

         

