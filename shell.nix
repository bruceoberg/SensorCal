{ pkgs ? import <nixpkgs> {} }:

let
  packagesLinux = pkgs.lib.optionals pkgs.stdenv.isLinux [
    pkgs.libGL
    pkgs.libGLU
    pkgs.freeglut
    pkgs.glm
    # for claude-code to be able to run tooling
    pkgs.python3
  ];
in

pkgs.mkShell
{
  packages =
  [
    # tools needed to build
    pkgs.cmake
    pkgs.clang
    pkgs.ninja        # the build system cmake will use (ninja is effectively make++)
    # libraries needed to build
    pkgs.wxGTK32      # wxwidgets
    # nix specific tools
    pkgs.pkg-config   # let's cmake find installed libs (eg wx)
    # utilities
    pkgs.claude-code
    pkgs.imagemagick
  ] ++ packagesLinux;

  # ensure we get clang from nix

  shellHook = ''
    export CC=${pkgs.clang}/bin/clang
    export CXX=${pkgs.clang}/bin/clang++
  '';
}