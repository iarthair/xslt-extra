# Download

The Libxslt Extension Modules are available from [GitHub][5].  Clone the
repository as follows:

```sh
$ git clone https://github.com/iarthair/xslt-extra.git
```

## Dependencies

### libxml2

The libxml2 development package is required to build the extension modules.
If your distribution does not provide libxml2, you can [download it here][1].

### libxslt

The libxslt development package is required to build the extension modules.
If your distribution does not provide libxml2, you can [download it here][2].

### regcomp()

Most modern C libraries provide the POSIX.1-2001, POSIX.1-2008 regcomp() family
of functions, required to build the regular expression functions.

## Installation

The libxslt extension functions use the [Meson build system][3].  Refer to the
Meson manual for standard configuration options.

Meson supports multiple build system backends.  To build with [Ninja][4] do the
following:

``` sh
$ meson [options] --buildtype=release builddir
$ ninja -C builddir install
```

Note that the meson/ninja installer does not require an explicit `sudo`,
instead it will prompt for a password during install. This avoids polluting
builddir with files owned by root.

## Reporting Bugs

Bug should be reported using the GitHub issue tracker.

[5]: https://github.com/iarthair/xslt-extra
[1]: http://xmlsoft.org/downloads.html
[2]: http://xmlsoft.org/XSLT/downloads.html
[3]: https://mesonbuild.com/Getting-meson.html
[4]: https://ninja-build.org/


