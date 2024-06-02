# Contributing / Development
Instructions for development setup and upstreaming patches.

If you just want to build or package quickshell see [BUILD.md](BUILD.md).

## Development
Install the dependencies listed in [BUILD.md](BUILD.md).
You probably want all of them even if you don't use all of them
to ensure tests work correctly and avoid passing a bunch of configure
flags when you need to wipe the build directory.

Quickshell also uses `just` for common development command aliases.

The dependencies are also available as a nix shell or nix flake which we recommend
using with nix-direnv.

Common aliases:
- `just configure [<debug|release> [extra cmake args]]` (note that you must specify debug/release to specify extra args)
- `just build` - runs the build, configuring if not configured already.
- `just run [args]` - runs quickshell with the given arguments
- `just clean` - clean up build artifacts. `just clean build` is somewhat common.

### Formatting
All contributions should be formatted similarly to what already exists.
Group related functionality together.

Run the formatter using `just fmt`.
If the results look stupid, fix the clang-format file if possible,
or disable clang-format in the affected area
using `// clang-format off` and `// clang-format on`.

### Linter
All contributions should pass the linter.

Note that running the linter requires disabling precompiled
headers and including the test codepaths:
```sh
$ just configure debug -DNO_PCH=ON -DBUILD_TESTING=ON
$ just lint
```

If the linter is complaining about something that you think it should not,
please disable the lint in your MR and explain your reasoning.

### Tests
If you feel like the feature you are working on is very complex or likely to break,
please write some tests. We will ask you to directly if you send in an MR for an
overly complex or breakable feature.

At least all tests that passed before your changes should still be passing
by the time your contribution is ready.

You can run the tests using `just test` but you must enable them first
using `-DBUILD_TESTING=ON`.

### Documentation
Most of quickshell's documentation is automatically generated from the source code.
You should annotate `Q_PROPERTY`s and `Q_INVOKABLE`s with doc comments. Note that the parser
cannot handle random line breaks and will usually require you to disable clang-format if the
lines are too long.

Before submitting an MR, if adding new features please make sure the documentation is generated
reasonably using the `quickshell-docs` repo. We recommend checking it out at `/docs` in this repo.

Doc comments take the form `///` or `///!` (summary) and work with markdown.
Look at existing code for how it works.

Quickshell modules additionally have a `module.md` file which contains a summary, description,
and list of headers to scan for documentation.
