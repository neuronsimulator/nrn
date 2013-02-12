.. _system:

system
------



.. function:: system

         

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



.. function:: stopwatch

         

    Name:
        startsw, stopsw --- stopwatch 
         

    Syntax:
        ``startsw()``

        ``stopsw()``



    Description:
        \ ``Startsw()`` initializes the stopwatch. \ ``Stopsw()`` returns the number of 
        seconds since the previous \ ``startsw()``. 
         

----



.. function:: neuronhome

         

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



.. function:: winio_show


    Syntax:
        ``winio_show(0or1)``


    Description:
        MSWin and Mac version only. Hides or shows the console window. 

         

----



.. function:: WinExec


    Syntax:
        ``WinExec("mswin command")``


    Description:
        MSWin version only. 

         

----



.. function:: unix_mac_pc


    Syntax:
        ``type = unix_mac_pc()``


    Description:
        Return 1 if unix, 2 if mac, 3 if mswin, or 4 if mac osx darwin 
        is the operating system. This 
        is useful when deciding if a machine specific function can be called or 
        a dll can be loaded. 

         

----



.. function:: nrn_load_dll


    Syntax:
        ``nrn_load_dll(dll_file_name)``


    Description:
        Loads a dll containing membrane mechanisms. This works for mswin, mac, 
        and linux. 

         

----



.. function:: nrnversion


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


         
         

