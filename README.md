# Compiling
## Building dependencies
GLFW is shipped in the repository, to easily build a static binary.  It only needs to be compiled once.
### Native
```
make glfw
```
### Cross-compiling for Windows
```
env TARGET=win32-cross make glfw
```

## Building Erizo
### Native
```
make
```
### Cross-compiling for Windows
```
env TARGET=win32-cross make
```
