# EIPScanner

EIPScanner is a client library for EtherNet/IP (Industrial Protocol) communication.

## Building the Library

1. Create a build directory and navigate to it:
```bash
mkdir build && cd build
```

2. Generate build files with CMake:
```bash
cmake ..
```

3. Build and install the library (requires root privileges):
```bash
cmake --build . --target install
```

## Building the Examples

Before building the examples, you need to modify `examples/CMakeLists.txt`. Apply the following patch:

```diff
-include_directories("${PROJECT_SOURCE_DIR}/src")
+include_directories("../src")

add_executable(explicit_messaging ExplicitMessagingExample.cpp)
target_link_libraries(explicit_messaging EIPScanner)
```

Then build the examples:

```bash
cd examples
cmake .
make
```

## Running the Examples

To run the examples, you need to set the library path:

```bash
LD_LIBRARY_PATH=/usr/local/lib/ ./implicit_messaging
```
