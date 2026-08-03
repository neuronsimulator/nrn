# Managed CI assets (`managed: true`)

Files here are the primary source for `ci/deps/fetch.sh` (local resolution).
Each file should have a matching row in `../MANIFEST.yml` with `managed: true`.

Currently:

| File | Manifest id |
|------|-------------|
| `mpich_4.2.0-5.1_amd64.deb` | `mpich-noble-4.2.0-5.1` |
| `libmpich12_4.2.0-5.1_amd64.deb` | `libmpich12-noble-4.2.0-5.1` |

Do not edit blobs in place without updating `sha256` in `../MANIFEST.yml`.
