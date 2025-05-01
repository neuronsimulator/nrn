.. _bbsavestate:

BBSaveState
-----------



.. class:: BBSaveState

    A more flexible, cell centered version of :class:`SaveState`

    The goal is to be able to save state to a file and restore state when the
    save and restore contexts have different numbers of processors, different
    distribution of gids, and different splitting. It needs to work efficiently
    in the context of many GByte file sizes and many thousands of processors.
    
    The class was originally developed with the needs of the BlueBrain
    neocortical model in mind.

    The following code fragment illustrates a basic test. When 'restore'
    is False, the simulation run stops half way and saves the state and then
    continues. When 'restore' is True, the simulation begins at the previous
    save time and continues to tstop.

    .. code-block::
       python

        def prun(tstop, restore=False):
          pc.set_maxstep(10)
          h.stdinit()
          bbss = h.BBSaveState()
          if restore:
            bbss.restore("temp.dat")
            print(f'after restore t={h.t}')
          else:
            pc.psolve(tstop/2)
            bbss.save("temp.dat")
          pc.psolve(tstop)

    In this case, the entire model state for this MPI rank is in the filename for save and restore.

    If multisplit is involved, or it is desired to reassemble the model cells on a different set of MPI ranks,
    one instead must use :meth:`BBSaveState.save_test`
    and :meth:`BBSaveState.restore_test`. This allows reassembly of the multisplit subtrees back into
    their complete cells to allow different multisplitting and different cell distribution on different ranks.
    In this case
    files are saved in a subdirectory called "bbss_out" and restored
    from a subdirectory called "bbss_in". A script filter
    (see :meth:`BBSaveState.save_test`) is needed to copy and sometimes
    concatenate files from the bbss_out to the bbss_in subfolders. These files have
    an ascii format.

    BBSaveState has a c++ API that allows one to replace the file reader and
    writer. See `nrn/src/nrniv/bbsavestate.cpp <https://github.com/neuronsimulator/nrn/blob/master/src/nrniv/bbsavestate.cpp>`_
    for a description of this API.
    The undocumented methods, ``save_test_bin`` and ``restore_test_bin`` demonstrate
    the use of this API.

    The user can mark a point process IGNORE by calling the method
    bbss.ignore(point_process_object)
    on all the point processes to be ignored.
    The internal list of ignored point processes can be cleared by calling
    bbss.ignore()
    
    Because a restore clears the event queue and because one cannot call
    finitialize from hoc without vitiating the restore, :meth:`Vector.play` will
    not work unless one calls :meth:`BBSaveState.vector_play_init` after a
    restore (similarly :func:`frecord_init` must be called for :meth:`Vector.record` to work.
    Note that it is necessary that Vector.play use a tvec argument with
    a first element greater than or equal to the restore time.
    
    Some model restrictions:

    1. "Real" cells must have gids.
    2. Artificial cells can have gids. If not they must be directly connected
       to just one synapse (e.g. NetStim -> NetCon -> synapse).
    3. There is only one spike output port per cell and that is associated
       with a base gid.
    4. NetCon.event in Hoc can be used only with NetCon's with a None source.

    RANDOM variables declared in a mod file NEURON block have their sequence value saved
    automatically.

    To allow extra state to be saved, eg. when POINTER and BBCOREPOINTER are used to manage objects,
    declare  FUNCTION bbsavestate() within the mechanism.
    That function is called when the
    mechanism instance is saved and restored.
    FUNCTION bbsavestate takes two pointers to double arrays
    xdir and xval.
    The first double array, xdir, has length 1 and xdir[0] is -1.0, 0.0, or 1.0 
    If xdir[0] == -1.0, then replace the xdir[0] with the proper number of elements
    of xval and return 0.0.  If xdir[0] == 0.0, then save double values into
    the xval array (which will be sized correctly from a previous call with
    xdir[0] == -1.0). If xdir[0] == 1.0, then the saved double values are in
    xval and should be restored to their original values.
    The number of elements saved/restored has to be apriori known by the instance
    since the size of the xval that was saved is not sent to the instance on
    restore.
    
    For example

    .. code-block::
       C++

          FUNCTION bbsavestate() {
            bbsavestate = 0
          VERBATIM
            double *xdir, *xval, *hoc_pgetarg();
            xdir = hoc_pgetarg(1);
            if (*xdir == -1.) { *xdir = 2; return 0.0; }
            xval = hoc_pgetarg(2);
            if (*xdir == 0.) { xval[0] = 20.; xval[1] = 21.;}
            if (*xdir == 1) { printf("%d %d\n", xval[0]==20.0, xval[1] == 21.0); }
          ENDVERBATIM
          }

----



.. method:: BBSaveState.save_test


    Syntax:
        ``.save_test()``


    Description:
        State of the model is saved in files within the subdirectory, `bbss_out`.
        The file `bbss_out/tmp` contains the value of t. Other files have the
        filename format tmp.<gid>.<rank> . Only in the case of multisplit
        is it possible to have the same gid in more than one filename. Note
        that the out folder needs to be created by the user prior to a call
        to save_test().

        To prepare for a restore, the tmp.<gid>.<rank> files should be copied
        from the `bbss_out` subfolder to a subfolder called `bbss_in`, with the filename
        bbss_in/tmp.<gid> . Each file should begin with a first line that specifies
        the number of files in the `out` folder that had the same gid.

        The following out2in.sh script shows how to do this (not particularly
        efficiently).

        .. code-block::
          bash

          #!/usr/bin/env bash
          rm -f bbss_in/*
          cat bbss_out/tmp > bbss_in/tmp
          for f in bbss_out/tmp.*.* ; do
            echo $f
            i=`echo "$f" | sed 's/.*tmp\.\([0-9]*\)\..*/\1/'`
            echo $i
            if test ! -f bbss_in/tmp.$i ; then
              cnt=`ls bbss_out/tmp.$i.* | wc -l`
              echo $cnt > bbss_in/tmp.$i
              cat bbss_out/tmp.$i.* >> bbss_in/tmp.$i
            fi
          done


----



.. method:: BBSaveState.restore_test


    Syntax:
        ``.restore_test()``



    Description:
        State of the model is restored from files within the
        subdirectory, "bbss_in". The file "bbss_in/tmp" supplies the value of t.
        Other files have the filename format tmp.<gid> and are read when
        that gid is restored. Note that in a multisplit context, the same
        "bbss_in/tmp.<gid>" file will be read by multiple ranks, but only the state
        assocated with sections that exist on a rank will be restored.

----


.. method:: BBSaveState.save

  Syntax:
    ``.save("filename")``

  Description:
    Saves the state of the entire model (on this rank). This is simpler to use than the ``save_test``, ``restore_test``
    pattern but does not work if one has multisplit cells or desires a different distribution of cells on a different
    number of ranks.


----


.. method:: BBSaveState.restore

  Syntax:
    ``.restore("filename")``

  Description:
    Restores the state of the entire model (on this rank). This is simpler to use than the ``save_test``, ``restore_test``
    pattern but does not work if one has multisplit cells or desires a different distribution of cells on a different
    number of ranks.

----

.. method:: BBSaveState.ignore


    Syntax:
        ``.ignore(ppobj)``


    Description:

       Point processes can be marked IGNORE
       which will skip them on save/restore.
       The internal list of these ignored point processes must be the same
       on save and restore.

----

.. method:: BBSaveState.vector_play_init


    Syntax:
        ``.vector_play_init()``


    Description:
        Allow :meth:`Vector.play` to work. Call this method after a restore
        if there are any Vector.play in the model.
