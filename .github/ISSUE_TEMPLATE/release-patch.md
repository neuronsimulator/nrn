---
name: NEURON Patch Release
about: Create a NEURON Patch release for an existing branch.
title: 'NEURON [x.y.z] patch release'
labels: 'release'
assignees: ''

---

Action items
============

GHA release knobs
---
[NEURON Release](https://github.com/neuronsimulator/nrn/actions/workflows/release.yml) is the release path (wheels, ModelDB CI, nrn-build-ci, full-src, Windows installer, tag, PyPI). Three knobs:

* **Controller** — branch whose workflow YAML runs (`Use workflow from`). Ship from **`master`**.
* **`rel_branch`** — content to build (usually `release/x.y`).
* **`rel_tag`** — version name (`x.y.z`).

`upload=false` is a dry-run (no git tag, no GitHub release, no PyPI). `upload=true` ships. See [wheel publishing](https://nrn.readthedocs.io/en/latest/install/python_wheels.html#publishing-the-wheels-on-pypi-via-github-actions).

Pre-release
---
- [ ] Create a cherrypicks branch where all commits go into new release and open a PR against `release/x.y` branch
- [ ] Watch for ModelDB regressions (dry-run ModelDB below, or [ModelDB CI (reuse wheels)](https://github.com/neuronsimulator/nrn/actions/workflows/modeldb-ci-reuse-wheels.yml) against a prior `wheels` artifact vs the previous PyPI version). Local `nrn-modeldb-ci` is optional.
- [ ] Update cherrypicks PR:
  - [ ] Update semantic version in `CMakeLists.txt`
  - [ ] Update changelog below and agree on it with everyone; then commit it to `docs/changelog` in the cherrypicks PR (copy structure as-is)
- [ ] Activate ReadTheDocs for the cherry-pick branch and ensure the documentation builds (when logged in, go to [the versions page](https://readthedocs.org/projects/nrn/versions/) and set the version to Active and Hidden; if the branch is missing, use **+ Add version**)
- [ ] Optional: [NEURON Release](https://github.com/neuronsimulator/nrn/actions/workflows/release.yml) dry-run on the cherry-pick branch (`Use workflow from` = `master`, `rel_branch` = the cherry-pick branch, `rel_tag` = `x.y.z`, **`upload=false`**) to confirm wheels build before merge

Sanity checks
---
- [ ] After cherrypicks PR is merged, make sure GitHub Actions pass for `release/x.y` (Azure still runs on `release/*` if enabled; it is not the publish path)
- [ ] Dry-run [NEURON Release](https://github.com/neuronsimulator/nrn/actions/workflows/release.yml) from **`master`**: `rel_branch=release/x.y`, `rel_tag=x.y.z`, **`upload=false`**. Confirm wheels are green, `neuron.__version__` is non-empty, and ModelDB V2 uses **this run’s** `wheels` artifact
- [ ] If only ModelDB failed, do **not** rebuild wheels: [ModelDB CI (reuse wheels)](https://github.com/neuronsimulator/nrn/actions/workflows/modeldb-ci-reuse-wheels.yml) with the dry-run `wheels` artifact id or URL, `neuron_v1=neuron==<previous>`, `modeldb_ci_ref=master` (optional `models_to_run` for a subset)
- [ ] nrn-build-ci runs inside NEURON Release; optional extra run: [nrn-build-ci](https://github.com/neuronsimulator/nrn-build-ci/actions/workflows/build-neuron.yml) with the same GHA `wheels` artifact URL ([manual workflow](https://github.com/neuronsimulator/nrn-build-ci#wheels-testing---manual-workflow))
- [ ] Activate ReadTheDocs build for `release/x.y` and make it hidden. Check docs after the build. If the branch is missing, **+ Add version**.
- [ ] Run BBP Simulation Stack & other relevant tests
- [ ] Freeze SHAs: dry-run `rel_branch` tip == the commit you will ship


Releasing
---
- [ ] Ship with the **same** controller (`master`), `rel_branch`, `rel_tag`, and SHAs as the last green dry-run, **`upload=true`**. This creates the annotated tag, a **pre-release** on GitHub (full-src-package and Windows installer attach when those jobs finish), and publishes wheels to PyPI.
- [ ] Create, test and upload manual artifacts
  - [ ] MacOS package installer (manual task, ask Michael)
- [ ] Once wheels are on PyPI, activate the `x.y.z` **tag** on ReadTheDocs: [versions page](https://readthedocs.org/projects/nrn/versions/) → **+ Add version** if the tag is not listed (new/unbuilt tags are hidden under “Recently built”). Leave it **not** Hidden.
- [ ] Publish release on GitHub (edit https://github.com/neuronsimulator/nrn/releases/tag/x.y.z and un-tick the pre-release checkbox)


Post-release
---
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
