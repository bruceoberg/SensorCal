{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell
{
  packages =
  [
    pkgs.cmake
    pkgs.clang
    pkgs.ninja        # the build system cmake will use (ninja is effectively make++)
    pkgs.wxGTK32      # wxwidgets
    pkgs.pkg-config   # let's cmake find installed libs (eg wx)
  ];

  shellHook = ''
    export CC=${pkgs.clang}/bin/clang
    export CXX=${pkgs.clang}/bin/clang++
  '';
}