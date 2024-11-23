{
  clangStdenv,
  gccStdenv,
}: {
  clang = { buildStdenv = clangStdenv; };
  gcc = { buildStdenv = gccStdenv; };
}
