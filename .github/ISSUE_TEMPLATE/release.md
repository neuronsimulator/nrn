---
name: NEURON Release
about: Create a NEURON release for a new branch from master.
title: 'NEURON [x.y.z] release'
labels: 'release'
assignees: ''

---

Action items
============

Pre-release
---
- [ ] Look out for ModelDB regressions by analyzing [nrn-modeldb-ci last version vs nightly reports](https://github.com/neuronsimulator/nrn-modeldb-ci/actions/workflows/nrn-modeldb-ci.yaml?query=event%3Aschedule++)
- [ ] Create CoreNEURON release & update submodule

Sanity checks
---
- [ ] Create `release/x.y` branch and make sure GitHub Actions builds pass
- [ ] Run [nrn-build-ci](https://github.com/neuronsimulator/nrn-build-ci/actions/workflows/build-neuron.yml) for the respective GitHub Actions build; see [the guide](https://github.com/neuronsimulator/nrn-build-ci#wheels-testing---manual-workflow)
- [ ] Activate ReadTheDocs build for `release/x.y` & make it hidden. Check docs are fine after build is done.
- [ ] Run a test wheel build WITHOUT upload for `release/x.y` to ensure all the wheels build ([see details](https://nrn.readthedocs.io/en/latest/install/python_wheels.html#publishing-the-wheels-on-pypi-via-github-actions))


Releasing
---
- [ ] Update changelog below and agree on it with everyone; then commit it to `docs/changelog` (copy structure as-is)
- [ ] Update `docs/index.rst` accordingly with the new `.pkg` and `.exe` links for `PKG installer` and `Windows Installer`
- [ ] Run the ReadTheDocs build again for `release-x.y`, make sure the build passes and inspect the Changelog page.
- [ ] Create new release+tag on GitHub via [release workflow](https://github.com/neuronsimulator/nrn/actions/workflows/release.yml?query=workflow%3A%22NEURON+Release%22). Note that the GitHub release will be marked as pre-release and  will contain the full-src-package and the Windows installer at the end of the release workflow.
- [ ] Build release wheels but WITHOUT upload ([see details](https://nrn.readthedocs.io/en/latest/install/python_wheels.html#publishing-the-wheels-on-pypi-via-github-actions))
- [ ] Create, test and upload manual artifacts
  - [ ] MacOS package installer (manual task, ask Michael)
- [ ] Publish the `x.y.z` wheels on Pypi; see [wheel publishing instructions](https://nrn.readthedocs.io/en/latest/install/python_wheels.html#publishing-the-wheels-on-pypi-via-github-actions)
- [ ] Once wheels are published, activate the `x.y.z` tag on ReadTheDocs
- [ ] Publish release on GitHub (edit https://github.com/neuronsimulator/nrn/releases/tag/x.y.z)


Post-release
---
- [ ] Tag `master` with `x.(y+1).dev` to mark the start of a new development cycle
- [ ] Deactivate ReadTheDocs build for release/x.y
- [ ] Let people know :rocket:


Changelog
======

# NEURON X.Y

## [x.y.z]
_Release Date_ : DD-MM-YYYY


### What's New
* [List new features/support added]
* .....

### Breaking Changes
* [List the changes that aren't backward compatible]
* ...


### Deprecations
* [List the features that are deprecated]
* ...


### Bug Fixes
* [List the important bug fixes]
* ...


### Improvements /  Other Changes
* [List the improvements made in the new release and any other changes]
* ...

### Upgrade Steps
* [Describe how to migrate from previous NEURON Version]
* ...

For the complete list of features and bug fixes, see the list in [GitHub Issue #[GH_no.]](https://github.com/neuronsimulator/nrn/issues/#[GH_no.])

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
