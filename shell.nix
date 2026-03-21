{ pkgs ? import <nixpkgs> {} }:

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
  ];

  # ensure we get clang from nix

  shellHook = ''
    export CC=${pkgs.clang}/bin/clang
    export CXX=${pkgs.clang}/bin/clang++
  '';
}