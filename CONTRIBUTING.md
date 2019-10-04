# NEURON contribution guidelines

## Table of contents

* **[What is this?](#what)**
* **[Contribute what?](https://github.com/unicef/magicbox/blob/master/.github/CONTRIBUTING.md#contribute-what)**
* **[Ground rules](https://github.com/unicef/magicbox/blob/master/.github/CONTRIBUTING.md#ground-rules)**
* **[Getting started](https://github.com/unicef/magicbox/blob/master/.github/CONTRIBUTING.md#getting-started)**
* **[Report a bug](https://github.com/unicef/magicbox/blob/master/.github/CONTRIBUTING.md#report-a-bug)**
* **[Code review process](https://github.com/unicef/magicbox/blob/master/.github/CONTRIBUTING.md#code-review-process)**
* **[Community](https://github.com/unicef/magicbox/blob/master/.github/CONTRIBUTING.md#community)**

## What is this?<a name="what"></a>

We encourage contributions to the NEURON simulator from individuals at all levels -- students, postdocs, academics, industry coders, etc.
Knowledge of the domain of neural simulation is also helpful but much of the simulation technology is comparable to other simulation fields in biology and beyond --
numerical integration of ordinary differential equations (ODEs), here coupled with *events* (event-driven).

## How to get started

If you want to pick up and try an existing improvement project, you will note that we have indicated levels of difficulty with [labels][github/labels].
Most internal hacks will require knowledge of C/C++. Knowledge of python is also necessary for writing accompanying test code.

Even if you are not contributing, you may want to suggest a needed improvement, or identify a problem or bug.
For a bug in the source code, please submit an issue; better yet, make the fix and submit a Pull Request.
You can also *request* a new feature by submitting an issue -- consider first labeling it as a **question** to let all consider whether this is needed or whether some existing
feature does approximately the same thing.
If you would like to yourself *implement* a new feature, please submit an issue with the label **proposal** for your work first, to be sure that we can use it.

There are some things that will make it easier to get your pull requests accepted quickly into the master build where it is teed-up for eventual release.

First, consider what kind of change it is:

* For a **Major Feature**, first open an issue and outline your proposal so that it can be
discussed. This will also allow us to better coordinate our efforts, prevent duplication of work, and help you to craft the change so that it is successfully accepted into the project.
* **Small Features** can be crafted and directly.

Before you submit an issue, please search the issue tracker, maybe an issue for your problem already exists and the discussion might inform you of workarounds readily available.

We want to address all issues as soon as possible, but before fixing a bug we need to reproduce and confirm it. In order to reproduce bugs we will need as much information as
possible, along with a *simple, short* Python example (zipped with any associated mod files and a README). 
This may require considerable work to isolate the presumed bug arising somewhere in the midst of a large simulation.
Sometimes, this process alone is enough to identify the bug as a function limitation or documentation insufficiency, rather than a code bug per se.

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
  * Rerun test-suites; Repush; Rereq

After your pull request is merged, you can safely delete your branch and pull the changes from the main (upstream) repository.

Development conventions:
NEURON code is being built according to C/C++, Python best-practices. The easiest way to figure out what this is is to take a look at current code and copy the way things are
formatted, indented, documented, and commented.
