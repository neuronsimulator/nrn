Brief instructions:

First, install the interviews distribution that comes from the same
place as you got the neuron distribution (see the INSTALL file there for
details).

following:

% gzip -dc nrn-4.2.3.tar.gz | tar -xf - # You've already done this if you're
%					# reading this file.
% cd nrn-4.2.3
% ./configure --prefix=/where/you/want/neuron --with-iv=/where/you/put/interviews
% make
% make install
% cd ..
% rm -rf nrn-4.2.3	# You don't need this directory any more.
% /where/you/want/neuron/bin/neurondemo
# the above line tests the nrnivmodl script and runs the demo. alternatively
# you can run the bare executable (with only built in model descriptions
# with
% /where/you/want/neuron/bin/nrniv

% cd (directory where you have all your .mod files)
% /where/you/want/neuron/bin/nrnivmodl
% i686/special		# Will be in a different subdirectory depending 
%			# on your architecture.
# the above special may be only a script with the actual executable in
# i686/.libs/special
# this latter can be moved to another location and executed.

-------------------------------------------------------------------------------
MACHINE PROBLEMS

Some machines have problems which have work arounds specifically
incorporated into the configure script.
Those machines are listed here as examples
of what has gone occasionally wrong in the past with
generic installation attempts and how to fix them.
If installation on your machine does not work the
first time execute
	config.guess
and see if your machine appears in this list for some hints about how
to install. If your machine does not appear in this list but it still
doesn't install, look at  the list of occasional problems.

No machines in list.

-----------
OCCASIONAL INSTALLATION PROBLEMS
(machines indicated for each problem automatically
do the indicated work around within the configure file)

bison exists but it doesn't work
	alpha-cray-unicosmk2.0.4.90 (at least on the t3e at sdsc)

	force use of yacc with
	setenv YACC yacc ; configure ....


parse1.c or lex.c does not compile
	alpha-cray-unicosmk2.0.4.90 (at least on the t3e at sdsc)

	force the re-translation of .y and .l files with
	touch `find . -name \*.\[yl\] -print`

too many ld warnings
	mips-sgi-irix6.2

	suppress warnings with:
	setenv LDFLAGS -w ; configure ....

compiler internal error
	gcc-2.8.1

	compile c++ files without optimization
	setenv CXXFLAGS -g

applications fail to launch because shared libraries cannot be found
	setenv LD_LIBRARY_PATH /list/of:/library/paths:/to/find/them/in
-------------------------------------------------------------------------------
More detailed instructions:

When neuron is configured it checks to see if the GNU readline library is
already installed. If not then it builds an old version of the
readline library which is located in src/readline.

In addition, you will need to install the interviews library.  You
cannot use the vanilla interviews library that may come packaged with
X11, or may be available as a separate package with your operating
system.  You must use the custom-modified interviews library which is
available from the neuron web site neuron.yale.edu or wherever you
got the this neuron distribution from.  Read the INSTALL file in that
distribution for details.  Again, you can install the interviews library 
in the same directory that you're going to install neuron in.

Once you've got these libraries installed, do

	./configure --prefix=/where/you/want/neuron

If you don't specify a prefix, it defaults to /usr/local.  Unlike
previous versions of neuron, it is difficult to move neuron once you've
installed it, so pick your installation directory carefully.

There are a couple of options to configure that you may need:

	--with-iv=/where/you/put/interviews
			Use this if you didn't install your interviews
			distribution in a standard place (like /usr/local).

	--with-readline=/where/you/put/readline
			Use this if you didn't install readline in a
			standard place.

	--enable-static --enable-debug
			This may be useful if you're trying to debug the
			neuron sources themselves.  This builds static
			libraries in addition to the shared libraries.

	--with-nrnpython=/where/you/put/python
                        Access the NEURON python API

It's possible that you need to specify the location of your X11 files,
if they aren't in some standard place like /usr/X11R6/lib or
/usr/lib/X11; do "configure --help" and look at the --with-X options.

You may also need to specify the name of your C++ compiler.  (The
configure script can guess the name of the C++ compiler for many
systems.)  Do this with a command like this:

	CXX=/some/weird/place/bin/my_special_c++ ./configure

or on systems with the env command,

	env CXX=/some/weird/place/bin/my_special_c++ ./configure

You might also need to specify the LIBS variable in the same way if
there are special libraries you need to link with.

Once the configure procedure has successfully run, type "make".  This
will take a few minutes and will build everything in neuron.  (Unlike
previous versions of neuron, this builds both nrnoc and nrniv.)

If you run into compilation errors with the files "lex.c", then you will
need to rerun lex or flex on the .l files.  In this case, you must have
lex or flex installed on your system and in your path.  (Flex is freely
available from the Free Software Foundation, www.gnu.org.)  Delete all
the files named "lex.c" (there are several in different directories),
and make should automatically rebuild them for you using the version of
flex that you have installed on your system.

Similarly, if you run into compilation errors with the files "ytab.c" or
"d_ytab.c", then you will probably need to rerun bison or yacc on the .y
files.  To do this, simply delete the offending .c files and the make
procedure will automatically rebuild them for you using the bison that
you have on your system.  If your problem is with d_ytab.c, then you
must have bison (yacc isn't good enough, sorry).  Bison is freely
available from www.gnu.org.

Once compilation is complete, you will need to install neuron in order
to use it.  (This is different from previous versions.)  Type "make
install" to do so.  Once you've installed neuron, no files in the
directory tree that you untarred neuron into are needed any more; you
may delete the whole thing.  (This is also different from previous
versions.)

As with any other unix package, it is not a good idea to move neuron
around once you've installed it, because the directory names are coded
into the files themselves.  If you need to move it, put a soft link in
its original location that points to the new location (so the old
filenames will still work).  Better yet, simply recompile neuron
specifying the new installation directory.

In previous versions of NEURON, you needed to set the NEURONHOME
environment variable and the CPU variable, and often LD_LIBRARY_PATH as
well.  This should no longer be necessary.  Simply run the binaries that
are installed.  E.g., if you did "configure
--prefix=/usr/local/stow/neuron-4.2.2", then you would run

	/usr/local/stow/neuron-4.2.2/bin/nrniv

Or you can put that directory in your path.

To compile your own special purpose .mod files into neuron, use the
following procedure:

1) Cd to the directory that contains your .mod files.
2) Type "/where/you/put/neuron/bin/nrnivmodl" (or, if you put that
   directory in your path, just type "nrnivmodl").  This will create a
   subdirectory of the current directory which is your CPU name (e.g.,
   if you're running neuron on a pentium II or III, the subdirectory is
   called "i686".  Inside this directory is created the program
   "special", which is the neuron binary that you want to run.


Automake and autoconf scripts were originally done for this package by
Gary Holt (holt@LNC.usc.edu).  These replace the treacherous primitive
Imakefile jungle with the modern temperate forest of autoconf.  If you
wish to modify the Makefile.am files, you will need both autoconf and
automake.  I used autoconf-2.13 and automake-1.4; I doubt you can use
earlier versions.

Following are the generic installation instructions for any package with 
a configure script.

-------------------------------------------------------------------------------

Basic Installation
==================

   These are generic installation instructions.

   The `configure' shell script attempts to guess correct values for
various system-dependent variables used during compilation.  It uses
those values to create a `Makefile' in each directory of the package.
It may also create one or more `.h' files containing system-dependent
definitions.  Finally, it creates a shell script `config.status' that
you can run in the future to recreate the current configuration, a file
`config.cache' that saves the results of its tests to speed up
reconfiguring, and a file `config.log' containing compiler output
(useful mainly for debugging `configure').

   If you need to do unusual things to compile the package, please try
to figure out how `configure' could check whether to do them, and mail
diffs or instructions to the address given in the `README' so they can
be considered for the next release.  If at some point `config.cache'
contains results you don't want to keep, you may remove or edit it.

   The file `configure.ac' is used to create `configure' by a program
called `autoconf'.  You only need `configure.ac' if you want to change
it or regenerate `configure' using a newer version of `autoconf'.

The simplest way to compile this package is:

  1. `cd' to the directory containing the package's source code and type
     `./configure' to configure the package for your system.  If you're
     using `csh' on an old version of System V, you might need to type
     `sh ./configure' instead to prevent `csh' from trying to execute
     `configure' itself.

     Running `configure' takes awhile.  While running, it prints some
     messages telling which features it is checking for.

  2. Type `make' to compile the package.

  3. Optionally, type `make check' to run any self-tests that come with
     the package.

  4. Type `make install' to install the programs and any data files and
     documentation.

  5. You can remove the program binaries and object files from the
     source code directory by typing `make clean'.  To also remove the
     files that `configure' created (so you can compile the package for
     a different kind of computer), type `make distclean'.  There is
     also a `make maintainer-clean' target, but that is intended mainly
     for the package's developers.  If you use it, you may have to get
     all sorts of other programs in order to regenerate files that came
     with the distribution.

Compilers and Options
=====================

   Some systems require unusual options for compilation or linking that
the `configure' script does not know about.  You can give `configure'
initial values for variables by setting them in the environment.  Using
a Bourne-compatible shell, you can do that on the command line like
this:
     CC=c89 CFLAGS=-O2 LIBS=-lposix ./configure

Or on systems that have the `env' program, you can do it like this:
     env CPPFLAGS=-I/usr/local/include LDFLAGS=-s ./configure

Compiling For Multiple Architectures
====================================

   You can compile the package for more than one kind of computer at the
same time, by placing the object files for each architecture in their
own directory.  To do this, you must use a version of `make' that
supports the `VPATH' variable, such as GNU `make'.  `cd' to the
directory where you want the object files and executables to go and run
the `configure' script.  `configure' automatically checks for the
source code in the directory that `configure' is in and in `..'.

   If you have to use a `make' that does not supports the `VPATH'
variable, you have to compile the package for one architecture at a time
in the source code directory.  After you have installed the package for
one architecture, use `make distclean' before reconfiguring for another
architecture.

Installation Names
==================

   By default, `make install' will install the package's files in
`/usr/local/bin', `/usr/local/man', etc.  You can specify an
installation prefix other than `/usr/local' by giving `configure' the
option `--prefix=PATH'.

   You can specify separate installation prefixes for
architecture-specific files and architecture-independent files.  If you
give `configure' the option `--exec-prefix=PATH', the package will use
PATH as the prefix for installing programs and libraries.
Documentation and other data files will still use the regular prefix.

   In addition, if you use an unusual directory layout you can give
options like `--bindir=PATH' to specify different values for particular
kinds of files.  Run `configure --help' for a list of the directories
you can set and what kinds of files go in them.

   If the package supports it, you can cause programs to be installed
with an extra prefix or suffix on their names by giving `configure' the
option `--program-prefix=PREFIX' or `--program-suffix=SUFFIX'.

Optional Features
=================

   Some packages pay attention to `--enable-FEATURE' options to
`configure', where FEATURE indicates an optional part of the package.
They may also pay attention to `--with-PACKAGE' options, where PACKAGE
is something like `gnu-as' or `x' (for the X Window System).  The
`README' should mention any `--enable-' and `--with-' options that the
package recognizes.

   For packages that use the X Window System, `configure' can usually
find the X include and library files automatically, but if it doesn't,
you can use the `configure' options `--x-includes=DIR' and
`--x-libraries=DIR' to specify their locations.

Specifying the System Type
==========================

   There may be some features `configure' can not figure out
automatically, but needs to determine by the type of host the package
will run on.  Usually `configure' can figure that out, but if it prints
a message saying it can not guess the host type, give it the
`--host=TYPE' option.  TYPE can either be a short name for the system
type, such as `sun4', or a canonical name with three fields:
     CPU-COMPANY-SYSTEM

See the file `config.sub' for the possible values of each field.  If
`config.sub' isn't included in this package, then this package doesn't
need to know the host type.

   If you are building compiler tools for cross-compiling, you can also
use the `--target=TYPE' option to select the type of system they will
produce code for and the `--build=TYPE' option to select the type of
system on which you are compiling the package.

Sharing Defaults
================

   If you want to set default values for `configure' scripts to share,
you can create a site shell script called `config.site' that gives
default values for variables like `CC', `cache_file', and `prefix'.
`configure' looks for `PREFIX/share/config.site' if it exists, then
`PREFIX/etc/config.site' if it exists.  Or, you can set the
`CONFIG_SITE' environment variable to the location of the site script.
A warning: not all `configure' scripts look for a site script.

Operation Controls
==================

   `configure' recognizes the following options to control how it
operates.

`--cache-file=FILE'
     Use and save the results of the tests in FILE instead of
     `./config.cache'.  Set FILE to `/dev/null' to disable caching, for
     debugging `configure'.

`--help'
     Print a summary of the options to `configure', and exit.

`--quiet'
`--silent'
`-q'
     Do not print messages saying which checks are being made.  To
     suppress all normal output, redirect it to `/dev/null' (any error
     messages will still be shown).

`--srcdir=DIR'
     Look for the package's source code in directory DIR.  Usually
     `configure' can determine that directory automatically.

`--version'
     Print the version of Autoconf used to generate the `configure'
     script, and exit.

`configure' also accepts some other, not widely useful, options.
