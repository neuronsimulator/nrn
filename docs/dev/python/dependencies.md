# Python dependency updates

NEURON keeps two layers of Python dependency pins.

The direct requirement files and `pyproject.toml` carry `<=` ceilings from
[#3338](https://github.com/neuronsimulator/nrn/pull/3338). A ceiling is the
newest release known to work. Those ceilings are what stop a new PyPI release
from floating CI, or a wheel build, onto a broken version.

CI does not install those files. It installs the hashed lock
`ci/requirements.txt`, produced by `uv pip compile`. That lock includes the
docs and test graph (Jupyter, notebook, Pillow, and the rest), not only the
packages NEURON imports.

`.github/dependabot.yml` tells Dependabot how to propose changes. It does not
turn security updates on or off. That switch is the repository setting
**Dependabot security updates**.

## Version updates

Once a month Dependabot opens at most one pull request. It may edit:

* `nrn_requirements.txt`
* `ci_requirements.txt`
* `nmodl_requirements.txt`
* `pyproject.toml`
* `docs/docs_requirements.txt`
* `packaging/python/build_requirements.txt`
* `packaging/python/test_requirements.txt`
* `packaging/python/oldest_numpy_requirements.txt`

`versioning-strategy: increase-if-necessary` raises a ceiling only when the
current constraint does not already allow the new release. A lower bound such
as `numpy>=1.9.3` in `pyproject.toml` `[project.dependencies]` already allows
the newest NumPy, so that line should stay a lower bound. Check the diff for
that before merging.

`open-pull-requests-limit: 1` on that entry means a second version-update
pull request waits until this one is merged or closed.

## Regenerating the lock

CI keeps installing the old lock until `ci/requirements.txt` is regenerated.
After reviewing the ceiling changes, regenerate the lock with the command in
the header of `ci/requirements.txt` and include the result in the same pull
request. Today that command is:

```bash
uv pip compile nrn_requirements.txt ci_requirements.txt docs/docs_requirements.txt packaging/python/test_requirements.txt -o ci/requirements.txt --universal --python-version 3.9 --generate-hashes
```

A major bump, such as pytest 9, is a decision inside that diff. Merge the
pull request when the lock matches the ceilings and CI is green.

The `/ci` entry sets `open-pull-requests-limit: 0`, so Dependabot does not
open version-update pull requests for `ci/requirements.txt` or
`ci/uv_requirements.txt`. A three-line edit of the lock is not a review unit.
The lock moves when a person regenerates it.

## Security updates

Security-update pull requests are created only while **Dependabot security
updates** is enabled. Leave that setting paused until the version-update job
log described below has been checked.

When the setting is on, this file groups those pull requests:

* one group for the direct manifests listed above
* one group for `/ci` (`ci/requirements.txt` and `ci/uv_requirements.txt`)

`open-pull-requests-limit` does not apply to security updates. The groups do.
Alerts stay on the security tab either way. They clear when the patched
release is in the lock. The NumPy alerts on
`packaging/python/oldest_numpy_requirements.txt` are the intentional old pin;
dismiss those two with that reason.

## Pull requests to close

Close a Dependabot pull request that:

* edits `packaging/python/oldest_numpy_requirements.txt` (that file shares a
  directory with the build and test requirements, so Dependabot can see it)
* edits only `ci/requirements.txt` or `ci/uv_requirements.txt` without a
  matching ceiling change and a full lock regeneration

The security-update pull requests that were already open before this file
existed are duplicates of that second kind. Close them after this file is on
`master` and the job log looks right. They predate the groups.

## First run after this file reaches master

GitHub reads `.github/dependabot.yml` only from the default branch. Merging
it starts a version-update check immediately, without waiting for the monthly
schedule. The same day, open **Insights → Dependency graph → Dependabot** and
read the job log.

The log should show:

* the direct manifests listed above, and no configuration error
* no version-update pull request that edits only the lock
* at most one version-update pull request

`CMakeLists.txt` and `docs/rst_substitutions.txt` sit next to requirement
files but are not manifests. If the log treats them as manifests, the
directory list needs a follow-up. A follow-up edit of this config starts
another immediate check.
