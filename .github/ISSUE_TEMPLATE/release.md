---
name: NEURON Major/Minor Release
about: Create a NEURON release for a new branch from master.
title: 'NEURON [x.y.z] release'
labels: 'release'
assignees: ''

---

Action items
============

GHA release knobs
---
[NEURON Release](https://github.com/neuronsimulator/nrn/actions/workflows/release.yml) is the release path (wheels, ModelDB CI, nrn-build-ci, full-src, Windows installer, tag, PyPI). Three knobs:

* **Controller** — branch whose workflow YAML runs (`Use workflow from`). Ship from **`master`**.
* **`rel_branch`** — content to build (`release/x.y`).
* **`rel_tag`** — version name (`x.y.z`).

`upload=false` is a dry-run (no git tag, no GitHub release, no PyPI). `upload=true` ships. See [wheel publishing](https://nrn.readthedocs.io/en/latest/install/python_wheels.html#publishing-the-wheels-on-pypi-via-github-actions).

Sanity checks
---
- [ ] Create `release/x.y` branch and make sure GitHub Actions pass (Azure still runs on `release/*` if enabled; it is not the publish path)
- [ ] Dry-run [NEURON Release](https://github.com/neuronsimulator/nrn/actions/workflows/release.yml) from **`master`**: `rel_branch=release/x.y`, `rel_tag=x.y.z`, **`upload=false`**. Confirm wheels are green, `neuron.__version__` is non-empty, and ModelDB V2 uses **this run’s** `wheels` artifact
- [ ] If only ModelDB failed, do **not** rebuild wheels: [ModelDB CI (reuse wheels)](https://github.com/neuronsimulator/nrn/actions/workflows/modeldb-ci-reuse-wheels.yml) with the dry-run `wheels` artifact id or URL, `neuron_v1=neuron==<previous>`, `modeldb_ci_ref=master` (optional `models_to_run` for a subset)
- [ ] Run any tests not contained in `nrn-build-ci` and `nrn-modeldb-ci`
- [ ] Freeze SHAs: dry-run `rel_branch` tip == the commit you will ship


Releasing
---
- [ ] Update semantic version in `CMakeLists.txt`
- [ ] Update changelog below and agree on it with everyone; then commit it to `docs/changelog` (copy structure as-is)
- [ ] Activate ReadTheDocs for `release/x.y` (Hidden until ship). If the branch is missing from [the versions page](https://readthedocs.org/projects/nrn/versions/), use **+ Add version**. Inspect the Changelog page after the build.
- [ ] Create, test and upload manual artifacts
  - [ ] MacOS package installer (manual task, ask Michael)
- [ ] Ship with the **same** controller (`master`), `rel_branch`, `rel_tag`, and SHAs as the last green dry-run, **`upload=true`**. This creates the annotated tag, a **pre-release** on GitHub (full-src-package and Windows installer attach when those jobs finish), and publishes wheels to PyPI.
- [ ] Once wheels are on PyPI, activate the `x.y.z` **tag** on ReadTheDocs: [versions page](https://readthedocs.org/projects/nrn/versions/) → **+ Add version** if the tag is not listed (new/unbuilt tags are hidden under “Recently built”). Leave it **not** Hidden.
- [ ] Publish release on GitHub (edit https://github.com/neuronsimulator/nrn/releases/tag/x.y.z and un-tick the pre-release checkbox)


Post-release
---
- [ ] To mark the start of a new development cycle, tag `master` as follows:
  - minor version: `x.(y+1).dev`
  - major version: `(x+1).0.dev`
- [ ] Deactivate ReadTheDocs build for `release/x.y` (keep the `x.y.z` tag active)
- [ ] Go to [ReadTheDocs advanced settings](https://readthedocs.org/dashboard/nrn/advanced/) and set `Default version` to `x.y.z`
- [ ] Let people know :rocket:
- [ ] Cherrypick changelog to `master`
- [ ] Update the changelog for the release on GitHub
- [ ] Update `codemeta.json` (`master` branch only) with the new version, changelog, date, and links

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
