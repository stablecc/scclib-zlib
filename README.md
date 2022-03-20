# scclib zlib library

Created by downloading [zlib 1.2.11 source code](http://zlib.net/zlib-1.2.11.tar.gz),
and patching via the following command:
```
cd zlib-1.2.11-ipp
patch p1 ../zlib-1.2.11.patch.bin
```

This library can be built with or without ipp support.

### ipp with bazel

To use ippcp, use the following command line:
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
