"README" file
     for
installation of NEURON under MSWindows


This will install the MSWindows version of the NEURON 
simulation environment on your hard disk.  The core 
source code for this software is identical to the 
UNIX/Linux version, which is available over the WWW.


Please read the following information before proceeding 
further.


========================================================


REQUIREMENTS

CPU        Pentium or above
OS         MSWin 95 or above
RAM        At least 8 MB
Hard disk  At least 15 MB


========================================================


UNINSTALLING PRIOR VERSIONS

Before installing a new version of NEURON, you should 
probably uninstall older version(s).  To uninstall v.4.2b and 
later, just use Add/Remove Programs in the MSWin Control Panel.

To uninstall earlier versions, follow these steps:

1. Remove the directory created by the install program
where the NEURON software resides.

2. Remove the NEURON program group.

3. Edit the win.ini file and remove the [INTERVIEWS] 
and [NEURON] sections.

4. Edit the autoexec.bat file and remove the 
NEURONHOME and DJGENV environment variables.  
Also eliminate the old NEURON directory from the 
PATH.

5. Remove (or return to its original state) the shell
command in config.sys (prior NEURON installations 
may have increased the environment size to 5000).


========================================================


INSTALLATION

1.  Open the Control Panel and click on the 
Add/Remove Programs icon.

2.  Follow the setup instructions that appear on the
screen.  This will install NEURON, the on-line help files,
and the NMODL compiler.

3.  If you wish to install NEURON in a directory 
other than C:\NRN, do NOT specify a path that 
contains a space character, e.g. C:\PROGRAM FILES
File names and paths that contain the space character 
" " can result in NEURON failing to run or to load 
your model files.  If you need to improve the 
readability of paths or file names, try using the 
underscore character "_", e.g. C:\COMP_NEUROSCI

4.  When the installer has finished, 
please be sure to read $(NEURONHOME)/notes.txt.


========================================================


WWW BROWSERS

After you install this version of NEURON, your PC should 
handle NEURON model files (.hoc files) and model archives 
(.nrnzip files) properly without any need for you to make any 
special adjustments to your browser or the Windows Registry.
