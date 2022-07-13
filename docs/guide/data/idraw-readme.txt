NAME
     idraw - drawing editor

SYNOPSIS
     idraw [options] [file]

DESCRIPTION
     Idraw is a drawing editor that  lets  you  create  and  edit
     drawings made up of graphics like text, lines, splines, rec-
     tangles, polygons, and ellipses.   Drawings  are  stored  in
     files  that can be printed on a PostScript printer.  You can
     can open an existing drawing when starting up idraw by  typ-
     ing a file name on the command line.

     Idraw displays a portrait or landscape view of an 8.5 by  11
     inch  page in its drawing area.  In a column along the draw-
     ing area's left side is a set of  drawing  tool  icons,  and
     above  the drawing area is a set of pull-down menus contain-
     ing commands.  A panner in the lower left  corner  lets  you
     pan  and  zoom the the drawing area.  Along the top is a set
     of indicators that display editing information.

DRAWING TOOLS
     You must engage a tool before you can use it.  You engage  a
     tool  by  clicking  on  its  icon or by typing the character
     below and to the right of its icon.  The icon of the drawing
     tool  that's  engaged  appears  in  inverted  colors.   Once
     engaged, you use the tool by clicking the left mouse  button
     in the drawing area.

     The Select, Move, Scale, Stretch, Rotate,  and  Alter  tools
     manipulate  existing  graphics.  Magnify makes a part of the
     view expand to fill the entire view.  Text, Line, Multiline,
     Open  Spline, Ellipse, Rectangle, Polygon, and Closed Spline
     create new graphics.  Each tool works as follows:

     Select         Select a graphic, unselecting all others.   A
                    graphic  is selected if its handles are visi-
                    ble.  Handles are small inverse-video squares
                    that either surround the graphic or demarcate
                    its important points (such as  the  endpoints
                    of  a  line). If you hold down the shift key,
                    Select extends the selection: it selects  the
                    unselected graphic (or unselects the selected
                    graphic) you clicked on but does not unselect
                    other  selections.   Clicking  anywhere other
                    than on a graphic unselects  everything;  you
                    may also drag a rubberband rectangle around a
                    group of graphics to select all  of  them  at
                    once.    Shortcut:  the  right  mouse  button
                    invokes Select while  the  mouse  is  in  the
                    drawing area.

     Move           Move  graphics  from  one  spot  to  another.
                    Shortcut:  the  middle  mouse  button invokes
                    Move while the mouse is in the drawing area.

     Scale          Scale graphics about their centers.

     Stretch        Stretch graphics vertically  or  horizontally
                    while tying down the opposite edge.

     Rotate         Rotate graphics about their centers according
                    to  the  angle  between  two  radii:  the one
                    defined by the original  clicking  point  and
                    the  one  defined  by  the  current  dragging
                    point.

     Alter          Modify a graphic's  structure.   This  tool's
                    effect is described below for each graphic.

     Magnify        Magnify a portion of the drawing specified by
                    sweeping  out a rectangular area.  Idraw will
                    magnify the area to occupy the entire screen,
                    if possible.

     Text           Create some text.  Left-click to position the
                    first  line  of  text,  and then type as much
                    text as you want.  You  may  use  emacs-style
                    keystrokes  to edit the text as well as enter
                    it.  You can leave text editing mode by  typ-
                    ing ESC or by simply clicking somewhere else.
                    The Alter tool lets you edit the text  in  an
                    existing text graphic.

     Line           Create a line.  The shift key constrains  the
                    line  to  lie  on  either the vertical or the
                    horizontal axis.  You may left-click with the
                    Alter  tool  on  either endpoint of a line to
                    move the endpoint to a new location.

     Multiline      Create a set of connected lines.   The  shift
                    key  constrains each segment to lie on either
                    the vertical or the  horizontal  axis.   Each
                    left-click starts a new segment (i.e., adds a
                    vertex); each right-click  removes  the  last
                    vertex  added.   The  middle button finalizes
                    the multiline.  The Alter tool lets you move,
                    add,  and  remove  vertices  from an existing
                    multiline.

     Open Spline    Create an open B-spline.  The shift key  con-
                    strains  each  control point to lie on either
                    the vertical or the horizontal axis with  the
                    preceding  point.   Each  left-click  adds  a
                    control point; each right-click  removes  the
                    last  control point added.  The middle button
                    finalizes the spline.  The  Alter  tool  lets
                    you move, add, and remove control points from
                    an existing open spline.

     Ellipse        Create an ellipse.  The shift key  constrains
                    the  ellipse  to  the shape of a circle.  The
                    Alter tool does not affect ellipses.

     Rectangle      Create a rectangle.  The shift key constrains
                    the  rectangle to the shape of a square.  The
                    Alter tool  lets  you  move  the  rectangle's
                    corners  independently  to  form a four-sided
                    polygon.

     Polygon        Create a polygon.  The shift  key  constrains
                    each  side  to  lie on either the vertical or
                    the horizontal axis.  Each left-click  starts
                    a  new  segment  (i.e.,  adds a vertex); each
                    right-click removes the  last  vertex  added.
                    The middle button finalizes the polygon.  The
                    Alter tool lets you  move,  add,  and  remove
                    vertices from an existing polygon.

     Closed Spline  Create a closed B-spline.  The shift key con-
                    strains  each  control point to lie on either
                    the vertical or the horizontal axis with  the
                    preceding point.  Each left-click adds a con-
                    trol point; each right-click removes the last
                    control   point  added.   The  middle  button
                    finalizes the spline.  The  Alter  tool  lets
                    you move, add, and remove control points from
                    an existing closed spline.

PULL-DOWN MENUS
     The pull-down menus File, Edit, Structure, Font, Brush, Pat-
     tern,  FgColor,  BgColor,  Align, and View above the drawing
     area contain commands for editing the drawing and  for  con-
     trolling idraw's execution.  The File menu contains the fol-
     lowing commands to operate on files:

     New            Destroy the current drawing  and  replace  it
                    with an unnamed blank drawing.

     Revert         Reread the current  drawing,  destroying  any
                    unsaved changes.

     Open...        Specify an existing drawing to edit through a
                    FileChooser(3I),  which  lets  you browse the
                    file system easily.

     Save As...     Save the current drawing in  a  file  with  a
                    specific name.

     Save           Save the current drawing in the file it  came
                    from.

     Print...       Send a PostScript version of the drawing to a
                    printer  or  to a file.  The bold rectangular
                    outline (called the page boundary)  appearing
                    in  the drawing area indicates the portion of
                    the drawing that will appear on  the  printed
                    page.

     Import Graphic...
                    Create a graphic from the  information  in  a
                    file  and insert it into the current drawing.
                    Idraw can import images  from  files  in  the
                    following formats: TIFF; PostScript generated
                    by pgmtops, ppmtops, and idraw; X bitmap for-
                    mat; and Unidraw format.

     Quit           Quit idraw.

     The Edit menu contains the following  commands  for  editing
     graphics:

     Undo           Undo the last editing operation.   Successive
                    Undo  commands undo earlier and earlier edit-
                    ing operations.

     Redo           Redo the last editing operation.   Successive
                    Redo  commands  redo  later and later editing
                    operations up to the first  operation  undone
                    by  Undo.   Undone  operations  that have not
                    been redone are lost as soon as a new  opera-
                    tion is performed.

     Cut            Remove the selected graphics from the drawing
                    and  place  them  in a temporary storage area
                    called the clipboard.

     Copy           Copy the selected  graphics  into  the  clip-
                    board.

     Paste          Paste copies of the graphics in the clipboard
                    into  the  drawing.  Together, Cut, Copy, and
                    Paste let you transfer graphics between draw-
                    ings  simply  by  cutting graphics out of one
                    view and pasting them into another.

     Duplicate      Duplicate the selected graphics and  add  the
                    copies to the drawing.

     Delete         Destroy the selected graphics.

     Select All     Select every graphic in the drawing.

     Flip Horizontal, Flip Vertical
                    Flip the selected graphics into their  mirror
                    images along the horizontal or vertical axes.

     90 Clockwise, 90 CounterCW
                    Rotate  the  selected  graphics  90   degrees
                    clockwise or counterclockwise.

     Precise Move..., Precise Scale..., Precise Rotate...
                    Move, scale,  or  rotate  graphics  by  exact
                    amounts  that  you type in a dialog box.  You
                    can specify movements in pixels, points, cen-
                    timeters,  or inches.  Scalings are specified
                    in terms of magnification factors in the hor-
                    izontal  and  vertical  dimensions. Rotations
                    are in degrees.

     The Structure menu contains the following commands to modify
     the  structure  of  the drawing, that is, the order in which
     graphics are drawn:

     Group          Nest the selected graphics in a newly created
                    picture.   A  picture  is just a graphic that
                    contains other graphics.  Group allows you to
                    build hierarchies of graphics.

     Ungroup        Dissolve any selected pictures.

     Bring To Front Bring the selected graphics to the  front  of
                    the  drawing so that they are drawn on top of
                    (after) other graphics.

     Send To Back   Send the selected graphics to the back of the
                    drawing   so   that  they  are  drawn  behind
                    (before) other graphics.

     The Font menu contains a set of fonts in  which  to  display
     text.  When you set the current font from the menu, you will
     also set all the selected graphics' fonts to that  font.   A
     font  indicator  in  the  upper  right  corner  displays the
     current font.

     The Brush menu contains a set of brushes with which to  draw
     lines.   When  you  set the current brush from the menu, you
     will also set all the selected  graphics'  brushes  to  that
     brush.   The  nonexistent  brush  draws  invisible lines and
     non-outlined graphics.  The arrowhead brushes add arrowheads
     to  either  or  both  ends  of  lines,  multilines, and open
     splines. A brush indicator in the upper left corner displays
     the current brush.

     The Pattern menu contains a set of patterns  with  which  to
     fill  graphics but not text.  Text always appears solid, but
     you can use a different color than black to get a  halftoned
     shade.   When you set the current pattern from the menu, you
     will also set all the selected graphics'  patterns  to  that
     pattern.   The  nonexistent pattern draws unfilled graphics,
     while the other patterns draw graphics filled with a  bitmap
     or a halftoned shade.

     The FgColor and BgColor menus contains a set of colors  with
     which  to  draw graphics and text.  When you set the current
     foreground or background color from the FgColor  or  BgColor
     menu,  you  will  also  set all the selected graphics' fore-
     ground or background colors.  The ``on'' bits in the bitmaps
     for  dashed lines and fill patterns appear in the foreground
     color while the ``off'' bits appear in the background color.
     A  black  and  white printer will print a halftoned shade of
     gray for any color other than black or  white.   The  brush,
     pattern, and font indicators all reflect the current colors.

     The Align menu contains  commands  to  align  graphics  with
     other  graphics.   The  first  graphic  selected stays fixed
     while the  other  graphics  move  in  the  order  they  were
     selected  according  to  the  type of alignment chosen.  The
     last Align command, Align to Grid, aligns  a  key  point  on
     each  selected  graphic to the nearest point on idraw's grid
     (see below).

     The View menu contains the following commands:

     New View       Create a duplicate idraw window containing  a
                    second  view  of  the  current  drawing.  The
                    second view may be panned, zoomed, and edited
                    independently  of  the  first.  Any number of
                    additional views may be made in this  manner.
                    Changes  made  to  a drawing through one view
                    appear synchronously in all  other  views  of
                    the  same drawing.  You may also view another
                    drawing in any idraw window via the Open com-
                    mand.

     Close View     Close the current idraw window.  Closing  the
                    last  idraw window is equivalent to issuing a
                    Quit command.

     Normal Size    Set the magnification to unity so the drawing
                    appears at actual size.

     Reduce to Fit  Reduce the magnification  until  the  drawing
                    fits entirely within the view.

     Center Page    Center the view over the center of the 8.5 by
                    11 inch page.

     Orientation    Toggle the  drawing's  orientation.   If  the
                    editor  was  formerly showing a portrait view
                    of the drawing, it will now show a  landscape
                    view of the drawing and vice versa.

     Grid on/off    Toggle idraw's grid on or off.  When the grid
                    is  on,  idraw draws a grid of equally spaced
                    points behind the drawing.

     Grid Spacing...
                    Change the grid spacing by specifying one  or
                    two  values  in  the  units  desired (pixels,
                    points,  centimeters,  or  inches).   If  two
                    values  are given (separated by a space), the
                    first specifies the  horizontal  spacing  and
                    second  the vertical spacing.  One value will
                    specify equal horizontal and  vertical  spac-
                    ing.

     Gravity on/off Toggle gravity on or off.  Gravity constrains
                    tool  operation  to  the grid, whether or not
                    the grid is visible.

X DEFAULTS
     You can customize the number of  undoable  changes  and  the
     font, brush, pattern, or color menus by setting resources in
     your  X  defaults  database.   Each  string  of   the   form
     ``idraw.resource:definition'' sets a resource.  For example,
     to customize any of the paint menus, set a resource given by
     the  concatenation of the menu's name and the entry's number
     (e.g., ``idraw.pattern8'') for each entry that you  want  to
     override.  All menus use the number 1 for the first entry.

     You must set resources only for the entries that you want to
     override,  not  all  of them.  If you want to add entries to
     the menus, simply set resources for  them.   However,  don't
     skip any numbers after the end of the menu, because the menu
     will end at the first undefined resource.  To shorten a menu
     instead  of  extending  it,  specify  a  blank string as the
     resource for the entry following the last.

     Idraw understands the following resources:

     history        Set the maximum number  of  undoable  changes
                    (20 by default).

     initialfont    Specify the  font  that  will  be  active  on
                    startup.
                     Supply a number that identifies the font  by
                    its position in the Font menu starting from 1
                    for the first entry.

     fonti          Define a custom font to use for the ith entry
                    in   the   Font  menu.   Give  three  strings
                    separated by whitespace.   The  first  string
                    defines  the  font's  name, the second string
                    the corresponding print font, and  the  third
                    string   the   print   size.    For  example,
                    ``idraw.font3:8x13bold   Courier-Bold    13''
                    defines the third font entry.

     initialbrush   Specify the brush  that  will  be  active  on
                    startup.   Give  a number that identifies the
                    brush by  its  position  in  the  Brush  menu
                    starting from 1 for the first entry.

     brushi         Define a custom brush  to  use  for  the  ith
                    entry  in  the  Brush  menu.   The definition
                    requires two numbers:  a  16-bit  hexadecimal
                    number to define the brush's line style (each
                    1 bit draws a dash and each 0 bit produces  a
                    gap),  and  a  decimal  integer to define the
                    brush's  width  in  pixels.    For   example,
                    ``idraw.brush2:ffff   1''  defines  a  single
                    pixel wide  solid  line.  If  the  definition
                    specifies  only  the string ``none'', then it
                    defines the nonexistent brush.

     initialpattern Specify the pattern that will  be  active  on
                    startup.   Give  a number that identifies the
                    pattern by its position in the  Pattern  menu
                    starting from 1 for the first entry.

     patterni       Define a custom pattern to use  for  the  ith
                    entry  in  the Pattern menu.  You can specify
                    the pattern from a 16x16 bitmap, a  8x8  bit-
                    map, a 4x4 bitmap, a grayscale number, or the
                    string ``none''.  You specify the 16x16  bit-
                    map  with sixteen 16-bit hexadecimal numbers,
                    the 8x8 bitmap with eight  8-bit  hexadecimal
                    numbers,  the 4x4 bitmap with a single 16-bit
                    hexadecimal number, and the grayscale  number
                    with  a  single  floating  point number.  The
                    floating point number must contain  a  period
                    to distinguish itself from the single hexade-
                    cimal number, and it must lie between 0.0 and
                    1.0, where 0.0 corresponds to a solid pattern
                    and 1.0 to a clear pattern.  On the  printer,
                    the  bitmap  patterns  appear as bitmaps, the
                    grayscale  patterns   appear   as   halftoned
                    shades,   and  the  ``none''  patterns  never
                    obscure any underlying graphics.   For  exam-
                    ple, ``idraw.pattern8:8421'' defines a diago-
                    nally hatched pattern.

     initialfgcolor Specify the foreground  color  that  will  be
                    active  on startup.  Give a number that iden-
                    tifies the  color  by  its  position  in  the
                    FgColor  menu  starting  from 1 for the first
                    entry.

     fgcolori       Define a custom color  to  use  for  the  ith
                    entry  in  the  FgColor  menu.  Give a string
                    defining the name of the color and optionally
                    three  decimal  numbers  between  0 and 65535
                    following the name to define the red,  green,
                    and blue components of the color's intensity.
                    The intensities override the name;  that  is,
                    idraw  will look the name up in a window sys-
                    tem database of common  colors  only  if  you
                    omit  the intensities.  You can define shades
                    of gray by using equal  proportions  of  each
                    primary       color.        For      example,
                    ``idraw.fgcolor8:Indigo   48896   0   65280''
                    defines  a color that is a mixture of red and
                    blue.

     initialbgcolor Specify the background  color  that  will  be
                    active  on startup.  Give a number that iden-
                    tifies the  color  by  its  position  in  the
                    BgColor  menu  starting  from 1 for the first
                    entry.

     bgcolori       Define a custom color  to  use  for  the  ith
                    entry  in  the  BgColor menu.  The same rules
                    apply to background colors as  to  foreground
                    colors.

WEB PAGE
	http://www.vectaport.com/ivtools/idraw.html
