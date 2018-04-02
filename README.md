ltree
=====

Utility to work with file lists in the format of the BSD
[mtree(8)](https://man.openbsd.org/mtree) tool.

**Note:** This is *not* a replacement for `mtree`, and you may want to have
both installed side-by-side.


Building
--------

Make sure you have [libarchive](https://www.libarchive.org/) and its
development headers installed. [Meson](http://mesonbuild.com) is used for
configuration and [Ninja](https://ninja-build.org/) for building;
[Samurai](https://github.com/michaelforney/samurai) can be used for building
as well:

```sh
meson build
ninja -C build
```


Usage
-----

File lists in `mtree` format are accepted on standard input.

- `ltree -l < MTREE` prints out the contents of the list. The `-l` flag can
  be omitted, as this is the default mode.

- `ltree -C < MTREE` checks whether the files from the list are present in the
  file system and match the specification.

Operations which act on the file system accept a `-p PATH` command line option
which prepends `PATH` as prefix to all the file names in the list. Verbose
operation can be enabled with `-v`, which lists each entry as it is processed.

Any operation which prints paths in the output can output them delimited by
null characters by passing `-0` as a command line option.


Interoperability
----------------

The `ltree` tool can use file lists in the [format recognized by
libarchive](https://github.com/libarchive/libarchive/wiki/ManPageMtree5).

On GNU/Linux the [mtree port](https://github.com/archiecobbs/mtree-port) by
[@archiecobbs](https://github.com/archiecobbs/) is known to generate
compatible output.


License
-------

Distributed under terms of the [MIT/X11
license](https://opensource.org/licenses/MIT)
