Mac Binary Package (Apple M1 and Mac x86_64)
------------------

Binary packages are built with the ```nrnmacpkgcmake.sh``` script which,
near the end of its operation,
involves code signing, package signing, and notarization.
Preparing your Mac development environment for correct functioning of
the script requires installing a few extra [Dependencies](#Dependencies) beyond the
[normal user source build](./install_instructions.html#Mac-OS-Depend), obtaining an Apple Developer Program membership,
and requesting two signing certificates from Apple. Those actions are
described in separate sections below.
On an Apple M1,
the script, by default, creates, e.g.,
```nrn-8.0a-427-g1a80b2cc-osx-arm64-py-38-39.pkg```
where the information between nrn and osx comes from ```git describe```
and the numbers after the py indicate the python versions that are
compatible with this package. Those python versions must be installed on
the developer machine. On a Mac x86_64 architecture the script, by default,
creates, e.g., ```nrn-8.0a-427-g1a80b2cc-osx-x86_64-py-27-36-37-38-39.pkg```

A space separated list of python executable arguments can be used in
place of the internal default lists. ```$NRN_SRC``` is the location of the
repository, default ```$HOME/neuron/nrn```. The script makes sure ```$NRN_SRC/build```
exists and uses that folder to configure and build the software. The
software is installed in ```/Applications/NEURON```.

At time of writing, the cmake
command in the script that is used to configure the build is
```
cmake .. -DCMAKE_INSTALL_PREFIX=$NRN_INSTALL \
  -DNRN_ENABLE_MPI_DYNAMIC=ON \ 
  -DPYTHON_EXECUTABLE=`which python3` -DNRN_ENABLE_PYTHON_DYNAMIC=ON \
  -DNRN_PYTHON_DYNAMIC="$pythons" \
  -DIV_ENABLE_X11_DYNAMIC=ON \
  -DNRN_ENABLE_CORENEURON=OFF \
  -DNRN_RX3D_OPT_LEVEL=2 \
  -DCMAKE_OSX_ARCHITECTURES="$CPU" \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

A universal build is possible with ```-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"```
but the installations for openmpi and python on your machine must also be universal.
I.e. configure with ```-DNRN_ENABLE_MPI_DYNAMIC=OFF -DNRN_PYTHON_DYNAMIC=/usr/bin/python3```.
As we have been unable to find or build a universal openmpi, and
only the Big Sur default Python 3.8 installaion is universal, we are
currently building separate installers for arm64 and x86_64.

```make -j install``` is used to build and install in ```/Applications/NEURON```

```make macpkg``` ( see ```src/mac/CMakeLists.txt``` ) is used to:

- activate the apps ```src/mac/activate-apps-cmake.sh```

- codesign all the binaries ```src/mac/nrn_codesign.sh```:

  - Codesigning all binaries in the package is a prerequisite for
    notarization and not necessary if the package is not notarized. In
    that case it is a bit more trouble for users to install as they must
    ignore the scary message that pops up before the installer
    and know to right click on that message, then
    click `Open`. Then accept the option to `Open` the installer.

  - To prevent Notarization failure the binaries need hardened runtime enabled
    and then some of the binaries need some special entitlements enabled.
    These entitlements are specified in ```src/mac/nrn_entitlements.plist```
    and allow use of the Apple Events, Just in time compilation, and
    disable library validation.  The user will be asked to accept the
    first time these entitlements are invoked.

- create NEURON.pkg . ```/usr/bin/packagesbuild``` uses ```src/mac/macdist/pkgproj.in```
  
- productsign NEURON.pkg ```src/mac/nrn_productsign.sh```

- request Apple to notarize NEURON.pkg ```src/macnrn_notarize.sh```

The script ends by printing:
```
  Until we figure out how to automatically staple the notarization
  the following two commands must be executed manually.
  xcrun stapler staple $PACKAGE_FILE_NAME
  cp $PACKAGE_FILE_NAME $HOME/$PACKAGE_FULL_NAME
```
where the variables are filled out and can be copy/pasted to your
terminal after Apple sends an email declaring that notarization was successful.

The email from Apple usually takes just a few minutes but can be hours.
I've been uploading the ```$PACKAGE_FULL_NAME``` as an artifact for a
Release version from https://github.com/neuronsimulatior/nrn by choosing
Releases, choosing the proper Release tag (e.g. Release 8.0a), Edit release,
and clicking in the "Attach binaries ..." near the bottom of the page.

<a name="Dependencies"></a>
#### Dependencies

This uses Apple M1 as the example but mostly the same for an x86_64 mac.
`lipo` tells what architecture a binary must be run on. For example,
```
lipo -archs `which python3`
x86_64 arm64
```
which is generally the case for all Big Sur mac software. On the other
hand, ```brew install ...``` executables and libraries are 
generally only for the architecture indicated by
```
uname -m
```

- ```xcode-select --install```:

  - Sadly, notarization requires ```altool``` which is not distributed
    with the command line tools. So one needs to install the full xcode
    (at least version 12) from the App Store. That is very large and may
    take an hour or so to download.

- Install latest [XQuartz](http://xquartz.org) release. At least XQuartz-2.8.0_rc1

- Install [PackagesBuild](http://s.sudre.free.fr/Software/Packages/about.html)

- ```brew install cmake open-mpi python3.9```

  - Python 3.8 is already installed as /usr/bin/python3

  - The [normal source build](./install_instructions.html#Mac-OS-Depend)
    explains how to install brew and add it to the PATH.
    ```
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/H$
    echo 'eval $(/opt/homebrew/bin/brew shellenv)' >> $HOME/.zprofile
    eval $(/opt/homebrew/bin/brew shellenv)
    ```

  - Both pythons need numpy installed
    ```
    pip3.9 install --user --upgrade pip
    pip3.9 install --user numpy
    python3 -m pip install --user --upgrade pip
    python3 -m pip install --user numpy
    ```

  - At least the first python3 in your PATH needs cython installed
    ```
    python3 -m pip install --user cython
    ```

#### Signing and Notarization

Notarization needs a $99/year [Apple Developer Program](https://help.apple.com/developer-account/) membership.
Certificates last 5 years.

The most clearly explained recipe for notarization that I found is
[Notarizing installers for macOS Catalina](https://www.davidebarranca.com/2019/04/notarizing-installers-for-macos-catalina/)
.

However it took me while to figure out that it takes [two certificates](https://help.apple.com/developer-account/#/dev04fd06d56)
from apple.  I.e.  A "developerID_application.cer" to sign all the 
binaries before the pkg is built and a "developerID_installer.cer" to
sign the pkg. Certificates can be obtained through Keychain by creating
a [certificate signing request](https://help.apple.com/developer-account/#/devbfa00fef7)
On receipt of the certificates, I added to my keychain by clicking on them. 

And one must manage an [App specific password](https://support.apple.com/en-us/HT204397)
to request notarization from Apple.
I put the password in ```$HOME/.ssh/notarization-password``` which is where
```src/mac/nrn_notarize.sh``` expects it.

Notarization requires that we "enable hardened runtime"
which is done during the binary signing, but that results in issues with
regard to apple events and mismatched, unsigned, and non-system
libraries (as well as preventing JIT compilation).  At this point
[How to add entitlements...](https://forum.xojo.com/t/how-to-add-entitlements-to-a-xojo-app-using-codesign/49735/9)
was very helpful with regard to picking
[entitlements](https://developer.apple.com/documentation/bundleresources/entitlements)
to develop a nrn_entitlements.plist to relax the strict hardened
runtime.
