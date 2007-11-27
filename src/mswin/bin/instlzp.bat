rem ****************** instlzp.bat **********************
rem
rem
rem ****** create single, nrngui, and mknrndll.bat ******
rem
echo %1\bin\neuron %1\lib\hoc\single.hoc>%1\bin\single.bat
echo %1\bin\neuron %1\lib\hoc\nrngui.hoc>%1\bin\nrngui.bat
rem
echo set include=> %1\bin\t2.bt
echo set path=%1\bin>> %1\bin\t2.bt
echo set NEURONHOME=%1>> %1\bin\t2.bt
%1\bin\d2uenv set NU=%1>> %1\bin\t2.bt
copy %1\bin\t2.bt + %1\bin\t1.bt %1\bin\mknrndll.bat
rem
rem
rem *************** create mkdllbox.bat *****************
rem
rem make mkdllbox.bat
echo @echo off>%1\bin\mkdllbox.bat
rem echo PATH %1\bin>>%1\bin\mkdllbox.bat
echo set PATH=%1\bin;%%PATH%%>>%1\bin\mkdllbox.bat
echo set NEURONHOME=%1>>%1\bin\mkdllbox.bat
echo prompt [mknrndll] $P$g>>%1\bin\mkdllbox.bat
echo echo ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿>>%1\bin\mkdllbox.bat
echo echo ³ Welcome to the mknrndll DOS box!                        ³>>%1\bin\mkdllbox.bat
echo echo ³                                                         ³>>%1\bin\mkdllbox.bat
echo echo ³ Navigate to the directory that contains your mod files, ³>>%1\bin\mkdllbox.bat
echo echo ³ then type                                               ³>>%1\bin\mkdllbox.bat
echo echo ³     modlunit filename.mod                               ³>>%1\bin\mkdllbox.bat
echo echo ³ to check the units in filename.mod                      ³>>%1\bin\mkdllbox.bat
echo echo ³ or type                                                 ³>>%1\bin\mkdllbox.bat
echo echo ³     mknrndll                                            ³>>%1\bin\mkdllbox.bat
echo echo ³ to create a new nrnmech.dll                             ³>>%1\bin\mkdllbox.bat
echo echo ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ>>%1\bin\mkdllbox.bat
rem cmd.exe is more powerful than command.com, 
rem   so the following line is better for WinNT users
rem echo %SystemRoot%\system32\cmd.exe>>%1\bin\mkdllbox.bat
rem Win 95/98 users are stuck with command.com and must use the next line
echo command.com>>%1\bin\mkdllbox.bat
rem
rem
rem don't bother with this for now--
rem *************** create mkdllbo2.bat *****************
rem
rem make mkdllbo2.bat
rem echo @echo off>%1\bin\mkdllbo2.bat
rem echo rem if this batch file is called from NEURON,>>%1\bin\mkdllbo2.bat
rem echo rem $(NEURONHOME) has already been prepended to PATH>>%1\bin\mkdllbo2.bat
rem echo rem so the following statement is not needed here>>%1\bin\mkdllbo2.bat
rem echo rem set PATH=%1\bin;%%PATH%%>>%1\bin\mkdllbo2.bat
rem echo set NEURONHOME=%1>>%1\bin\mkdllbo2.bat
rem echo prompt [mknrndll] $P$g>>%1\bin\mkdllbo2.bat
rem echo echo ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿>>%1\bin\mkdllbo2.bat
rem echo echo ³  Welcome to the mknrndll DOS box!                      ³>>%1\bin\mkdllbo2.bat
rem echo echo ³                                                        ³>>%1\bin\mkdllbo2.bat
rem echo echo ³  COMMAND:                ACTION:                       ³>>%1\bin\mkdllbo2.bat
rem echo echo ³  modlunit filename.mod   checks units in filename.mod  ³>>%1\bin\mkdllbo2.bat
rem echo echo ³  mknrndll                creates a new nrnmech.dll     ³>>%1\bin\mkdllbo2.bat
rem echo echo ³  exit                    closes this DOS box           ³>>%1\bin\mkdllbo2.bat
rem echo echo ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ>>%1\bin\mkdllbo2.bat
rem cmd.exe is more powerful than command.com, 
rem   so the following line is better for WinNT users
rem echo %SystemRoot%\system32\cmd.exe>>%1\bin\mkdllbo2.bat
rem Win 95/98 users are stuck with command.com and must use the next line
rem echo command.com>>%1\bin\mkdllbo2.bat
rem
rem
rem ******************** clean up ***********************
rem
del %1\examples\dummy.txt
del %1\examples\hmodl\dummy.txt
del %1\examples\nmodl\dummy.txt
del %1\examples\nrnoc\dummy.txt
del %1\src\dummy.txt
rem
rem
rem *********** a nice idea that didn't work *************
rem
rem using this batch file to try to hide these files 
rem can cause sharing violations
rem
rem attrib +h %1\DeIsL1.isu
rem attrib +h %1\_DEISREG.ISR
rem attrib +h %1\_ISREG32.DLL
