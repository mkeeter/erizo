# Compiling
Erizo supports compiling a native application on MacOS,
or cross-compiling to Windows using the `x86_64-w64-mingw32-gcc` toolchain.

## Building dependencies
GLFW is shipped in the repository, to easily build a static binary.  It only needs to be compiled once.
```
[env TARGET=win32-cross] make glfw
```

## Building Erizo
```
[env TARGET=win32-cross] make
```
