
.. _hoc_notify:

Notification
------------



.. hoc:function:: doEvents


    Syntax:
        ``doEvents()``


    Description:
        Deal with any pending events in the middle of a computation. 
        This also causes any previous flushes to make their effect felt on 
        the screen. 
         
        Faster than :hoc:func:`doNotify` since field editors are not updated.

         

----



.. hoc:function:: doNotify


    Syntax:
        ``doNotify()``


    Description:
        All panels are updated so field editors show current values. 
        This is slower than :hoc:func:`doEvents` which does not check the field editors.


