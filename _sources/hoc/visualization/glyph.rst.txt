
.. _hoc_glyph:

         
Glyph
-----



.. hoc:class:: Glyph


    Syntax:
        ``g = new Glyph()``


    Description:
        Specification of a drawing. The drawing can be rendered on a Graph 
        as many times as desired using 

        .. code-block::
            none

            graph.glyph(g, x, y, scalex, scaley, angle) 

        The drawing style uses commands reminiscent of postscript. 

    .. seealso::
        :hoc:class:`Graph`, :hoc:meth:`Graph.glyph`

         

----



.. hoc:method:: Glyph.new_path


    Syntax:
        ``g = g.path()``


    Description:
        Begin a new path. 

         

----



.. hoc:method:: Glyph.move_to


    Syntax:
        ``g = g.m(x, y)``


    Description:
        Set the current point to the coordinates. 

         

----



.. hoc:method:: Glyph.line_to


    Syntax:
        ``g = g.l(x, y)``


    Description:
        A line from the current point to the coordinates. 

         

----



.. hoc:method:: Glyph.curve_to


    Syntax:
        ``g = g.curve(x,y, x1,y1, x2,y2)``


    Description:
        Draw a curve from the current point to x,y 

         

----



.. hoc:method:: Glyph.close_path


    Syntax:
        ``g = g.close()``


    Description:
        A line from the current point to the first point of the path. 

         

----



.. hoc:method:: Glyph.circle


    Syntax:
        ``g = g.circle(x, y, r)``


    Description:
        A circle at location x, y and radius r. This is implemented using
        the glyph methods new_path, move_to, curve_to, and close_path.
        Can stroke and/or fill.

    Example:
        Rotated ellipse

        .. code-block::
            none
            
            objref gr, gl
            gr = new Graph()
            
            gl = new Glyph()
            gl.circle(0,0,1)
            gl.fill(3)
            gl.s(2, 3)
            
            gr.glyph(gl, 150, 100, 30, 60, PI/4*DEG)



----



.. hoc:method:: Glyph.stroke


    Syntax:
        ``g = g.s()``

        ``g = g.s(colorindex)``

        ``g = g.s(colorindex, brushindex)``


    Description:
        Render the current path as a line. 

         

----



.. hoc:method:: Glyph.fill


    Syntax:
        ``g = g.fill()``

        ``g = g.fill(colorindex)``


    Description:
        For a closed path, fill the interior with the indicated color. 

         

----



.. hoc:method:: Glyph.control_point


    Syntax:
        ``g = g.cpt(x,y)``


    Description:
        Draw a small open rectangle at the coordinates. Intended to indicate 
        special locations on the glyph which can be selected. Not very useful 
        at this time. 

         

----



.. hoc:method:: Glyph.erase


    Syntax:
        ``g = g.erase()``


    Description:
        The drawing is empty 

         

----



.. hoc:method:: Glyph.label


    Syntax:
        ``g = g.label("string", x, y, fixtype, colorindex)``


    Description:
        Not implemented 

         

----



.. hoc:method:: Glyph.glyph


    Syntax:
        ``g = g.glyph(glyphobject, x, y, scale, angle)``


    Description:
        Not implemented 

         

----



.. hoc:method:: Glyph.gif


    Syntax:
        ``g = g.gif("filename")``


    Description:
        Reads the gif image in the file. All :hoc:class:`Glyph` arguments still work
        when the glyph contains a gif image. The gif image is drawn first so 
        other drawing specs will appear on top of it. 

    .. seealso::
        :hoc:meth:`Graph.gif`, :hoc:meth:`Graph.glyph`

         
         
         

