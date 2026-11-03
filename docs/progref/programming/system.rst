System Calls
------------

If you're using Python, many of these system calls have equivalents in
Python's os module <https://docs.python.org/3/library/os.html>`_

Path Manipulation
~~~~~~~~~~~~~~~~~

.. function:: chdir

    .. tab:: Python
    
        Syntax:
            ``n.chdir("path")``

        Description:
            Change working directory to the indicated path. Returns 0 if successful 
            otherwise -1, ie the usual unix os shell return style. 
         

    .. tab:: HOC


        Syntax:
            ``chdir("path")``
        
        
        Description:
            Change working directory to the indicated path. Returns 0 if successful 
            otherwise -1, ie the usual unix os shell return style. 
        
----

.. function:: getcwd

    .. tab:: Python
    
        Syntax:
            ``n.getcwd()``

        Description:
            Returns absolute path of current working directory in unix format. 
            The last character of the path name is '/'. 
            Must be less than 
            1000 characters long. 

    .. tab:: HOC


        Syntax:
            ``string = getcwd()``
        
        
        Description:
            Returns absolute path of current working directory in unix format. 
            The last character of the path name is '/'. 
            Must be less than 
            1000 characters long. 
        
----

.. function:: neuronhome

    .. tab:: Python
    
        Syntax:
            ``n.neuronhome()``

        Description:
            Returns the full installation path in unix format or, if it exists, the 
            ``NEURONHOME`` environment variable in unix format. 
         
            Note that for unix, it isn't exactly the installation path 
            but the 
            --prefix/share/nrn directory where --prefix is the 
            location specified during installation. For the Windows version, it is the location 
            selected during installation and the value is derived from the location 
            of ``neuron.exe`` in ``neuronhome()/bin/neuron.exe``. 
            For macOS, it is the folder that contains the neuron 
            executable program. 
         


    .. tab:: HOC


        Name:
            neuronhome -- installation path 
        
        
        Syntax:
            ``string = neuronhome()``
        
        
        Description:
            Returns the full installation path in unix format or, if it exists, the 
            NEUROHOME environment variable in unix format. 
        
        
            Note that for unix, it isn't exactly the installation path 
            but the 
            --prefix/share/nrn directory where --prefix is the 
            location specified during installation. For the mswin version it is the location 
            selected during installation and the value is derived from the location 
            of neuron.exe in neuronhome()/bin/neuron.exe. 
            For mac it is the folder that contains the neuron 
            executable program. 
        
----

Machine Identification
~~~~~~~~~~~~~~~~~~~~~~

.. seealso:: `Python's "platform" module provides access to this information and more <https://docs.python.org/3/library/platform.html>`_

.. function:: machine_name

    .. tab:: Python
    
        Syntax:
            ``n.machine_name(strdef)``

        Description:
            Sets the NEURON string (not a Python string) ``strdef`` to the hostname of the machine. 
            Create a NEURON string via, e.g., ``n.ref('')``.
    
        Example:
            .. code-block::
                python

                from neuron import n
                my_machine_name = n.ref('')
                n.machine_name(my_machine_name)
                print(f"My hostname is {my_machine_name[0]}")


    .. tab:: HOC


        Syntax:
            ``strdef name``
        
        
            ``machine_name(name)``
        
        
        Description:
            returns the hostname of the machine. 
        
----

.. function:: unix_mac_pc

    .. tab:: Python
    
        Syntax:
            ``n.unix_mac_pc()``

        Description:
            Return 1 if unix, 2 if (an older) mac, 3 if mswin, or 4 if mac osx darwin 
            is the operating system. This 
            is useful when deciding if a machine specific function can be called or 
            a dll can be loaded.

        Example:
            .. code-block::
                python

                from neuron import n
                type = n.unix_mac_pc()

                if type == 1:
                    print("This os is unix based")
                elif type == 2:
                    print("This os is classic mac based")
                elif type == 3:
                    print("This os is mswin based")
                elif type == 4:
                    print("This os is mac osx darwin based")
         


         

    .. tab:: HOC


        Syntax:
            ``type = unix_mac_pc()``
        
        
        Description:
            Return 1 if unix, 2 if mac, 3 if mswin, or 4 if mac osx darwin 
            is the operating system. This 
            is useful when deciding if a machine specific function can be called or 
            a dll can be loaded. 
        
----

.. function:: nrnversion

    .. tab:: Python
    
        Syntax:
            ``n.nrnversion()``

            ``n.nrnversion(i)``

        Description:
            Returns a string consisting of version information. 
            When this function was introduced the majorstring was "5.6" 
            and the branch string was "2004/01/22 Main (36)". 
            Now the arg can range from 0 to 6. The value of 6 returns 
            the args passed to configure. When this function was last changed 
            the return values were.


            An arg of 7 now returns a space separated string of the arguments used 
            during launch. 
            e.g. 

            .. code-block::
                none

                $ nrniv -nobanner -c 'nrnversion()' -c 'nrnversion(7)' 
                NEURON -- VERSION 7.2 twophase_multisend (534:2160ccb31406) 2010-12-09 
                nrniv -nobanner -c nrnversion() -c nrnversion(7) 
                $  

            An arg of 8 now returns the host-triplet. E.g.

            .. code-block::
              none

              $ nrniv -nobanner -c 'nrnversion(8)'
              x86_64-unknown-linux-gnu

            An arg of 9 now returns "1" if the neuron main program was launched,
            "2" if the library was loaded by Python, and "0" if the launch
            progam is unknown

            .. code-block::
              none

              $ nrniv -nobanner -c 'nrnversion(9)'
              1

            .. code-block::
              none

              $ python 2</dev/null
              >>> from neuron import n
              >>> n.nrnversion(9)
              '2'

        Example:
            .. code-block::
                python

                from neuron import n, gui
                n.nrnversion() 
                'NEURON -- VERSION 8.2.2 HEAD (93d41fafd) 2022-12-15'

                for i in range(10): 
                    print(f'{i} : {n.nrnversion(i)}')
            
                0 : 8.2.2
                1 : NEURON -- VERSION 8.2.2 HEAD (93d41fafd) 2022-12-15
                2 : VERSION 8.2.2 HEAD (93d41fafd)
                3 : 93d41fafd
                4 : 2022-12-15
                5 : 8.2.2
                6 : cmake option default differences: 'NRN_ENABLE_RX3D=OFF' 'NRN_ENABLE_CORENEURON=ON' 'NRN_ENABLE_PYTHON_DYNAMIC=ON' 'NRN_MPI_DYNAMIC=/usr/local/opt/openmpi/include;/usr/local/opt/mpich/include' 'CMAKE_BUILD_TYPE=Release' 'CMAKE_INSTALL_PREFIX=/Users/runner/work/1/s/build/cmake_install' 'CMAKE_C_COMPILER=/Applications/Xcode_13.2.1.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc' 'CMAKE_CXX_COMPILER=/Applications/Xcode_13.2.1.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++' 'PYTHON_EXECUTABLE=/Users/runner/work/1/s/nrn_build_venv38_-4745831/bin/python'
                7 : NEURON
                8 : x86_64-Darwin
                9 : 2        


    .. tab:: HOC


        Syntax:
            ``versionstring = nrnversion()``
        
        
            ``string = nrnversion(i)``
        
        
        Description:
            Returns a string consisting of version information. 
            When this function was introduced the majorstring was "5.6" 
            and the branch string was "2004/01/22 Main (36)". 
            Now the arg can range from 0 to 6. The value of 6 returns 
            the args passed to configure. When this function was last changed 
            the return values were 
        
        
            .. code-block::
                none
        
        
                oc>nrnversion() 
                NEURON -- VERSION 7.1 (296:ff4976021aae) 2009-02-27 
                oc>for i=0,6 print i,": ", nrnversion(i) 
                0 : 7.1 
                1 : NEURON -- VERSION 7.1 (296:ff4976021aae) 2009-02-27 
                2 : VERSION 7.1 (296:ff4976021aae) 
                3 : ff4976021aae 
                4 : 2009-02-27 
                5 : 296 
                6 : '--prefix=/home/hines/neuron/nrnmpi' '--srcdir=../nrn' '--with-paranrn' '--with-nrnpython' 
                oc> 
        
        
        .. warning::
            An arg of 7 now returns a space separated string of the arguments used 
            during launch. 
            e.g. 
        
        
            .. code-block::
                none
        
        
                $ nrniv -nobanner -c 'nrnversion()' -c 'nrnversion(7)' 
                NEURON -- VERSION 7.2 twophase_multisend (534:2160ccb31406) 2010-12-09 
                nrniv -nobanner -c nrnversion() -c nrnversion(7) 
                $  
        
        
            An arg of 8 now returns the host-triplet. E.g.
        
        
            .. code-block::
              none
        
        
              $ nrniv -nobanner -c 'nrnversion(8)'
              x86_64-unknown-linux-gnu
        
        
            An arg of 9 now returns "1" if the neuron main program was launched,
            "2" if the library was loaded by Python, and "0" if the launch
            progam is unknown
        
        
            .. code-block::
              none
        
        
              $ nrniv -nobanner -c 'nrnversion(9)'
              1
        
        
            .. code-block::
              none
        
        
              $ python 2</dev/null
              >>> from neuron import n
              >>> n.nrnversion(9)
              '2'
        
----

Execute a Command
~~~~~~~~~~~~~~~~~


.. function:: WinExec

    .. tab:: Python
    
        Syntax:
            ``n.WinExec("mswin command")``

        Description:
            MSWin version only. Use :func:`system` for a more generic solution, or
            use ``os.system`` or ``subprocess.run`` in Python. 
         
    .. tab:: HOC


        Syntax:
            ``WinExec("mswin command")``
        
        
        Description:
            MSWin version only. 
        
----

.. function:: system

    .. tab:: Python
    
        Name:
            system --- issue a shell command 

        Syntax:
            ``exitcode = n.system(cmdstr)``

            ``exitcode = n.system(cmdstr, stdout_str)``

        Description:
            Executes ``cmdstr`` as though it had been typed as 
            command to a unix shell from the terminal. NEURON waits until the command is 
            completed. If the second strdef arg is present, it receives the stdout stream 
            from the command. Only available memory limits the line length and 
            number of lines. 

        Example:

            ``n.system("ls")`` 
                Prints a directory listing in the console terminal window. 
                will take up where it left off when the user types the \ ``exit`` 
                command 

        .. warning::
            Fully functional on unix, mswin under cygwin, and mac osx. 
         
            Does not work on the mac os 9 version. 
         
            Following is obsolete: 
            Under mswin, executes the string under the cygwin sh.exe in :file:`$NEURONHOME/bin`
            via the wrapper, :file:`$NEURONHOME/lib/nrnsys.sh`. Normally, stdout is directed to 
            the file :file:`tmpdos2.tmp` in the working directory and this is copied to the 
            terminal. The neuron.exe busy waits until the nrnsys.sh script creates 
            a tmpdos1.tmp file signaling that the system command has completed. 
            Redirection of stdout to a file can only be done with the idiom 
            "command > filename". No other redirection is possible except by modifying 
            :file:`nrnsys.sh`. 
    
        .. note::

            A pure Python alternative would be to use ``os.system`` or ``subprocess.run``.
         

    .. tab:: HOC


        Name:
            system --- issue a shell command 
        
        
        Syntax:
            ``exitcode = system(cmdstr)``
        
        
            ``exitcode = system(cmdstr, stdout_str)``
        
        
        Description:
            Executes *cmdstr* as though it had been typed as 
            command to a unix shell from the terminal.  HOC waits until the command is 
            completed. If the second strdef arg is present, it receives the stdout stream 
            from the command. Only available memory limits the line length and 
            number of lines. 
        
        
        Example:
        
        
            \ ``system("ls")`` 
                Prints a directory listing in the console terminal window. 
                will take up where it left off when the user types the \ ``exit`` 
                command 
        
        
        .. warning::
            Fully functional on unix, mswin under cygwin, and mac osx. 
        
        
            Does not work on the mac os 9 version. 
        
        
            Following is obsolete: 
            Under mswin, executes the string under the cygwin sh.exe in :file:`$NEURONHOME/bin`
            via the wrapper, :file:`$NEURONHOME/lib/nrnsys.sh`. Normally, stdout is directed to 
            the file :file:`tmpdos2.tmp` in the working directory and this is copied to the 
            terminal. The neuron.exe busy waits until the nrnsys.sh script creates 
            a tmpdos1.tmp file signaling that the system command has completed. 
            Redirection of stdout to a file can only be done with the idiom 
            "command > filename". No other redirection is possible except by modifying 
            :file:`nrnsys.sh`. 
        
----

Timing
~~~~~~

.. function:: startsw

    .. tab:: Python
    
        Syntax:
            ``n.startsw()``


            Initializes a stopwatch with a resolution of 0.01 second. See :func:`stopsw`.


    .. tab:: HOC


        Initializes a stopwatch with a resolution of 0.01 second. See :func:`stopsw`.
        
----

.. function:: stopsw

    .. tab:: Python
    
        Syntax:
            ``n.stopsw()``

            Returns the time in seconds since the stopwatch was last initialized with a :func:`startsw` . 

        Description:
            Really the idiom 

            .. code-block::
                python

                x = n.startsw() 
                n.startsw() - x 

            should be used since it allows nested timing intervals. 


        Example:
            .. code-block::
                python

                from neuron import n
                from math import sin
                n.startsw()
                for i in range(100_000):
                    x = sin(0.2)
                print(n.stopsw())
    
        .. note::

            A pure Python alternative would be to use the ``time`` module's ``perf_counter`` function.

            .. code-block::
                python

                from neuron import n
                from math import sin
                import time

                start = time.perf_counter()
                for i in range(100_000):
                    x = sin(0.2)
            
                print(time.perf_counter() - start)




    .. tab:: HOC


        Returns the time in seconds since the stopwatch was last initialized with a :func:`startsw` .
        
        
        .. code-block::
            none
        
        
            startsw() 
            for i=1,1000000 { x = sin(.2) ] 
            stopsw() 
        
.. seealso::

    :class:`Timer`


----

Miscellaneous
~~~~~~~~~~~~~

.. function:: nrn_load_dll

    .. tab:: Python
    
        Syntax:
            ``n.nrn_load_dll(dll_file_name)``

        Description:
            Loads a dll containing membrane mechanisms (i.e., compiled MOD files).
            This works for mswin, mac, and linux. 


    .. tab:: HOC


        Syntax:
            ``nrn_load_dll(dll_file_name)``
        
        
        Description:
            Loads a dll containing membrane mechanisms. This works for mswin, mac, 
            and linux. 
        
.. function:: show_winio

    .. tab:: Python
    
        Syntax:
            ``n.show_winio(0or1)``

        Description:

            Does nothing in recent NEURON versions.

            In some older versions, could hide or show the console window
            in MSWin and Mac.

    .. tab:: HOC


        Syntax:
            ``show_winio(0or1)``
        
        
        Description:
        
        
            Does nothing in recent NEURON versions.
        
        
            In some older versions, could hide or show the console window
            in MSWin and Mac.
        
