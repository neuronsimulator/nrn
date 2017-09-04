#!/bin/bash

# eval "`sh nrnpyenv.sh`"
# if PYTHONHOME does not exist,
# will set bash environment variables so that nrniv -python has same
# environment as python

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

if test "$PYTHONHOME" != "" ; then
  echo '# PYTHONHOME exists. Do nothing' 1>&2
  exit 0
fi

PYTHON=python
if test "$1" != "" ; then
  PYTHON="$1"
fi

# what is the python library for Darwin
z=''
if type -P uname > /dev/null ; then
  z=`uname`
fi
if test "$z" = "Darwin" ; then
  DYLD_PRINT_LIBRARIES=1
  export DYLD_PRINT_LIBRARIES
  z=`$PYTHON -c 'quit()' 2>&1 | sed -n 's/^dyld: loaded: //p' | sed -n /libpython/p`
  if test "$z" = "" ; then
    z=`$PYTHON -c 'quit()' 2>&1 | sed -n 's/^dyld: loaded: //p' | sed -n 2p`
  fi
  unset DYLD_PRINT_LIBRARIES  
  PYLIB_DARWIN=$z
  export PYLIB_DARWIN
fi

$PYTHON << 'here'
###########################################

import sys, os, site

usep = "/"
upathsep = ":"

def upath(path):
  #return linux path
  if path == None:
    return ""
  import posixpath
  plist = path.split(os.pathsep)
  for i, p in enumerate(plist):
    p = os.path.splitdrive(p)
    if p[0]:
      p = usep + p[0][:p[0].rfind(":")] + usep + p[1].replace(os.sep, usep)
    else:
      p = p[1].replace(os.sep, usep)
    p = posixpath.normpath(p)
    plist[i] = p
  p = upathsep.join(plist)
  return p

#a copy of nrnpylib_linux() but with some os x specific modifications
def nrnpylib_darwin_helper():
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
        print ("from lsof: %s" % line)
        nrn_pylib = line.strip()
        return nrn_pylib
      if re.search(r'[Ll][Ii][Bb].*[Pp]ython', line):
        cnt += 1  
        if cnt == 1: # skip 1st since it is the python executable
          continue
        print ("from lsof: %s" % line)
        nrn_pylib = line.strip()
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
          return i
  return nrn_pylib

def nrnpylib_darwin():
  import os
  nrn_pylib = os.getenv("PYLIB_DARWIN")
  if nrn_pylib is not "":
    return nrn_pylib
  return nrnpylib_darwin_helper()
          
def nrnpylib_mswin():
  import os, sys, re
  e = '/'.join(sys.executable.split(os.path.sep))
  cmd = 'cygcheck "%s"' % e
  f = os.popen(cmd)
  nrn_pylib = None
  for line in f:
    if re.search('ython..\.dll', line):
      nrn_pylib = '/'.join(line.split(os.path.sep)).strip()
  return nrn_pylib

def nrnpylib_linux():
  import os, sys, re, subprocess
  #in case it was dynamically loaded by python
  pid = os.getpid()
  cmd = "lsof -p %d"%pid
  f = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
  nrn_pylib = None
  for bline in f:
    fields = bline.decode().split()
    if len(fields) > 8:
      line = fields[8]
      if re.search(r'libpython.*\.so', line):
        print ("from lsof: %s" % line)
        nrn_pylib = line.strip()
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
          return i
  return nrn_pylib

nrn_pylib = None
if 'darwin' in sys.platform:
  nrn_pylib = nrnpylib_darwin()
elif 'win' in sys.platform:
  nrn_pylib = nrnpylib_mswin()
elif 'linux' in sys.platform:
  nrn_pylib = nrnpylib_linux()

#there is a question about whether to use sys.prefix for PYTHONHOME
#or whether to derive from site.__file__.
#to help answer, ask how many sys.path items begin with sys.prefix and
#how many begin with site.__file__ - 3
p = [upath(i) for i in sys.path]
print ("# items in sys.path = " + str(len(p)))
sp = upath(sys.prefix)
print ("# beginning with sys.prefix = " + str(len([i for i in p if sp in i])))
s = usep.join(upath(site.__file__).split(usep)[:-3])
if s == sp:
  print ("# site-3 same as sys.prefix")
else:
  print ("# beginning with site-3 = " + str(len([i for i in p if s in i])))
foo = [i for i in p if sp not in i]
foo = [i for i in foo if s not in i]
print ("# in neither location " + str(foo))
print ("# sys.prefix = " + sp)
print ("# site-3 = " + s)
	
if "darwin" in sys.platform or "linux" in sys.platform or "win" in sys.platform:
  # What, if anything, did python prepend to PATH
  path=""
  oldpath = upath(os.getenv("originalPATH"))
  newpath = upath(os.getenv("PATH"))
  i = newpath.find(oldpath)
  if i > 1:
    path = newpath[:i]

  pythonhome = upath(sys.prefix)
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
  print ("\n# if launch nrniv, then likely need:")
  if pythonhome:
    print ("export PYTHONHOME=" + dq + pythonhome + dq)
  if ldpath and nrn_pylib == None:
    print ("export LD_LIBRARY_PATH=" + dq + ldpath + upathsep + "$LD_LIBRARY_PATH" + dq)
  if path:
    print ("export PATH=" + dq + path + "$PATH" + dq)
  if nrn_pylib != None:
    print ('export NRN_PYLIB="%s"' % nrn_pylib)

quit()

###################################
here
