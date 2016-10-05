# Nix development environment
#
# build:
# nix-build -I "BBPpkgs=https://github.com/BlueBrain/bbp-nixpkgs/archive/master.tar.gz" default.nix
#
# build and test:
# nix-build -I "BBPpkgs=https://github.com/BlueBrain/bbp-nixpkgs/archive/master.tar.gz" --arg testExec true  default.nix  -j 4
#
# dev shell:
# nix-shell -I "BBPpkgs=https://github.com/BlueBrain/bbp-nixpkgs/archive/master.tar.gz"  default.nix
#
with import <BBPpkgs> { };

{ testExec ? false} : {


    func = stdenv.lib.overrideDerivation coreneuron (oldAttrs: {
      name = "coreneuron-DEVLOCAL";
      src = ./.;

      buildInputs = oldAttrs.buildInputs ++ [ bbptestdata ];

    });

}


