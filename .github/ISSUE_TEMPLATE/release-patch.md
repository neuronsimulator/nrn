---
name: NEURON Patch Release
about: Create a NEURON Patch release for an existing branch.
title: 'NEURON [x.y.z] patch release'
labels: 'release'
assignees: ''

---

Action items
============

Pre-release
---
- [ ] Create a cherrypicks branch where all commits go into new release and open a PR against `release/x.y` branch
- [ ] Make sure to look out for ModelDB regressions by manually submitting and analyzing [nrn-modeldb-ci](https://github.com/neuronsimulator/nrn-modeldb-ci/actions/workflows/nrn-modeldb-ci.yaml?query=event%3Aschedule++) for the cherrypicks branch vs previous version
- [ ] Update cherrypicks PR:
  - [ ] Update semantic version in `CMakeLists.txt`
  - [ ] Update changelog below and agree on it with everyone; then commit it to `docs/changelog` in the cherrypicks PR (copy structure as-is)
  - [ ] Update `docs/index.rst` accordingly with the new `.pkg` and `.exe` links for `PKG installer` and `Windows Installer`

Sanity checks
---
- [ ] After cherrypicks PR is merged, make sure GitHub, Azure and CircleCI builds pass for `release/x.y` branch
- [ ] Run [nrn-build-ci](https://github.com/neuronsimulator/nrn-build-ci/actions/workflows/build-neuron.yml) for the `release/x.y` branch; see [nrn-build-ci guide](https://github.com/neuronsimulator/nrn-build-ci#azure-wheels-testing---manual-workflow)
- [ ] Activate ReadTheDocs build for `release/x.y` & make it hidden. Check docs are fine after build is done.
- [ ] Run BBP Simulation Stack & other relevant tests


Releasing
---
- [ ] Create new release+tag on GitHub via [release workflow](https://github.com/neuronsimulator/nrn/actions/workflows/release.yml?query=workflow%3A%22NEURON+Release%22). Note that the GitHub release will be marked as pre-release and  will contain the full-src-package and the Windows installer at the end of the release workflow.
- [ ] Build release wheels but WITHOUT upload ([see details](https://nrn.readthedocs.io/en/latest/install/python_wheels.html#publishing-the-wheels-on-pypi-via-azure))
- [ ] Create, test and upload manual artifacts
  - [ ] MacOS package installer (manual task, ask Michael)
  - [ ] arm64 wheels (manual task, check with Alex or Pramod)
  - [ ] aarch64 wheels (use existing `release/x.y-aarch64` branch for this, see [guide](https://nrn.readthedocs.io/en/latest/install/python_wheels.html#publishing-the-wheels-on-pypi-via-circleci))
- [ ] Publish the `x.y.z` wheels on PyPI; see [wheel publishing instructions](https://nrn.readthedocs.io/en/latest/install/python_wheels.html#publishing-the-wheels-on-pypi-via-azure)
- [ ] Once wheels are published, activate the `x.y.z` tag on ReadTheDocs
- [ ] Rename the Windows installer in the GitHub release to match the new version and the supported python versions (i.e. `nrn-8.2.2.w64-mingw-py-37-38-39-310-311-setup.exe`)
- [ ] Publish release on GitHub (edit https://github.com/neuronsimulator/nrn/releases/tag/x.y.z and un-tick the pre-release checkbox)


Post-release
---
- [ ] Deactivate ReadTheDocs build for `release/x.y`
- [ ] Go to [ReadTheDocs advanced settings](https://readthedocs.org/dashboard/nrn/advanced/) and set `Default version` to `x.y.z`
- [ ] Let people know :rocket:
- [ ] Cherrypick changelog and installer links to `master`
- [ ] Update the changelog for the release on GitHub


Changelog
======

# NEURON X.Y

## [x.y.z]
_Release Date_ : DD-MM-YYYY


### What's New
* [List new features/support added]
* .....


### Bug Fixes
* [List the important bug fixes]
* ...


### Improvements /  Other Changes
* [List the improvements made in the new release and any other changes]
* ...


For the complete list of commits check  [GitHub Issue #[GH_no.]](https://github.com/neuronsimulator/nrn/issues/#[GH_no.])

ReadTheDocs sneak peek
======================
* https://nrn.readthedocs.io/en/release-x.y

Commits going into x.y.z
========================

[given `a.b.c` is the last release:]

Since [a.b.c], with:
```bash
git log --pretty=format:"%h : %s" a.b.c..release/x.y
```
we get:

- [ ] commit 1
- [ ] commit 2
- [ ] ...
