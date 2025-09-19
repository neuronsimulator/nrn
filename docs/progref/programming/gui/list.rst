.. _list:

List
----



.. class:: List

    .. tab:: Python
    
        List of objects 

        Syntax:
            ``n.List()``

            ``n.List("templatename")``


        Description:
            The ``List`` class provides useful tools for creating and manipulating lists of objects. 
            In many cases, e.g., if you have a network of cells connected at synapses and each synapse 
            is a separate object, you might want to store such information (pre-syns, post-syns, cells)
            using regular Python lists.

            Besides interacting with legacy code, the NEURON ``List`` class provides two key features
            not available in Python lists:

            1. The ability to create a dynamic ``List`` of all the object instances of a template (e.g.,
               all instances of an ``"IClamp"`` or of a cell class.)
            2. The ability to create a GUI browser for the list, which can be used to select and interact
               with the objects in the list.


            There are two ways of invoking the constructor:

            ``n.List()`` 
                Create an empty list. Objects added to the list are referenced. 
                Objects removed from the list are unreferenced. 

            ``n.List("templatename")`` 
                Create a list of all the object instances of the template. 
                These object instances are NOT referenced and therefore the list 
                dynamically changes as objects of template instances are 
                created and destroyed. Adding and removing objects 
                from this kind of list is not allowed. 

        Example:

            .. code-block::
                python

                from neuron import n

                clamps = n.IClamp(), n.IClamp(), n.IClamp()

                all_iclamps = n.List('IClamp')
                print(f'There are initially {len(all_iclamps)} IClamp objects.') # 3

                another = n.IClamp()

                print(f'There are now {len(all_iclamps)} IClamp objects.')       # 4

         

    .. tab:: HOC


        List of objects 
        
        
        Syntax:
            ``List()``
        
        
            ``List("templatename")``
        
        
        Description:
            The List class provides useful tools for creating and manipulating lists of objects. 
            For example, if you have 
            a network of cells connected at synapses and each synapse is a separate object, you may want to use 
            lists to help organize the network.  You could create one list of all pre-synaptic connections and 
            another of post-synaptic connections, as well as a list of all the connecting cells. 
        
        
            ``List()`` 
                Create an empty list. Objects added to the list are referenced. 
                Objects removed from the list are unreferenced. 
        
        
            ``List("templatename")`` 
                Create a list of all the object instances of the template. 
                These object instances are NOT referenced and therefore the list 
                dynamically changes as objects of template instances are 
                created and destroyed. Adding and  removing objects 
                from this kind of list is not allowed. 
        
----



.. method:: List.append

    .. tab:: Python
    
    
        Syntax:
            ``l.append(object)``


        Description:
            Append an object to a list, and return the number of items in list. 

         

    .. tab:: HOC


        Syntax:
            ``.append(object)``
        
        
        Description:
            Append an object to a list, and return the number of items in list. 
        
----



.. method:: List.prepend

    .. tab:: Python
    
    
        Syntax:
            ``l.prepend(object)``


        Description:
            Add an object to the beginning of a list, and return the number of objects in the list. 
            The inserted object has index=0.  Following objects have an incremented 
            index. 

         

    .. tab:: HOC


        Syntax:
            ``.prepend(object)``
        
        
        Description:
            Add an object to the beginning of a list, and return the number of objects in the list. 
            The inserted object has index=0.  Following objects have an incremented 
            index. 
        
----



.. method:: List.insrt

    .. tab:: Python
    
    
        Syntax:
            ``l.insrt(i, object)``


        Description:
            Insert an object before item *i*, and return the number of items in the list. 
            The inserted object has index *i*, following items have an incremented 
            index. 
         
            Not called :ref:`insert <keyword_insert>` because that name is a HOC keyword.

         

    .. tab:: HOC


        Syntax:
            ``.insrt(i, object)``
        
        
        Description:
            Insert an object before item *i*, and return the number of items in the list. 
            The inserted object has index *i*, following items have an incremented 
            index. 
        
        
            Not called :ref:`insert <hoc_keyword_insert>` because that name is a keyword
        
----



.. method:: List.remove

    .. tab:: Python
    
    
        Syntax:
            ``l.remove(i)``


        Description:
            Remove the object at index *i*. Following items have a decremented 
            index. ie it's often most convenient to remove items from back 
            to  front. Return the number of objects remaining in the list. 

         

    .. tab:: HOC


        Syntax:
            ``.remove(i)``
        
        
        Description:
            Remove the object at index *i*. Following items have a decremented 
            index. ie it's often most convenient to remove items from back 
            to  front. Return the number of objects remaining in the list. 
        
----



.. method:: List.remove_all

    .. tab:: Python
    
    
        Syntax:
            ``l.remove_all()``


        Description:
            Remove all the objects from the list. Return 0. 

         

    .. tab:: HOC


        Syntax:
            ``.remove_all()``
        
        
        Description:
            Remove all the objects from the list. Return 0. 
        
----



.. method:: List.index

    .. tab:: Python
    
    
        Syntax:
            ``l.index(object)``


        Description:
            Return the index of the object in the ``List``. Return a -1 if the 
            object is not in the ``List``.

            This is approximately analogous to the Python list method ``.index()``,
            except that the method for Python lists raises a ``ValueError`` if the
            object is not in the list.
         

    .. tab:: HOC


        Syntax:
            ``.index(object)``
        
        
        Description:
            Return the index of the object in the list. Return a -1 if the 
            object is not in the list. 
        
----



.. method:: List.count

    .. tab:: Python
    
    
        Syntax:
            ``l.count()``


        Description:
            Return the number of objects in the list.
        
            This is mostly useful for legacy code. A more Python solution is to just use ``len(my_list)``.

         

    .. tab:: HOC


        Syntax:
            ``.count()``
        
        
        Description:
            Return the number of objects in the list. 
        
----



.. method:: List.browser

    .. tab:: Python
    
    
        Syntax:
            ``l.browser()``

            ``l.browser("title", "strname")``

            ``l.browser("title", py_callable)``


        Description:


            ``l.browser(["title"], ["strname"])`` 
                Make the list visible on the screen. 
                The items are normally the object names but if the second arg is 
                present and is the name of a string symbol that is defined 
                in the object's     template, then that string is displayed in the list. 

            ``l.browser("title", py_callable)`` 
                Browser labels are computed. For each item, ``py_callable`` is executed 
                with ``n.hoc_ac_`` set to the index of the item. Some objects 
                notify the List when they change, ie point processes when they change 
                their location notify the list. 

        Example:

            .. code-block::
                python

                from neuron import n, gui

                my_list = n.List()

                for word in ['Python', 'HOC', 'NEURON', 'NMODL']:
                    my_list.append(n.String(word))

                my_list.browser('title', 's')   # n.String objects have an s attribute that returns the Python string


            .. image:: ../../images/list-browser1.png
                :align: center
                    
        Example of computed labels:

            .. code-block::
                python

                from neuron import n, gui

                my_list = n.List()
                for word in ['NEURON', 'HOC', 'Python', 'NMODL']:
                    my_list.append(n.String(word))

                def label_with_lengths():
                    item_id = n.hoc_ac_
                    item = my_list[item_id].s
                    return f'{item} ({len(item)})'

                my_list.browser('Words!', label_with_lengths)

            .. image:: ../../images/list-browser2.png
                :align: center

            If we now execute the following line to add an entry to the List, the new entry will appear in the browser immediately:         

            .. code-block::
                python

                my_list.append(n.String('Neuroscience'))

            .. image:: ../../images/list-browser2b.png
                :align: center

    .. tab:: HOC


        Syntax:
            ``.browser()``
        
        
            ``.browser("title", "strname")``
        
        
            ``.browser("title", strdef, "command")``
        
        
        Description:
        
        
            ``.browser(["title"], ["strname"])`` 
                Make the list visible on the screen. 
                The items are normally the object names but if the second arg is 
                present and is the name of a string symbol that is defined 
                in the object's     template, then that string is displayed in the list. 
        
        
            ``.browser("title", strdef, "command")`` 
                Browser labels are computed. For each item, command is executed 
                with :data:`hoc_ac_` set to the index of the item. On return, the
                contents of *strdef* are used as the label. Some objects 
                notify the List when they change, ie point processes when they change 
                their location notify the list. 
        
----



.. method:: List.selected

    .. tab:: Python
    
    
        Syntax:
            ``l.selected()``


        Description:
            Return the index of the highlighted object or -1 if no object is highlighted. 

        .. seealso::
            :meth:`List.browser`

         

    .. tab:: HOC


        Syntax:
            ``.selected()``
        
        
        Description:
            Return the index of the highlighted object or -1 if no object is highlighted. 
        
        
        .. seealso::
            :meth:`List.browser`
        
----



.. method:: List.select

    .. tab:: Python
    
    
        Syntax:
            ``l.select(i)``


        Description:
            Highlight the object at index *i*. 

        .. seealso::
            :meth:`List.browser`

         

    .. tab:: HOC


        Syntax:
            ``.select(i)``
        
        
        Description:
            Highlight the object at index *i*. 
        
        
        .. seealso::
            :meth:`List.browser`
        
----



.. method:: List.scroll_pos

    .. tab:: Python
    
    
        Syntax:
            ``index = list.scroll_pos()``

            ``list.scroll_pos(index)``


        Description:
            Returns the index of the top of the browser window. Sets the scroll so that 
            index is the top of the browser window. A large number will cause a scroll 
            to the bottom. 

        .. seealso::
            :meth:`List.browser`

         

    .. tab:: HOC


        Syntax:
            ``index = list.scroll_pos()``
        
        
            ``list.scroll_pos(index)``
        
        
        Description:
            Returns the index of the top of the browser window. Sets the scroll so that 
            index is the top of the browser window. A large number will cause a scroll 
            to the bottom. 
        
        
        .. seealso::
            :meth:`List.browser`
        
----



.. method:: List.select_action

    .. tab:: Python
    
    
        Syntax:
            ``l.select_action(command)``

            ``l.select_action(command, False or True)``


        Description:
            Execute a command (a Python funciton handle) when an item in the 
            list :meth:`List.browser` is selected by single clicking the mouse. 
         
            If the second arg exists and is True (or 1) then the action is only called on 
            the mouse button release. If nothing is selected at that time then 
            :data:`hoc_ac_` = -1 

        Example:

            .. code-block::
                python

                from neuron import n, gui

                my_list = n.List()

                def on_click():
                    item_id = my_list.selected()
                    if item_id >= 0: # check to make sure selection isn't dragged off
                        print(f'Item {item_id} selected ({my_list[item_id].s})')


                for word in ['Python', 'HOC', 'NEURON', 'NMODL']:
                    my_list.append(n.String(word))

                my_list.browser('title', 's')
                my_list.select_action(on_click)


            .. image:: ../../images/list-browser1.png
                :align: center
                    
         

    .. tab:: HOC


        Syntax:
            ``list.select_action("command")``
        
        
            ``list.select_action("command", 0or1)``
        
        
        Description:
            Execute a command when an item in the 
            list :meth:`List.browser` is selected by single clicking the mouse.
            :data:`hoc_ac_` contains the index when the command is executed. Thus
            ``l.select_action("action(hoc_ac_)")`` is convenient usage. 
            action will be invoked within the object context that existed when 
            ``select_action`` was called. 
        
        
            If the second arg exists and is 1 then the action is only called on 
            the mouse button release. If nothing is selected at that time then 
            :data:`hoc_ac_` = -1
        
        
        Example:
            This example shows that the object context is saved when an action is 
            registered. 
        
        
            .. code-block::
                none
        
        
                begintemplate A 
                objref this, list, obj 
                proc init() { 
                    list = new List() 
                    list.append(this) 
                    for i=0,4 { 
                            obj = new Random() 
                            list.append(obj) 
                    } 
                    list.browser() 
                    list.select_action("act(hoc_ac_)") 
                } 
                proc act() { 
                    printf("item %d selected in list of object %s\n", $1, this) 
                } 
                endtemplate A 
        
        
                objref a[2] 
                for i=0,1 a[i] = new A() 
        
----



.. method:: List.accept_action

    .. tab:: Python
    
    
        Syntax:
            ``l.accept_action(command)``


        Description:
            Execute a command (a Python function handle) when double clicking 
            on an item displayed in the list :meth:`List.browser` by the mouse. 

            Usage mirrors that of :meth:`List.select_action`.


         

    .. tab:: HOC


        Syntax:
            ``list.accept_action("command")``
        
        
        Description:
            Execute a command when double clicking 
            on an item displayed in the list :meth:`List.browser` by the mouse.
            :data:`hoc_ac_` contains the index when the command is executed. Command is
            executed within the object context that existed when ``accept_action`` 
            was called. 
        
        
        Example:
        
        
            .. code-block::
                none
        
        
                objref list, obj 
                list = new List() 
                for i=0,4 { 
                        obj = new Random() 
                        list.append(obj)  
                    obj = new List() 
                    list.append(obj) 
                } 
                list.browser() 
                list.accept_action("act()") 
                proc act() { 
                        printf("item %d accepted\n", hoc_ac_) 
                } 
        
----



.. method:: List.object

    .. tab:: Python
    
    
        Syntax:
            ``l.object(i)``

            ``l.o(i)``


        Description:
            Return the object at index *i*. 
        
            This is mostly useful for legacy code. In Python, use, e.g. ``my_list[i]`` instead.

         

    .. tab:: HOC


        Syntax:
            ``.object(i)``
        
        
            ``.o(i)``
        
        
        Description:
            Return the object at index *i*. 
        
----



.. method:: List.o

    .. tab:: Python
    
    
        Syntax:
            ``l.object(i)``

            ``l.o(i)``


        Description:
            Return the object at index *i*. 
        
            This is mostly useful for legacy code. In Python, use, e.g. ``my_list[i]`` instead.



    .. tab:: HOC


        Syntax:
            ``.object(i)``
        
        
            ``.o(i)``
        
        
        Description:
            Return the object at index *i*. 
        
