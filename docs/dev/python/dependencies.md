# Updating Python dependencies

Use this guide when changing Python dependency constraints or reviewing a
Dependabot pull request.

## Know which file to change

The source files declare dependency constraints. CI installs a generated,
fully pinned file from those sources.

| File | Role |
| --- | --- |
| `nrn_requirements.txt` | Main development and CI dependencies; `-r` lines include the build, test, and NMODL requirement files below. |
| `ci_requirements.txt` | Additional CI tools. |
| `nmodl_requirements.txt` | NMODL dependencies, included through `nrn_requirements.txt`. |
| `docs/docs_requirements.txt` | Documentation build dependencies. |
| `packaging/python/build_requirements.txt` | Python wheel build dependencies. |
| `packaging/python/test_requirements.txt` | Python package tests; included in the CI compile command. |
| `pyproject.toml` | Python build-system requirements and dependencies installed for NEURON users. |
| `packaging/python/oldest_numpy_requirements.txt` | Deliberately old NumPy versions for compatibility testing. |
| `ci/requirements.txt` | Generated CI requirements with exact versions and hashes. Do not edit by hand. |
| `ci/uv_requirements.txt` | Sets an allowed version range for the `uv` tool CI uses. This is a tool constraint, not NEURON's dependency set. |

Many source constraints have an upper bound (`<=`). Each bound is the highest
PyPI release identified as compatible with NEURON when that constraint was
last reviewed. It prevents builds from silently picking a newer, unverified
release; it does not mean every platform and Python version was exhaustively
tested with the bounded release. Treat raising a ceiling as a compatibility
change, and check the relevant CI and build configurations before doing so.

## Update a dependency

1. Change the source file that declares the dependency for the build or test
  you intend to affect. If the same dependency is declared in multiple files
  for the same purpose, keep those constraints aligned. Some differences are
  intentional: for example, `packaging/python/oldest_numpy_requirements.txt`
  tests older NumPy versions, while `numpy>=1.9.3` in `pyproject.toml` is an
  open-ended lower bound for NEURON users.
2. If a file read by the `uv pip compile` command below changes, regenerate
  `ci/requirements.txt` and include it in the same pull request. The exact
  inputs, including files brought in with `-r`, are listed below. That command
  does not read `pyproject.toml` or `ci/uv_requirements.txt`.
3. Review the complete diff, especially major upgrades and compatibility
   bounds, then run the relevant tests and CI checks.

### Regenerate CI requirements

`uv` is a Python package installer and dependency resolver. In this repository,
CI uses it in two ways: `uv pip compile` resolves the source constraints into
`ci/requirements.txt`, and `uv pip install` installs that compiled set. The
compile output pins transitive dependencies and includes hashes, so CI gets a
repeatable dependency set instead of resolving the latest allowed versions on
every run.

CI installs `uv` using the constraint in `ci/uv_requirements.txt`. That file
currently allows versions up to `0.7.3`; it does not select one exact version.
To install an allowed version locally, run:

```bash
python -m pip install -r ci/uv_requirements.txt
```

From the repository root, regenerate the compiled CI requirements with the
command recorded in the header of `ci/requirements.txt`. The command reads
these files directly:

* `nrn_requirements.txt` (which includes `packaging/python/build_requirements.txt`,
  `packaging/python/test_requirements.txt`, and `nmodl_requirements.txt` using
  pip's `-r` syntax)
* `ci_requirements.txt`
* `docs/docs_requirements.txt`
* `packaging/python/test_requirements.txt`

The test requirements file is both included by `nrn_requirements.txt` and
listed directly in the command. `uv pip compile` resolves all these constraints
together; it does not search the repository for every requirements file.

Run:

```bash
uv pip compile \
  nrn_requirements.txt \
  ci_requirements.txt \
  docs/docs_requirements.txt \
  packaging/python/test_requirements.txt \
  -o ci/requirements.txt \
  --universal \
  --python-version 3.9 \
  --generate-hashes
```

Review the generated diff. It can include transitive package changes, not just
the dependency you changed. A major upgrade, such as a new pytest major
version, needs an explicit decision.

## Review Dependabot pull requests

Dependabot is configured in `.github/dependabot.yml`:

* It checks `/`, `/docs`, and `/packaging/python` monthly. These paths are
  the directories scanned for manifests; `/` means the repository root, not
  every subdirectory under it. Version updates for these directories are one
  group (`python-direct`, `patterns: ["*"]`): one pull request updates every
  dependency whose constraint allows a newer release. Security updates for
  the same directories are a second group (`python-direct-security`, also
  `patterns: ["*"]`): one pull request for every vulnerable dependency there.
* Only one version-update pull request may be open for that entry at a time.
  Further version-update pull requests wait until it is merged or closed.
* Scheduled version updates look for newer releases. Security updates address
  known vulnerabilities and run only if the repository's **Dependabot security
  updates** setting is enabled. Version updates for `/ci` are disabled; the
  zero PR limit there does not disable security updates. Security updates
  under `/ci` are one group (`ci-security`, `patterns: ["*"]`).

The direct-update entry uses `increase-if-necessary`: Dependabot leaves a
constraint unchanged when it already allows the proposed version; otherwise it
changes the constraint to allow the update. Review any changed upper bounds as
compatibility decisions.

When reviewing a PR, check which source files it changes and whether changes
to the compiler inputs listed above are accompanied by a regenerated
`ci/requirements.txt`.
Do not accept an automated change to `packaging/python/oldest_numpy_requirements.txt`
without reviewing the compatibility-test intent. Inspect the full PR before
closing it, since it may contain other useful updates too.

## Check the first Dependabot run

GitHub reads `.github/dependabot.yml` from the default branch. When this file
first reaches that branch, Dependabot starts a check immediately. Review the
job log under **Insights → Dependency graph → Dependabot** and confirm that:

* the expected requirement files were found, with no configuration errors;
* no version-update PR was opened for `/ci`;
* proposed source changes and any regenerated `ci/requirements.txt` agree.

Security updates only run if enabled in the repository settings. Alerts remain
until patched versions are represented in the dependencies used by CI. The
old NumPy pins are intentional; dismiss their alerts only with that reason.
