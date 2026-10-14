# Code Formatting

Pull requests fail if python, c++, and cmake files are not formatted according
``CONTRIBUTING.md`` policies. After a CMake configure, ``make format-pr`` or
``ninja format-pr`` (depending on the generator) can be used to ensure all
the files that are a part of the pull request are formatted according to
those policies.

## Dependencies
```
pip install black # format python files
pip install cmake-format=0.6.13 # format cmake files
```
### Linux
```
sudo apt install clang-format
```
### Mac
```
brew install clang-format
```

## Instructions

Clone the nrn repository (a ``git worktree add`` checkout is fine) and
configure with cmake. Cmake will automatically clone a subrepository into
``external/coding-conventions``. No special cmake formatting options are needed.

```
# from the CMake build directory (or with cmake --build)
make format       # formats all cmake, c++, and *.py files
make format-pr    # formats files that differ from the nrn master branch

# equivalent when the generator is Ninja:
ninja format
ninja format-pr
```

Use the same Python environment you use for development (for example Python
3.13) so ``black`` and related tools match CI expectations.

## Behind the scenes

``nrn/.bbp-project.yaml`` specifies the tools used for format python
files (black), cpp,c,h files (ClangFormat version 12.0.1),
and cmake files (CMakeFormat version 0.6.13)

``nrn/.clang-format`` specfies our choices for clang format options.

``nrn/.cmake-format.yaml`` specifies our choices for cmake file format options.

``format-pr`` runs the coding-conventions formatter on paths that differ from
``master``, for example:

```
external/coding-conventions/bin/format $(git diff --name-only master)
```

(The exact invocation is provided by the CMake target; run it via
``make format-pr`` or ``ninja format-pr`` from a configured build tree.)

``black`` can be executed from anywhere with folder args or python file args.
