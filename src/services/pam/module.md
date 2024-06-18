name = "Quickshell.Services.Pam"
description = "Pam authentication"
headers = [
	"qml.hpp",
	"conversation.hpp",
]
-----

## Writing pam configurations

It is a good idea to write pam configurations specifically for quickshell
if you want to do anything other than match the default login flow.

A good example of this is having a configuration that allows entering a password
or fingerprint in any order.

### Structure of a pam configuration.
Pam configuration files are a list of rules, each on a new line in the following form:
```
<type> <control_flag> <module_path> [options]
```

Each line runs in order.

PamContext currently only works with the `auth` type, as other types require root
access to check.

#### Control flags
The control flags you're likely to use are `required` and `sufficient`.
- `required` rules must pass for authentication to succeed.
- `sufficient` rules will bypass any remaining rules and return on success.

Note that you should have at least one required rule or pam will fail with an undocumented error.

#### Modules
Pam works with a set of modules that handle various authentication mechanisms.
Some common ones include `pam_unix.so` which handles passwords and `pam_fprintd.so`
which handles fingerprints.

These modules have options but none are required for basic functionality.

### Examples

Authenticate with only a password:
```
auth required pam_unix.so
```

Authenticate with only a fingerprint:
```
auth required pam_fprintd.so
```

Try to authenticate with a fingerprint first, but if that fails fall back to a password:
```
auth sufficient pam_fprintd.so
auth required pam_unix.so
```

Require both a fingerprint and a password:
```
auth required pam_fprintd.so
auth required pam_unix.so
```


See also: [Oracle: PAM configuration file](https://docs.oracle.com/cd/E19683-01/816-4883/pam-32/index.html)
