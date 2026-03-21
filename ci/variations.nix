{
  clangStdenv,
  gccStdenv,
}: {
  clang = { stdenv = clangStdenv; };
  gcc = { stdenv = gccStdenv; };
}
