System Calls
------------

.. seealso:: `For most of these system calls, check Python's os module <https://docs.python.org/3/library/os.html>`_

Path Manipulation
~~~~~~~~~~~~~~~~~

.. function:: chdir

    Syntax:
        ``h.chdir("path")``

    Description:
        Change working directory to the indicated path. Returns 0 if successful 
        otherwise -1, ie the usual unix os shell return style. 
         

----

.. function:: getcwd

    Syntax:
        ``h.getcwd()``

    Description:
        Returns absolute path of current working directory in unix format. 
        The last character of the path name is '/'. 
        Must be less than 
        1000 characters long. 

----

.. function:: neuronhome

    Name:
        neuronhome -- installation path 

    Syntax:
        ``h.neuronhome()``

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

    Syntax:
        ``h("strdef name")``

        ``h.machine_name(h.name)``

    Description:
        returns the hostname of the machine. 


----

.. function:: unix_mac_pc

    Syntax:
        ``h.unix_mac_pc()``

    Description:
        Return 1 if unix, 2 if (an older) mac, 3 if mswin, or 4 if mac osx darwin 
        is the operating system. This 
        is useful when deciding if a machine specific function can be called or 
        a dll can be loaded.

    Example:
        .. code-block::
            python

            from neuron import h
            type = h.unix_mac_pc()

            if type == 1:
                print("This os is unix based")
            elif type == 2:
                print("This os is mac based")
            elif type == 3:
                print("This os is mswin based")
            elif type == 4:
                print("This os is mac osx darwin based")
         


         

----

.. function:: nrnversion

    Syntax:
        ``h.nrnversion()``

        ``h.nrnversion(i)``

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
          >>> from neuron import h
          >>> h.nrnversion(9)
          '2'

    Example:
        .. code-block::
            python

            from neuron import h, gui
            h.nrnversion() 
            'NEURON -- VERSION 8.2.2 HEAD (93d41fafd) 2022-12-15'

            for i in range(10): 
                print(f'{i} : {h.nrnversion(i)}')
            
            0 : 8.2.2
            1 : NEURON -- VERSION 8.2.2 HEAD (93d41fafd) 2022-12-15
            2 : VERSION 8.2.2 HEAD (93d41fafd)
            3 : 93d41fafd
            4 : 2022-12-15
            5 : 8.2.2
            6 : cmake option default differences: 'NRN_ENABLE_RX3D=OFF' 'NRN_ENABLE_CORENEURON=ON' 'NRN_ENABLE_MODULE_INSTALL=OFF' 'NRN_ENABLE_PYTHON_DYNAMIC=ON' 'NRN_MPI_DYNAMIC=/usr/local/opt/openmpi/include;/usr/local/opt/mpich/include' 'CMAKE_BUILD_TYPE=Release' 'CMAKE_INSTALL_PREFIX=/Users/runner/work/1/s/build/cmake_install' 'CMAKE_C_COMPILER=/Applications/Xcode_13.2.1.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc' 'CMAKE_CXX_COMPILER=/Applications/Xcode_13.2.1.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++' 'PYTHON_EXECUTABLE=/Users/runner/work/1/s/nrn_build_venv38_-4745831/bin/python'
            7 : NEURON
            8 : x86_64-Darwin
            9 : 2        


----

Execute a Command
~~~~~~~~~~~~~~~~~


.. function:: WinExec

    Syntax:
        ``h.WinExec("mswin command")``

    Description:
        MSWin version only. 
         
----

.. function:: system

    Name:
        system --- issue a shell command 

    Syntax:
        ``exitcode = h.system(cmdstr)``

        ``exitcode = h.system(cmdstr, stdout_str)``

    Description:
        Executes *cmdstr* as though it had been typed as 
        command to a unix shell from the terminal.  HOC waits until the command is 
        completed. If the second strdef arg is present, it receives the stdout stream 
        from the command. Only available memory limits the line length and 
        number of lines. 

    Example:

        \ ``h.system("ls")`` 
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

    Syntax:
        ``h.startsw()``


        Initializes a stopwatch with a resolution of 1 second or 0.01 second if 
        gettimeofday system call is available. See :func:`stopsw` . 


----

.. function:: stopsw

    Syntax:
        ``h.stopsw()``

        Returns the time in seconds since the stopwatch was last initialized with a :func:`startsw` . 

    Description:
        Really the idiom 

        .. code-block::
            python

            x = h.startsw() 
            h.startsw() - x 

        should be used since it allows nested timing intervals. 


    Example:
        .. code-block::
            python

            from neuron import h
            from math import sin
            h.startsw()
            for i in range(100000):
                x = sin(0.2)
            print(h.stopsw())
    
    .. note::

        A pure Python alternative would be to use the time module's perf_counter function.




.. seealso::

    :class:`Timer`


----

Miscellaneous
~~~~~~~~~~~~~

.. function:: nrn_load_dll

    Syntax:
        ``h.nrn_load_dll(dll_file_name)``

    Description:
        Loads a dll containing membrane mechanisms. This works for mswin, mac, 
        and linux. 


.. function:: show_winio

    Syntax:
        ``h.show_winio(0or1)``

    Description:
        MSWin and Mac version only. Hides or shows the console window. 
         

