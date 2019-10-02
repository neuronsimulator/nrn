# 

We encourage contributions to the NEURON simulator from individuals all levels -- students, postdocs, academics, industry, coders, etc.
Most internal hacks will require knowledge of C/C++. Knowledge of python is also necessary, at least for testing.
Knowledge of the domain of neural simulation is also helpful but much of the simulation technology is comparable to other simulation fields in biology and beyond which
feature numerical integration of ordinary differential equations (ODEs), here coupled with event-driven systems.

We have indicated levels of difficulty with [labels][github/labels].

Even if you are not contributing, you may want to suggest a needed improvement, or identify a problem or bug.
If you are *requesting* a new feature by [submitting an issue](#submit-issue) to this GitHub Repository. 
If you would like to yourself *implement* a new feature, please submit an issue with a proposal for your work first, to be sure that we can use it.

As a contributor, there are a set of guidelines that will make it easy to accept your pull requests into the master build where it is teed-up for eventual release.
 - [Question or Problem?](#question)
 - [Issues and Bugs](#issue)
 - [Feature Requests](#feature)
 - [Submission Guidelines](#submit)
 - [Development Conventions](#devconv)

If you find a bug in the source code, you can help us by [submitting an issue](#submit-issue) to our [GitHub Repository][github]. Even better, you can [submit a Pull Request](#submit-pr) with a fix.

Please consider what kind of change it is:

* For a **Major Feature**, first open an issue and outline your proposal so that it can be
discussed. This will also allow us to better coordinate our efforts, prevent duplication of work,
and help you to craft the change so that it is successfully accepted into the project.
* **Small Features** can be crafted and directly [submitted as a Pull Request](#submit-pr).

# <a name="submit"></a> Submission Guidelines

## <a name="submit-issue"></a> Submitting an Issue

Before you submit an issue, please search the issue tracker, maybe an issue for your problem already exists and the
discussion might inform you of workarounds readily available.

We want to fix all the issues as soon as possible, but before fixing a bug we need to reproduce and confirm it. In order to reproduce bugs we will need as much information as possible, and preferably a sample MOD file or Python example.

## <a name="submit-pr"></a> Submitting a Pull Request (PR)

When you wish to contribute to the code base, please consider the following guidelines:

* Make a [fork](https://guides.github.com/activities/forking/) of this repository.
* Make your changes in your fork, in a new git branch:

     ```shell
     git checkout -b my-fix-branch master
     ```
* Create your patch, **including appropriate test cases**.
* Enable `NMODL_FORMATTING` and `NMODL_PRECOMMIT` CMake variables
  to ensure that your change follows coding conventions of this project.
  Please see [README.md](./README.md) for more information.
* Run the full test suite, and ensure that all tests pass.
* Commit your changes using a descriptive commit message.

     ```shell
     git commit -a
     ```
* Push your branch to GitHub:

    ```shell
    git push origin my-fix-branch
    ```
* In GitHub, send a Pull Request to the `master` branch of the upstream repository of the relevant component.
* If we suggest changes then:
  * Make the required updates.
  * Re-run the test suites to ensure tests are still passing.
  * Rebase your branch and force push to your GitHub repository (this will update your Pull Request):

       ```shell
        git rebase master -i
        git push -f
       ```

That’s it! Thank you for your contribution!

#### After your pull request is merged

After your pull request is merged, you can safely delete your branch and pull the changes from the main (upstream)
repository:

* Delete the remote branch on GitHub either through the GitHub web UI or your local shell as follows:

    ```shell
    git push origin --delete my-fix-branch
    ```
* Check out the master branch:

    ```shell
    git checkout master -f
    ```
* Delete the local branch:

    ```shell
    git branch -D my-fix-branch
    ```
* Update your master with the latest upstream version:

    ```shell
    git pull --ff upstream master
    ```

[github]: https://github.com/BlueBrain/nmodl

## <a name="devconv"></a> Development Conventions

If you are developing NMODL, make sure to enable both `NMODL_FORMATTING` and `NMODL_PRECOMMIT`
CMake variables to ensure that your contributions follow the coding conventions of this project:

```cmake
cmake -DNMODL_FORMATTING:BOOL=ON -DNMODL_PRECOMMIT:BOOL=ON <path>
```

The first variable provides the following additional targets to format
C, C++, and CMake files:

```
make clang-format cmake-format
```

The second option activates Git hooks that will discard commits that
do not comply with coding conventions of this project. These 2 CMake variables require additional utilities:

* [ClangFormat 7](https://releases.llvm.org/7.0.0/tools/clang/docs/ClangFormat.html)
* [cmake-format](https://github.com/cheshirekow/cmake_format)
* [pre-commit](https://pre-commit.com/)

clang-format can be installed on Linux thanks
to (http://apt.llvm.org/). On MacOS, there is a
[brew recipe](https://gist.github.com/ffeu/0460bb1349fa7e4ab4c459a6192cbb25)
to install clang-format 7. _cmake-format_ and _pre-commit_ utilities can be installed with *pip*.


## Memory Leaks and clang-tidy

If you want to test for memory leaks, do :

```
valgrind --leak-check=full --track-origins=yes  ./bin/nmodl_lexer
```

Or using CTest as:

```
ctest -T memcheck
```

If you want to enable `clang-tidy` checks with CMake, make sure to have `CMake >= 3.5` and use following cmake option:

```
cmake .. -DENABLE_CLANG_TIDY=ON
```


