# scclib zlib library

Contains modified [zlib](https://www.zlib.net/) source code.

Created by downloading [zlib 1.2.11 source code](http://zlib.net/zlib-1.2.11.tar.gz),
and patching via the following command:
```
cd zlib-1.2.11-ipp
patch p1 ../zlib-1.2.11.patch.bin
```

## ipp support

This library can be built with or without ipp support.

### ipp with bazel

To enable ipp, add the following to the command line:
```
$ bazel build :importzlib --define ipp=on
```
or modify `.bazelrc` with the following:
```
build --define ipp=on
```

### ipp with gnu make

To build the libraries with ipp, use the following command line:
```
make IPP=on
```

## licensing

Original source:
* [BSD 3-Clause License](lic/bsd_3_clause.txt)

External and redistributable:
* [zlib](lic/zlib.txt)
* [ipp](lic/intel.txt)
* [googletest](lic/google.txt)
* [bazel (google)](lic/bazel.txt)
