# 8.0.0


## New and Noteworthy/Highlights/What's New

-
-

## Breaking Changes
- `h.Section` now interprets positional arguments as `name`, `cell`. Previously positional arguments were interpreted in the other order. (Calling it with keyword arguments is unchanged.)
-  For 3d reaction-diffusion simulations, the voxelization and segment mapping algorithms have been adjusted, especially around the soma. Voxel indices and sometimes counts will change from previous versions.

## Deprecations
- Five functions in the `neuron` module: `neuron.init`, `neuron.run`, `neuron.psection`, `neuron.xopen`, and `neuron.quit`.
- Compile-time configuration with autotools will be removed in the next release. Use cmake instead.

## Bug Fixes
-
-

## Improvements
-
-

## Other Changes
-
-

## Upgrade Steps
- [ACTION REQUIRED]
-

## Contributors

-  Link to Github Contributor page

## Feedback / Help