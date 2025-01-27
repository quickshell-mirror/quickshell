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

#### Style preferences not caught by clang-format
These are flexible. You can ignore them if it looks or works better to
for one reason or another.

Use `auto` if the type of a variable can be deduced automatically, instead of
redeclaring the returned value's type. Additionally, auto should be used when a
constructor takes arguments.

```cpp
auto x = <expr>; // ok
auto x = QString::number(3); // ok
QString x; // ok
QString x = "foo"; // ok
auto x = QString("foo"); // ok

auto x = QString(); // avoid
QString x(); // avoid
QString x("foo"); // avoid
```

Put newlines around logical units of code, and after closing braces. If the
most reasonable logical unit of code takes only a single line, it should be
merged into the next single line logical unit if applicable.
```cpp
// multiple units
auto x = <expr>; // unit 1
auto y = <expr>; // unit 2

auto x = <expr>; // unit 1
emit this->y(); // unit 2

auto x1 = <expr>; // unit 1
auto x2 = <expr>; // unit 1
auto x3 = <expr>; // unit 1

auto y1 = <expr>; // unit 2
auto y2 = <expr>; // unit 2
auto y3 = <expr>; // unit 2

// one unit
auto x = <expr>;
if (x...) {
  // ...
}

// if more than one variable needs to be used then add a newline
auto x = <expr>;
auto y = <expr>;

if (x && y) {
  // ...
}
```

Class formatting:
```cpp
//! Doc comment summary
/// Doc comment body
class Foo: public QObject {
  // The Q_OBJECT macro comes first. Macros are ; terminated.
  Q_OBJECT;
  QML_ELEMENT;
  QML_CLASSINFO(...);
  // Properties must stay on a single line or the doc generator won't be able to pick them up
  Q_PROPERTY(...);
  /// Doc comment
  Q_PROPERTY(...);
  /// Doc comment
  Q_PROPERTY(...);

public:
  // Classes should have explicit constructors if they aren't intended to
  // implicitly cast. The constructor can be inline in the header if it has no body.
  explicit Foo(QObject* parent = nullptr): QObject(parent) {}

  // Instance functions if applicable.
  static Foo* instance();

  // Member functions unrelated to properties come next
  void function();
  void function();
  void function();

  // Then Q_INVOKABLEs
  Q_INVOKABLE function();
  /// Doc comment
  Q_INVOKABLE function();
  /// Doc comment
  Q_INVOKABLE function();

  // Then property related functions, in the order (bindable, getter, setter).
  // Related functions may be included here as well. Function bodies may be inline
  // if they are a single expression. There should be a newline between each
  // property's methods.
  [[nodiscard]] QBindable<T> bindableFoo() { return &this->bFoo; }
  [[nodiscard]] T foo() const { return this->foo; }
  void setFoo();

  [[nodiscard]] T bar() const { return this->foo; }
  void setBar();

signals:
  // Signals that are not property change related go first.
  // Property change signals go in property definition order.
  void asd();
  void asd2();
  void fooChanged();
  void barChanged();

public slots:
  // generally Q_INVOKABLEs are preferred to public slots.
  void slot();

private slots:
  // ...

private:
  // statics, then functions, then fields
  static const foo BAR;
  static void foo();

  void foo();
  void bar();

  // property related members are prefixed with `m`.
  QString mFoo;
  QString bar;

  // Bindables go last and should be prefixed with `b`.
  Q_OBJECT_BINDABLE_PROPERTY(Foo, QString, bFoo, &Foo::fooChanged);
};
```

### Linter
All contributions should pass the linter.

Note that running the linter requires disabling precompiled
headers and including the test codepaths:
```sh
$ just configure debug -DNO_PCH=ON -DBUILD_TESTING=ON
$ just lint-changed
```

If the linter is complaining about something that you think it should not,
please disable the lint in your MR and explain your reasoning if it isn't obvious.

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
You can reference other types using the `@@[Module.][Type.][member]` shorthand
where all parts are optional. If module or type are not specified they will
be inferred as the current module. Member can be a `property`, `function()` or `signal(s)`.
Look at existing code for how it works.

Quickshell modules additionally have a `module.md` file which contains a summary, description,
and list of headers to scan for documentation.

## Contributing

### Commits
Please structure your commit messages as `scope[!]: commit` where
the scope is something like `core` or `service/mpris`. (pick what has been
used historically or what makes sense if new). Add `!` for changes that break
existing APIs or functionality.

Commit descriptions should contain a summary of the changes if they are not
sufficiently addressed in the commit message.

Please squash/rebase additions or edits to previous changes and follow the
commit style to keep the history easily searchable at a glance.
Depending on the change, it is often reasonable to squash it into just
a single commit. (If you do not follow this we will squash your changes
for you.)

### Sending patches
You may contribute by submitting a pull request on github, asking for
an account on our git server, or emailing patches / git bundles
directly to `outfoxxed@outfoxxed.me`.

### Getting help
If you're getting stuck, you can come talk to us in the
[quickshell-development matrix room](https://matrix.to/#/#quickshell-development:outfoxxed.me)
for help on implementation, conventions, etc.
Feel free to ask for advice early in your implementation if you are
unsure.
