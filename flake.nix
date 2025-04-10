{
  description = "OFC dev shell";

  inputs = {
    nixpkgs.url = "nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    nixpkgs,
    flake-utils,
    ...
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      devShells.default = pkgs.mkShellNoCC {
        buildInputs = with pkgs; [
          clang
          clang-tools
          conan
          cmake
          ninja
          pkg-config
          entr

          # For GTK support
          gtk2
          glib
          libGL
          zlib

          hyperfine
        ];
        LD_LIBRARY_PATH = "${pkgs.zlib}/lib:${pkgs.libGL}/lib:${pkgs.glib.out}/lib";
      };
    });
}
