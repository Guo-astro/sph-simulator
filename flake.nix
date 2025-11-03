{
  description = "SPH (Smoothed Particle Hydrodynamics) Simulation";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            cmake
            gcc
            boost
            nlohmann_json
            python3
            gtest
            conan
          ];

          shellHook = ''
            echo "SPH Simulation Development Environment"
            echo "CMake version: $(cmake --version | head -n1)"
            echo "GCC version: $(gcc --version | head -n1)"
          '';
        };

        packages.default = pkgs.stdenv.mkDerivation {
          pname = "sph-simulation";
          version = "0.1.0";
          
          src = ./.;
          
          nativeBuildInputs = with pkgs; [ cmake ];
          buildInputs = with pkgs; [ boost nlohmann_json ];
          
          cmakeFlags = [ ];
          
          meta = with pkgs.lib; {
            description = "SPH Simulation Code";
            license = licenses.mit;
            platforms = platforms.unix;
          };
        };
      }
    );
}
