#!/usr/bin/env bash

# eval "`sh nrnpyenv.sh`"
# will set bash environment variables so that nrniv -python has same
# environment as python

# May specify the python executable with explicit first argument.
# Without arg use python3 and if that does not exist then python

# Overcome environment issues when --with-nrnpython=dynamic .

# The problems might be an immediate exit due to 'No module named site',
# inability to find common modules and shared libraries that support them,
# and not loading the correct python library.

#Run python and generate the following on stdout
#export PYTHONHOME=...
#export PYTHONPATH=...
#export LD_LIBRARY_PATH=...
#export PATH=...
#export NRN_PYLIB=...

#with NRN_PYLIB as a full path to the Python library,
#it may not be necessary to change LD_LIBRARY_PATH

#Some python installations, such as enthought canopy, do not have site
#in a subfolder of prefix. In that case, the site folder defines home and
#if there is a site-packages subfolder of prefix, that is added to the
#pythonpath. Also the lib under home is added to the ld_library_path.

# This script is useful for linux, mingw, and mac versions.
# Append the output to your .bashrc file.

export originalPATH="$PATH"
export originalPYTHONPATH="$PYTHONPATH"
export originalPYTHONHOME="$PYTHONHOME"
export originalLDLIBRARYPATH="$LD_LIBRARY_PATH"

# get the last argument
for last; do true; done

# if the last argument begins with --NEURON_HOME=
if [[ $last == "--NEURON_HOME="* ]] ; then
last=${last#"--NEURON_HOME="}
export PATH=/cygdrive/${last//:/}/mingw/usr/bin:/cygdrive/${last//:/}/mingw/mingw64/bin:$PATH
# remove the last argument
set -- "${@:1:$(($#-1))}"
fi

if test "$PYTHONHOME" != "" ; then
  echo "# Ignoring existing PYTHONHOME=$PYTHONHOME."
  unset PYTHONHOME
fi

WHICH=which

function trypy {
  a=`ls "$1" |grep "$2"`
  if test "$a" != "" ; then
    b=`cygpath -U "$1/$a/$3"`
    c=`nrnbinstr "$4" "$b" 2> /dev/null`
    if test "$c" != "" ; then
      c=`cygpath -U "$c"`
      c=`dirname "$c"`
      c=`dirname "$c"`
      c="$c/python"
      if $WHICH "$c" >& /dev/null ; then
        PYTHON=`$WHICH "$c"`
        # if python.exe not in PATH then cygcheck may not find the library
        PYTHON=`cygpath -U "$PYTHON"`
        export PATH=`dirname "$PYTHON"`:"$PATH"
        PYTHON=`basename "$PYTHON"`
      fi
    fi
  fi
}

# On some windows systems python is an empty executable which, when
# launched in a Command Prompt, directs the user to the Microsoft Store.
# With bash, it returns a 128 exit status. So we loop until we
# find a working python (or no python). Each time a python is non-working
# we remove that path from the PATH. If not Windows, break out after first
# attempt at finding a Python.
while true ; do
  PYTHON=""
  # Priority is the argument -pyexe, NRN_PYTHONEXE, python3 and then python

  # check -pyexe option
  if test "$1" != "" ; then
    if $WHICH "$1" >& /dev/null ; then
      PYTHON="$1"
    fi
  # If NRN_PYTHONEXE is set (e.g. from wheel wrapper) then use it
  elif test "$NRN_PYTHONEXE" != ""; then
    PYTHON=$NRN_PYTHONEXE
  elif $WHICH python3 >& /dev/null ; then
    PYTHON=python3
  elif $WHICH python >& /dev/null ; then
    PYTHON=python
  fi

  # do not do the following craziness if not Windows.
  if test "$OS" != "Windows_NT" ; then
    break
  fi

  if test "$PYTHON" == "" ; then
    break
  else
    if $PYTHON -c 'quit()' >& /dev/null ; then #working
      break
    else # remove from PATH
      oldpath="$PATH"
      a="`$WHICH $PYTHON`"
      b="`dirname \"$a\"`"
      PATH="`echo \"$PATH\" | sed \"s,:$b:,:,\"`" #remove b from path if internal
      PATH="`echo \"$PATH\" | sed \"s,^$b:,,\"`" #remove b from path if begin
      PATH="`echo \"$PATH\" | sed \"s,:$b\$,\",`" #remove b from path if end
      export PATH
      if test "$oldpath" = "$PATH" ; then
        echo "\"$b\", that contained a failing Python, did not get removed from PATH=\"$PATH\"" 1>&2
        exit 1
      fi
    fi
  fi
done

if test "$PYTHON" = "" ; then
  # Often people install Anaconda on Windows without adding it to PATH
  if test "$OS" = "Windows_NT" -a "$APPDATA" != "" ; then
    smenu="$APPDATA/Microsoft/Windows/Start Menu/Programs"
    if test "$PYTHON" = "" ; then
      trypy "$smenu" "Anaconda3 (64-bit)" "Anaconda Prompt (anaconda3).lnk" activate.bat
      # Anaconda3 2020 may need more PATH for numpy to work.
      if test "$PYTHON" != "" ; then
        if ! $PYTHON -c 'import numpy' >& /dev/null ; then
          # first item added in trypy
          a="`echo $PATH | sed 's/:.*//'`"
          export PATH="$PATH:$a/Library/mingw-w64/bin:$a/Library/usr/bin:$a/Library/bin:$a/Scripts:$a/bin:$a/condabin"
          # Actually get this PATH when scripts do a -- eval "`nrnpyenv.sh`"
          echo "export PATH=\"$PATH\""
        fi
      fi
    fi
    if test "$PYTHON" = "" ; then
      trypy "$smenu" Anaconda3 "Anaconda Prompt.lnk" activate.bat
    fi
    if test "$PYTHON" = "" ; then
      trypy "$smenu" Anaconda2 "Anaconda Prompt.lnk" activate.bat
    fi
    if test "$PYTHON" = "" ; then
      trypy "$smenu" Anaconda "Anaconda Prompt.lnk" activate.bat
    fi
    if test "$PYTHON" = "" ; then #brittle but try Enthought
      a=`cygpath -U "$APPDATA/../local/enthought/canopy/edm/envs/user"`
      if test -d "$a" ; then
        export PATH="$a":"$PATH"
        PYTHON=python
      fi
    fi
  fi
fi

if test "$PYTHON" = "" ; then
  echo "Cannot find executable python3 or python" 1>&2
  exit 1;
fi

echo "# PYTHON=`$WHICH $PYTHON`"

# what is the python library for Darwin
nrnpylib_provenance=""
nrn_pylib=""
kernel_name=''
if type -P uname > /dev/null ; then
  kernel_name=`uname`
fi
if test "$kernel_name" = "Darwin" ; then
  python_path=`$WHICH $PYTHON`
  pyexedir=`dirname $python_path`
  # Get the python lib dir in an official way, working with virtualenv
  PYLIB=$($python_path -c 'import sysconfig; print(sysconfig.get_config_var("LIBDIR"))')
  for path in $PYLIB/libpython*.dylib; do
    if test -f "$path"; then
      nrn_pylib="$path"
      break
    fi
  done
  if test -f "$nrn_pylib" ; then
    unset python_path
    unset pyexedir
    nrnpylib_provenance="sysconfig LIBDIR"
  fi
  if test "$nrn_pylib" = "" ; then
    nrn_pylib=$($python_path -c '
try:
  from neuron import h
  shlib=h.libpython_path()
  shlib = shlib if ".dylib" in shlib else ""
  print(shlib)
except:
  print("")
')
    if test "$nrn_pylib" != "" ; then
      nrnpylib_provenance="h.libpython_path()"
    fi
  fi
  if test "$nrn_pylib" = "" ; then
    DYLD_PRINT_LIBRARIES=1
    export DYLD_PRINT_LIBRARIES
    nrn_pylib=`$PYTHON -c 'quit()' 2>&1 | sed -n 's/^dyld: loaded: //p' | sed -n /libpython/p`
    if test "$nrn_pylib" = "" ; then
      nrn_pylib=`$PYTHON -c 'quit()' 2>&1 | sed -n 's/^dyld: loaded: //p' | sed -n 2p`
    fi
    unset DYLD_PRINT_LIBRARIES
    if test "$nrn_pylib" != "" ; then
      nrnpylib_provenance=DYLD_PRINT_LIBRARIES
    fi
  fi
  if test -f "$nrn_pylib" ; then
    PYLIB_DARWIN=$nrn_pylib
  else
    PYLIB_DARWIN=""
  fi
  export nrnpylib_provenance
  export PYLIB_DARWIN
fi

$PYTHON << 'here'
###########################################

import sys, os, site

usep = "/"
upathsep = ":"

nrnpylib_provenance = "not found"
nrnpyhome_provenance = "not found"

def upath(path):
  #return linux path
  if path is None:
    return ""
  import posixpath, sys
  plist = path.split(os.pathsep)
  for i, p in enumerate(plist):
    p = os.path.splitdrive(p)
    if p[0]:
      p = "/cygdrive/" + p[0][:p[0].rfind(":")] + usep + p[1].replace(os.sep, usep)
    else:
      p = p[1].replace(os.sep, usep)
    p = posixpath.normpath(p)
    plist[i] = p
  p = upathsep.join(plist)
  return p

def u2d(p):
  if "darwin" not in sys.platform and "win" in sys.platform:
    p = p.split(usep)
    if "cygdrive" == p[1]:
      p = p[2] + ':/' + usep.join(p[3:])
    else:
      p = usep.join(p)
  return p

#a copy of nrnpylib_linux() but with some os x specific modifications
def nrnpylib_darwin_helper():
  global nrnpylib_provenance
  import os, sys, re, subprocess
  #in case it was dynamically loaded by python
  pid = os.getpid()
  cmd = "lsof -p %d"%pid
  f = []
  try: # in case lsof does not exist
    f = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
  except:
    pass
  nrn_pylib = None
  cnt = 0
  for bline in f:
    fields = bline.decode().split()
    if len(fields) > 8:
      line = fields[8]
      if re.search(r'libpython.*\.[ds]', line):
        print ("# nrn_pylib from lsof: %s" % line)
        nrn_pylib = line.strip()
        nrnpylib_provenance = "lsof search for libpython..."
        return nrn_pylib
      if re.search(r'[Ll][Ii][Bb].*[Pp]ython', line):
        cnt += 1
        if cnt == 1: # skip 1st since it is the python executable
          continue
        if re.search(r'[Pp]ython', line.split('/')[-1]):
          print ("# nrn_pylib from lsof: %s" % line)
          candidate = line.strip()
          # verify the file defines a PyRun_SimpleString
          cmd = r'nm %s | grep PyRun_SimpleString' % candidate
          try:
            f = os.popen(cmd)
            i=0
            for line in f:
              i += 1
            if i == 0:
              continue
          except:
            continue
          nrn_pylib = candidate
          nrnpylib_provenance = 'lsof search for occurrence of [Ll][Ii][Bb].*[Pp]ython defineing PyRun_SimpleString'
          return nrn_pylib
  else: # figure it out from the os path
    p = os.path.sep.join(os.__file__.split(os.path.sep)[:-1])
    name = "libpython%d.%d" % (sys.version_info[0], sys.version_info[1])
    cmd = r'find %s -name %s\*.dylib' % (p, name)
    print ('# %s'%cmd)
    f = os.popen(cmd)
    libs = []
    for line in f:
      libs.append(line.strip())
    if len(libs) == 0: # try again searching the parent folder
      p = os.path.sep.join(os.__file__.split(os.path.sep)[:-2])
      cmd = r'find %s -name %s\*.dylib' % (p, name)
      print ('# %s'%cmd)
      f = os.popen(cmd)
      for line in f:
        libs.append(line.strip())
    print ('# %s'%str(libs))
    if len(libs) == 1:
      nrnpylib_provenance="search based on os.__file__, found unique"
      print ("# nrn_pylib from os.path %s"%str(libs[0]))
      return libs[0]
    if len(libs) > 1:
      # which one do we want? Check the name of an imported shared object
      try:
        import _ctypes
      except:
        import ctypes
      for i in sys.modules.values():
        try:
          s = i.__file__
          if s.endswith('.dylib'):
            match = re.search(r'-%d%d([^-]*)-' % (sys.version_info[0], sys.version_info[1]), s)
            if match:
              name = name + match.group(1) + '.dylib'
            break
          elif s.endswith('.so'):
            match = re.search(r'-%d%d([^-]*)-' % (sys.version_info[0], sys.version_info[1]), s)
            if match:
              name = name + match.group(1) + '.so'
            break
        except:
          pass
      for i in libs:
        if name in i:
          print ("# nrn_pylib from os.path %s" % i)
          nrnpylib_provenance='search based on os.__file__, found one with version in name'
          return i
      print ("# nrn_pylib from os.path %s" % str(nrn_pylib))
  return nrn_pylib

def nrnpylib_darwin():
  global nrnpylib_provenance
  import os
  nrn_pylib = os.getenv("PYLIB_DARWIN")
  if nrn_pylib != "":
    print ("# nrn_pylib from PYLIB_DARWIN %s"%nrn_pylib)
    nrnpylib_provenance = os.getenv("nrnpylib_provenance")
    return nrn_pylib
  return nrnpylib_darwin_helper()

def nrnpylib_mswin():
  global nrnpylib_provenance
  import os, sys, re
  e = '/'.join(sys.executable.split(os.path.sep))
  cmd = 'cygcheck "%s"' % e
  f = os.popen(cmd)
  nrn_pylib = None
  for line in f:
    if re.search('ython[a-zA-Z0-9_.]*\.dll', line):
      nrn_pylib = '/'.join(line.split(os.path.sep)).strip()
      nrnpylib_provenance="cygcheck"
  return nrn_pylib

def nrnpylib_linux():
  global nrnpylib_provenance
  import os, sys, re, subprocess

  # Try the official way first
  import sysconfig
  libdir=sysconfig.get_config_var("LIBDIR")
  try:
    from os.path import isfile, join
    ver = "%d.%d"%(sys.version_info[0], sys.version_info[1])
    for f in os.listdir(libdir):
      if 'libpython' in f and '.so' in f and ver in f:
        nrn_pylib = join(libdir, f)
        nrnpylib_provenance='sysconfig LIBDIR'
        return nrn_pylib
  except:
    pass

  #in case it was dynamically loaded by python
  try:
    from neuron import h
    s=h.libpython_path()
    s = s if ".so" in s else ""
    if (s != ""):
      nrnpylib_provenance="h.libpython_path()"
      return s
  except:
    print("")
  pid = os.getpid()
  cmd = "lsof -p %d"%pid
  f = []
  try: # in case lsof does not exist
    f = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
  except:
    pass
  nrn_pylib = None
  for bline in f:
    fields = bline.decode().split()
    if len(fields) > 8:
      line = fields[8]
      if re.search(r'libpython.*\.so', line):
        print ("# from lsof: %s" % line)
        nrn_pylib = line.strip()
        nrnpylib_provenance = 'lsof search for libpython.*\.so'
        return nrn_pylib
  else: # figure it out from the os path
    p = os.path.sep.join(os.__file__.split(os.path.sep)[:-1])
    name = "libpython%d.%d" % (sys.version_info[0], sys.version_info[1])
    cmd = r'find %s -name %s\*.so' % (p, name)
    print ('# %s'%cmd)
    f = os.popen(cmd)
    libs = []
    for line in f:
      libs.append(line.strip())
    if len(libs) == 0: # try again searching the parent folder
      p = os.path.sep.join(os.__file__.split(os.path.sep)[:-2])
      cmd = r'find %s -name %s\*.so' % (p, name)
      print ('# %s'%cmd)
      f = os.popen(cmd)
      for line in f:
        libs.append(line.strip())
    print ('# %s'%str(libs))
    if len(libs) == 1:
      nrnpylib_provenance="search based on os.__file__, found unique"
      return libs[0]
    if len(libs) > 1:
      # which one do we want? Check the name of an imported shared object
      try:
        import _ctypes
      except:
        import ctypes
      for i in sys.modules.values():
        try:
          s = i.__file__
          if s.endswith('.so'):
            match = re.search(r'-%d%d([^-]*)-' % (sys.version_info[0], sys.version_info[1]), s)
            if match:
              name = name + match.group(1) + '.so'
            break
        except:
          pass
      for i in libs:
        if name in i:
          nrnpylib_provenance='search based on os.__file__, found one with version in name'
          return i
  return nrn_pylib

nrn_pylib = None
if 'darwin' in sys.platform:
  nrn_pylib = nrnpylib_darwin()
elif 'win' in sys.platform:
  nrn_pylib = nrnpylib_mswin()
elif 'linux' in sys.platform:
  nrn_pylib = nrnpylib_linux()

#Use sys.base_prefix for PYTHONHOME if available, otherwise sys.prefix
try:
  sp = upath(sys.base_prefix)
  spname='sys.base_prefix'
  base=True
except:
  sp = upath(sys.prefix)
  spname='sys.prefix'
  base=False

#there is a question about whether to use sys.prefix for PYTHONHOME
#or whether to derive from site.__file__.
#to help answer, ask how many sys.path items begin with sys.prefix and
#how many begin with site.__file__ - 3
p = [upath(i) for i in sys.path]
print ("# items in sys.path = " + str(len(p)))
print ("# beginning with sys.prefix = " + str(len([i for i in p if sp in i])))
s = usep.join(upath(site.__file__).split(usep)[:-3])
if s == sp:
  print ("# site-3 same as " + spname)
else:
  print ("# beginning with site-3 = " + str(len([i for i in p if s in i])))
foo = [i for i in p if sp not in i]
foo = [i for i in foo if s not in i]
print ("# in neither location " + str(foo))
print ("# " + spname + " = " + sp)
print ("# site-3 = " + s)

if "darwin" in sys.platform or "linux" in sys.platform or "win" in sys.platform:
  # What, if anything, did python prepend to PATH
  path=""
  oldpath = upath(os.getenv("originalPATH"))
  newpath = upath(os.getenv("PATH"))
  i = newpath.find(oldpath)
  if i > 1:
    path = newpath[:i]

  pythonhome = upath(sp)
  print ("#pythonhome=" + pythonhome)
  pythonpath = upath(os.getenv("PYTHONPATH"))

  ldpath = ""
  oldldpath = upath(os.getenv("originalLD_LIBRARY_PATH"))
  newldpath = upath(os.getenv("LD_LIBRARY_PATH"))
  i = newldpath.find(oldldpath)
  if  i > 1:
    ldpath = newldpath[:i]

  sitedir = usep.join(upath(site.__file__).split(usep)[:-1])

  # if sitedir is not a subfolder of pythonhome, add to pythonpath
  if not pythonhome in sitedir:
    if not sitedir in pythonpath:
      pythonpath = (pythonpath + upathsep if pythonpath else "") + sitedir

  # add the parent of sitedir to LD_LIBRARY_PATH
  ldp = usep.join(sitedir.split(usep)[:-1])
  if ldp not in oldldpath:
    ldpath = (ldpath + upathsep if ldpath else "") + ldp

  try:
    #if a representative shared libary not under pythonhome, add to pythonpath
    import _ctypes
    f = usep.join(upath(_ctypes.__file__).split(usep)[:-1])
    if f.find(pythonhome) == -1:
      pythonpath = (pythonpath + upathsep if pythonpath else "") + f
  except:
    pass

  dq = "\""
  if pythonpath:
    print ("\n# if launch python, then need:")
    print ("export PYTHONPATH=" + dq + pythonpath + dq)

  if path:
    print ("\n#PYTHON prepended the following to PATH")
    print ("export PATH=" + dq + path + "$PATH" + dq)

  print("\n#NRN_PYLIB provenance: " + str(nrnpylib_provenance))
  print ("\n# if launch nrniv, then likely need:")
  if pythonhome:
    pythonhome=u2d(pythonhome)
    print ("export NRN_PYTHONHOME=" + dq + pythonhome + dq)
  if ldpath and nrn_pylib is None:
    print ("export LD_LIBRARY_PATH=" + dq + ldpath + upathsep + "$LD_LIBRARY_PATH" + dq)
  if nrn_pylib is not None:
    print ('export NRN_PYLIB="%s"' % nrn_pylib)

quit()

###################################
here
