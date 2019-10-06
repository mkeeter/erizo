# About
Erizo is another extremely fast STL viewer.
It is a sequel to [`fstl`](https://www.mattkeeter.com/projects/fstl/),
which was challenged by [`meshview`](https://github.com/fogleman/meshview).

- Supports both binary and ASCII STLs
- Implemented in fast, native code
- Distributed as a single, statically-linked binary

![terrace bridge](http://www.mattkeeter.com/projects/erizo/terrace.png)  
*Model by Jennifer Keeter*

# Platforms
| OS | Compiler | Maintainer | Notes |
|-|-|-|-|
|MacOS|`llvm`|[@mkeeter](https://github.com/mkeeter)|Main development platform|
|Windows|`x86_64-w64-mingw32-gcc`|[@mkeeter](https://github.com/mkeeter)|Cross-compiled and tested with Wine|
|Your OS here|`???`|Your username here|Contributors welcome!|

Other platforms will be supported if implemented and maintained by other contributors.

To become a platform maintainer, open a PR which:
- Implements a new platform
- Add details to the table above
- Updates the **Compiling** instructions below.

# Compiling
At the moment, Erizo supports compiling a native application on MacOS,
or cross-compiling to Windows (if `TARGET=win32-cross` is set).

## Building dependencies
GLFW is shipped in the repository, to easily build a static binary.  It only needs to be compiled once.
```
[env TARGET=win32-cross] make glfw
```

## Building Erizo
```
[env TARGET=win32-cross] make
```

## Deploying an application bundle
### MacOS
[`cd deploy/darwin && deploy.sh`](https://github.com/mkeeter/hedgehog/blob/master/deploy/darwin/deploy.sh)
