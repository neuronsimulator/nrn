Mac Binary Package (Apple M1 and Mac x86_64)
------------------

Macos binary packages are built with the
[bldnrnmacpkgcmake.sh](https://github.com/neuronsimulator/nrn/blob/master/bldnrnmacpkgcmake.sh)
script which, near the end of its operation,
involves code signing, package signing, and notarization.
The build will be universal2 and work on arm64 and x86_64 architectures
(if the pythons used are themselves, universal2).
Preparing your Mac development environment for correct functioning of
the script requires installing a few extra [Dependencies](#Dependencies) beyond the
[normal user source build](./install_instructions.md#Mac-OS-Depend),
obtaining an Apple Developer Program membership,
and requesting two signing certificates from Apple. Those actions are
described in separate sections below.
On an Apple M1 or x86_64,
the script, by default, creates, e.g.,
```nrn-8.0a-726-gb9a811a32-macosx-11-universal2-py-38-39-310.pkg```
where the information between nrn and macosx comes from ```git describe```,
the number after macosx refers to the ```MACOSX_DEPLOYMENT_TARGET=11```
the next item(s) before py indicate the architectures on which
the program can run (i.e. ```arm64```, ```x86_64```, or ```universal2```
for both)
and the numbers after the py indicate the python versions that are
compatible with this package. Those python versions must be installed on
the developer machine.
The script will build a universal pkg only
if all the Python's are themselves universal.
Presently, one must manually make sure that all the python builds used
the same MACOSX_DEPLOYMENT_TARGET. You can check both of these with
```python3 -c 'import sysconfig; print(sysconfig.get_platform())'```

A space separated list of python executable arguments can be used in
place of the internal default lists. ```$NRN_SRC``` is the location of the
repository, default is the current working directory (hopefully you launch at the location of this script, e.g. ```$HOME/neuron/nrn```). The script makes sure ```$NRN_SRC/build```
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
  $archs_cmake \
  -DCMAKE_PREFIX_PATH=/usr/X11 \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```
[^1]
[^1]: NRN_RX3D_OPT_LEVEL=2 can build VERY slowly (cython translated cpp file can take a half hour or more). So for testing, we generally copy the script to temp.sh and modify to NRN_RX3D_OPT_LEVEL=0

The default variables above will be
```
pythons="python3.8;python3.9;python3.10;python3.11"
archs_cmake='-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
```

A universal build is possible with ```-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"```
but the installations of python on your machine must also be universal.
The MPI library does not have to be universal if configuring with
```-DNRN_ENABLE_MPI_DYNAMIC=ON``` as the library is not linked against
during the build.

```make -j install``` [^2] is used to build and install in ```/Applications/NEURON```

[^2]: Instead of make, the script uses Ninja. ```ninja install``` and the cmake line begins with ```cmake .. -G Ninja ...```

Runs some basic tests to verify a successful build

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

  If notarization succeeds it will finish with a few lines of the form
  ```
  Current status: Accepted........Processing complete
    id: bda82fa9-e0ab-4f86-aad8-3fee1d8c2369
    status: Accepted
  ```
  and staple the package.

  If notarizaton fails, it is occasionally due to Apple
  changing the contracts and demanding that 
  "You must first sign the relevant contracts online. (1048)". In that
  case, go to [appstoreconnect.apple.com](https://appstoreconnect.apple.com)
  to accept the legal docs an try again. For other notarization failures, one must consult the LogFileURL which can be obtained with [^3]
  [^3]: altool has been replaced by notarytool for purposes of notarization. See
  https://developer.apple.com/documentation/technotes/tn3147-migrating-to-the-latest-notarization-tool
  and ```nrn/src/mac/nrn_notarize.sh```

  ```
  % xcrun notarytool log \
    --apple-id "$apple_id" \
    --team-id "$team_id" \
    --password "$app_specific_password" \
    "$id"
  ```
  where $id was printed by the notarization request. The apple_id, team_id, and
  app_specfic_password are set by the script to the contents of the files
  ```
  $HOME/.ssh/apple-id
  $HOME/.ssh/apple-team-id
  $HOME/.ssh/apple-notarization-password
  ```
  

The script ends by printing:
```
  cp $PACKAGE_FILE_NAME $HOME/$PACKAGE_FULL_NAME
  Manually upload $HOME/nrn-9.0a-88-gc6d8b9af6-macosx-10.15-universal2-py-39-310-311.pkg to github
```

I've been uploading the ```$PACKAGE_FULL_NAME``` as an artifact for a
Release version from https://github.com/neuronsimulator/nrn by choosing
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
generally only for the architecture indicated by ```uname -m```. That is
ok for openmpi but since the various python libraries are linked against
during build to create the version specific neuron modules, those python
installers also have to be universal. Fortunately, universal python versions
can be found at [python.org](http://python.org/Downloads/macOS) at least for
(as of 2022-01-01) python3.8, python3.9, and python3.10.

- ```xcode-select --install```:

  - Sadly, notarization requires ```altool``` which is not distributed
    with the command line tools. So one needs to install the full xcode
    (at least version 12) from the App Store. That is very large and may
    take an hour or so to download. (should be checked to see if this is
    still the case.)

- Install latest [XQuartz](http://xquartz.org) release. At least XQuartz-2.8.0_rc1

- Install [PackagesBuild](http://s.sudre.free.fr/Software/Packages/about.html)

- ```brew install coreutils cmake bison flex open-mpi```

  - Python 3.8 is already installed as /usr/bin/python3 and is universal2.

  - The [normal source build](./install_instructions.md#Mac-OS-Depend)
    explains how to install brew and add it to the PATH.
    ```bash
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    echo 'eval $(/opt/homebrew/bin/brew shellenv)' >> $HOME/.zprofile
    eval $(/opt/homebrew/bin/brew shellenv)
    ```

  - Default build uses universal pythons from python.org. I also redundantly
    include their python3.8 as I find it more convenient to be able to
    explicitly specify the full name as python3.8.

  - All pythons used for building need numpy installed, e.g.
    ```
    pip3.9 install --user --upgrade pip
    pip3.9 install --user numpy
    ```

  - At least one python3 in your PATH needs cython installed.
    And, to avoid compile errors, the minimum version is 0.29.26
    ```
    python3.10 -m pip install --user cython
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
