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
$PYTHON -c '
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

#there is a question about whether to use sys.prefix for PYTHONHOME
#or whether to derive from site.__file__.
#to help answer, ask how many sys.path items begin with sys.prefix and
#how many begin with site.__file__ - 3
p = [upath(i) for i in sys.path]
print "# items in sys.path = ",len(p)
sp = upath(sys.prefix)
print "# beginning with sys.prefix = ", [i for i in p if sp in i].__len__()
s = usep.join(upath(site.__file__).split(usep)[:-3])
if s == sp:
  print "# site-3 same as sys.prefix"
else:
  print "# beginning with site-3 = ", [i for i in p if s in i].__len__()
foo = [i for i in p if sp not in i]
foo = [i for i in foo if s not in i]
print "# in neither location ", foo
print "# sys.prefix = " + sp
print "# site-3 = " + s
	
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
  if pythonhome:
    print "export PYTHONHOME=" + dq + pythonhome + dq
  if pythonpath:
    print "export PYTHONPATH=" + dq + pythonpath + dq
  if ldpath:
    print "export LD_LIBRARY_PATH=" + dq + ldpath + upathsep + "$LD_LIBRARY_PATH" + dq
  if path:
    print "export PATH=" + dq + path + "$PATH" + dq

quit()

###################################
'
