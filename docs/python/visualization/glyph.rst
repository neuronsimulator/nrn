.. _glyph:

         
Glyph
-----



.. class:: h.Glyph()


    Specification of a drawing. The drawing can be rendered on a Graph 
    as many times as desired using 

    .. code-block::
        none

        graph.glyph(g, x, y, scalex, scaley, angle) 

    The drawing style uses commands reminiscent of postscript. 

    Example:

    Rotated ellipse

    .. code-block::
        python
    
        from neuron import h, gui
        
        gr = h.Graph()
        
        gl = h.Glyph()
        gl.circle(0,0,1)
        gl.fill(3)
        gl.s(2, 3)
        
        gr.glyph(gl, 150, 100, 30, 60, h.PI/4*h.DEG)

    .. image:: ../images/glyphcircle.png
                :align: center


    .. seealso::
        :class:`Graph`, :meth:`Graph.glyph`

         

----



.. method:: Glyph.path()


    Begin a new path. 

         

----



.. method:: Glyph.m(x, y)


    Set the current point to the coordinates. 

         

----



.. method:: Glyph.l(x, y)


    A line from the current point to the coordinates. 

         

----



.. method:: Glyph.curve(x,y, x1,y1, x2,y2)


    Draw a curve from the current point to x,y 

         

----



.. method:: Glyph.close()


    A line from the current point to the first point of the path. 

         

----



.. method:: Glyph.circle(x, y, r)


    A circle at location x, y and radius r. This is implemented using
    the glyph methods new_path, move_to, curve_to, and close_path.
    Can stroke and/or fill.

----



.. method:: Glyph.s()
            Glyph.s(colorindex)
            Glyph.s(colorindex, brushindex)

    
    Render the current path as a line. 

         

----



.. method:: Glyph.fill()
            Glyph.fill(colorindex)

    
    For a closed path, fill the interior with the indicated color. 

         

----



.. method:: Glyph.cpt(x,y)


    Draw a small open rectangle at the coordinates. Intended to indicate 
    special locations on the glyph which can be selected. Not very useful 
    at this time. 

         

----



.. method:: Glyph.erase()


    The drawing is empty 

         

----



.. method:: Glyph.label("string", x, y, fixtype, colorindex)


    Not implemented 

         

----



.. method:: Glyph.glyph(glyphobject, x, y, scale, angle)


    Not implemented 

         

----



.. method:: Glyph.gif("filename")


    Reads the gif image in the file. All :class:`Glyph` arguments still work 
    when the glyph contains a gif image. The gif image is drawn first so 
    other drawing specs will appear on top of it. 

    .. seealso::
        :meth:`Graph.gif`, :meth:`Graph.glyph`

         
         
         

