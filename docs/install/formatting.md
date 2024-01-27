# Code Formatting

Pull requests fail if python, c++, and cmake files are not formatted according
``CONTRIBUTING.md`` policies.  ``make format-pr`` can be used to ensure all
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

Clone the nrn repository and configure with cmake. Cmake will
automatically clone a subrepository into nrn/external/coding-conventions
No special cmake formatting options are needed.

```
make format # formats all cmake, c++, and *.py files.

make format-pr # formats all files that are different from the nrn master branch.
```

## Behind the scenes

``nrn/.bbp-project.yaml`` specifies the tools used for format python
files (black), cpp,c,h files (ClangFormat version 12.0.1),
and cmake files (CMakeFormat version 0.6.13)

``nrn/.clang-format`` specfies our choices for clang format options.

``nrn/.cmake-format.yaml`` specifies our choices for cmake file format options.

``make format-pr`` executes the command
```
cd /home/hines/neuron/format && external/coding-conventions/bin/format `git diff --name-only master`
```

``black`` can be executed from anywhere with folder args or python file args.
