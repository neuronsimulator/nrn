# NEURON contribution guidelines

## Table of contents

* **[Navigation](#nav)**
* **[Getting oriented](#orient)**
* **[Howto](#how)**
* **[Reporting a bug](#bug)**
* **[Code review process](#process)**

## Navigation<a name="nav"></a>

There are several separate repositories that make up the NEURON project and you may contribute to any of these. A single *code board* manages the full set. 
This *Contributing* document is relevant not only for this repository but also for these other repositories.

Five major repositories are directly included at this github:
1. [**nrn**](https://github.com/neuronsimulator/nrn) -- the main NEURON code. When you fork and clone this repository you will find the actual source in subdirectories under *nrn/src*. The subdirectories of primary
interest will generally be those most recently edited -- *nrniv*, *nrnmpi*, *nrnpython*. By contrast some of the subdirectories will rarely if ever need updating.
1. [**progref-py**](https://github.com/neuronsimulator/progref-py) -- NEURON Python programmer's reference. As mentioned further below, the beginning user can often be of particular help to the community by clarifying
documentation and examples which may have seemed clear to the person who wrote it but are lacking details that are needed for a student.
1. [**tutorials**](https://github.com/neuronsimulator/pythontutorial) -- here again, the student or beginning user can be of assistance either by improving existing tutorials or by providing new tutorials to illuminate aspects of
NEURON that he or she had trouble with.
1. [**iv**](https://github.com/neuronsimulator/iv) -- the old Interviews graphical program, now largely deprecated in favor of modern GUIs.
1. [**progref-hoc**](https://github.com/neuronsimulator/progref-hoc) -- this is the _programmers reference_ for the hoc language, the 'higher order calculator' of Kernighan (a co-founder of UNIX). _hoc_ is now largely retired in
favor of Python. The hoc code itself is in the nrn repository and also should not usually be touched.

There are several NEURON-related repositories hosted elsewhere which also encourage contributions. Each of these will have it's own *Contributing* document.
1. [NMODL](https://github.com/BlueBrain/nmodl) -- improved method for compiling and porting *.mod* files using *abstract syntax trees*
1. [CoreNEURON](https://github.com/BlueBrain/CoreNeuron) -- an optimized NEURON for running on high performance computers (HPCs)
1. [NetPyNE](https://github.com/Neurosim-lab/netpyne) -- multiscale modeling tool for developing, simulating and analyzing networks which include complex cells, potentially
with detailed molecular-modeling.
1. [NetPyNE-UI](https://github.com/MetaCell/NetPyNE-UI) -- graphical user interface for NetPyNE

## Getting oriented<a name="orient"></a>

We encourage contributions to the NEURON simulator from individuals at all levels -- students, postdocs, academics, industry coders, etc.
Knowledge of the domain of neural simulation is also helpful but much of the simulation technology is comparable to other simulation fields in biology and beyond --
numerical integration of ordinary differential equations (ODEs), here coupled with *events* (event-driven).

If you want to pick up and try an existing improvement project, you will note that we have indicated levels of difficulty with [labels](https://github.com/neuronsimulator/nrn/labels).
Most internal hacks will require knowledge of C/C++. Knowledge of python is also necessary for writing accompanying test code.
Note that it can be much easier to get started by improving documentation or by adding new tutorials.

There are some things that will make it easier to get your pull requests accepted quickly into the master build where it is teed-up for eventual release.

Before you submit an issue, search the issue tracker to see if the problem or something very similar has already been addressed.
The discussion there might show you some workaround or identify the status of a project.

Consider what kind of change you have in mind:

* For a **Major Feature**, first open an issue and make clear whether you are request the feature from the community or offering to provide the feature yourself.
The *code board* will consider whether this is needed or whether some existing feature does approximately the same thing.
If you would like to yourself *implement* the new feature, please submit an issue with the label **proposal** for your work first.
This will also allow us to better coordinate effort and prevent duplication of work. We will also then be able to help you as you develop the feature so 
that it is can be readily integrated.

* **Small Features** can be submitted directly.

## How to do this<a name="how"></a>
When you're ready to contribute to the code base, please consider the following guidelines:

* Make a [fork](https://guides.github.com/activities/forking/) of this repository. From there you will download your own version of the repository to your machine for
compilation and running.
* Make your changes in your version of the repo in a new git branch:
     ```shell
     git checkout -b my-fix-branch master
     ```
* When creating your fix or addition, consider what **new tests** may be needed for *continuous integration (CI)*, and what **new documentation** or documentation change is needed.
* The full test-suite will run once you submit a pull-request. If you have concerns about the code, or if you've already failed the test-suite, you can 
run the test-suite locally on your machine.
* Pull (fetch and merge) from *upstream* so that your current version contains all changes that have been made since you originally cloned or last pulled.
    ```shell
    git pull --ff upstream master
    ```
* Commit your changes using a descriptive commit message.
     ```shell
     git commit -a
     ```
* Push your branch to GitHub; note that this will push back to your version of NEURON code in your own github repository (the *origin*).
    ```shell
    git push origin my-fix-branch
    ```
* In GitHub, send a Pull Request to the `master` branch of the upstream repository of the relevant component -- this will run the test-suite before alerting the team.
* If we suggest changes then:
  * Make the required updates.
  * Rewrite; Rerun test-suites; Repush; Rereq

After your pull request is merged, you can safely delete your branch and pull the changes from the main (upstream) repository.

Development conventions:
NEURON code is being built according to C/C++, Python best-practices. The easiest way to figure out what this is is to take a look at current code and copy the way things are
formatted, indented, documented, and commented.

### Code Formatting

Currently we have enabled CMake code formatting using [cmake-format](https://github.com/cheshirekow/cmake_format). Before submitting PR, if you have changed any CMake build related code, make sure to run cmake-format as below:

* Make sure to install cmake-format utility with Python version you are using:

```
pip3.7 install cmake-format==0.6.0 --user
```
Now you should have `cmake-format` command available.

* Use `-DNEURON_CMAKE_FORMAT=ON` option of CMake to enable CMake code formatting targets:

```
cmake .. -DPYTHON_EXECUTABLE=`which python3.7` -DNEURON_CMAKE_FORMAT=ON
```

With this, new target called **cmake-format** can be used to automatically format all CMake files:

```
$ make cmake-format
Scanning dependencies of target cmake-format
Built target cmake-format
```

You can now use `git diff` to see how cmake-format has formatted existing CMake files.

Note that if you want to exclude specific code section to be not formatted (e.g. comment blocks), you can use guards:

```
# ~~~
# This comment is fenced
#   and will not be formatted
# ~~~
```

Or,

```
# cmake-format: off
# This bunny should remain untouched:
# . 　 ＿　∩
# cmake-format: on
```

See [cmake-format](https://github.com/cheshirekow/cmake_format) documentation for details.

## Reporting a bug<a name="bug"></a>

Have you tested on the current alpha version of NEURON? If not, please clone, compile, install and run with current version (this one).

Please let us know what operating system you were using when you found the bug. If you have access to another operating system, it is helpful if you can find out if the bug
shows up there as well. Please indicate which operating system(s) the bug has been found in.

In order to address a bug quickly, we need to reproduce and confirm it. 
Usually bugs will arise in the context of a simulation which will in many cases be extremely large. 
Please provide a *simple, short* example (zipped with any associated mod files and a README). 
The simplifying process may require considerable work to isolate the bug arising somewhere in the midst of a large simulation.
Sometimes, this process alone is enough to identify the bug as a function limitation or documentation insufficiency, rather than a code bug per se.

## Code review process<a name="process"></a>

Pull requests are reviewed by the development team.
If you don't receive any feedback after a couple weeks, please follow up with a new comment.

