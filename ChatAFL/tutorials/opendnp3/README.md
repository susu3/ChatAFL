# OpenDNP3 Build and Run Guide

This guide explains how to build and run the OpenDNP3 master and outstation examples.

## Building OpenDNP3

1. Navigate to the project root directory:
```bash
cd opendnp3
```

2. Build and install the project:
```bash
cmake .
make -j
sudo make install
```

## Running the Examples

### Setup
Before running the examples, set the library path:
```bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### Running the Outstation
1. Navigate to the outstation example:
```bash
cd cpp/examples/outstation
```

2. Build and run:
```bash
cmake .
make
./outstation-demo
```

### Running the Master
1. Navigate to the master example:
```bash
cd cpp/examples/master
```

2. Build and run:
```bash
cmake .
make
./master-demo
```

## Packet Capture
To capture DNP3 traffic between master and outstation:
```bash
sudo tcpdump -i lo -w opendnp3.pcap port 20000
```

If you want to capture the traffic, run packet capture first, then run the outstation (server) and master (client).

## Fuzzing with AFLNet

### Building for Fuzzing

1. First, modify the CMake configuration to use AFL compiler and add necessary flags. Update the following CMakeLists.txt files:

- Root CMakeLists.txt
- cpp/examples/master/CMakeLists.txt  
- cpp/examples/outstation/CMakeLists.txt

```
diff --git a/CMakeLists.txt b/CMakeLists.txt
index f32554685..1b3e07d64 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -6,6 +6,20 @@ if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
     set(is_root ON)
 endif()
 
+# Set AFL compiler
+set(CMAKE_C_COMPILER "afl-clang-fast")
+
+# Add compiler flags
+add_compile_options(
+    -Wno-return-type
+    -fsanitize=address
+    -g
+    -O0
+)
+
+# Add linker flags
+add_link_options(-fsanitize=address)
+
 # Project declaration
 set(OPENDNP3_MAJOR_VERSION 3)
 set(OPENDNP3_MINOR_VERSION 1)

diff --git a/cpp/examples/master/CMakeLists.txt b/cpp/examples/master/CMakeLists.txt
index f4946c537..8d39e8277 100644
--- a/cpp/examples/master/CMakeLists.txt
+++ b/cpp/examples/master/CMakeLists.txt
@@ -1,3 +1,18 @@
+# Set AFL compiler
+set(CMAKE_C_COMPILER "afl-clang-fast")
+set(CMAKE_CXX_COMPILER "afl-clang-fast++")
+
+# Add compiler flags
+add_compile_options(
+    -Wno-return-type
+    -fsanitize=address
+    -g
+    -O0
+)
+
+# Add linker flags
+add_link_options(-fsanitize=address)
+
 add_executable(master-demo ./main.cpp)
 target_link_libraries (master-demo PRIVATE opendnp3)
 set_target_properties(master-demo PROPERTIES FOLDER cpp/examples)

diff --git a/cpp/examples/outstation/CMakeLists.txt b/cpp/examples/outstation/CMakeLists.txt
index 63bbf8dab..d0ce24354 100644
--- a/cpp/examples/outstation/CMakeLists.txt
+++ b/cpp/examples/outstation/CMakeLists.txt
@@ -1,3 +1,18 @@
+# Set AFL compiler
+set(CMAKE_C_COMPILER "afl-clang-fast")
+set(CMAKE_CXX_COMPILER "afl-clang-fast++")
+
+# Add compiler flags
+add_compile_options(
+    -Wno-return-type
+    -fsanitize=address
+    -g
+    -O0
+)
+
+# Add linker flags
+add_link_options(-fsanitize=address)
+
 add_executable(outstation-demo ./main.cpp)
 target_link_libraries (outstation-demo PRIVATE opendnp3)
 set_target_properties(outstation-demo PROPERTIES FOLDER cpp/examples)
```

2. Build the project with AFL instrumentation:
```bash
cd cpp
CC=afl-clang-fast CXX=afl-clang-fast++ cmake .
make -j
```

### Running the Fuzzer

1. You need to make sure afl-fuzz is installed. For example, in `~/.bashrc`:

```bash
export PATH=$PATH:$HOME/aflnet-ICS
export AFL_PATH=$HOME/aflnet-ICS
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

2. Make sure you have a directory with seed files:

```
~/Sys-AFLnet/AFLIcs/test/tutorials/dnp3/in-dnp3/
```

3. Start the outstation in fuzzing mode:

```bash
cd cpp/examples/outstation
LD_LIBRARY_PATH=/home/ecs-user/Sys-AFLnet/DNP3/opendnp3/cpp/lib /home/ecs-user/Sys-AFLnet/AFLIcs/test/afl-fuzz -t 5000 -d -i ~/Sys-AFLnet/AFLIcs/test/tutorials/dnp3/in-dnp3/ -o out-dnp3 -N tcp://127.0.0.1/20000 -P DNP3 -W 1000 -q 3 -s 3 -E -K -R -m none ./outstation-demo 20000
```

You must specify the `LD_LIBRARY_PATH` to the compiled opendnp3 path.

Fuzzing options explained:
- `-t 5000`: Set timeout to 5 seconds
- `-d`: Skip deterministic steps
- `-N`: Specify network protocol and target
- `-P DNP3`: Use DNP3 protocol
- `-W 1000`: Wait 1000ms for initial response
- `-q 3`: Stop if 3 consecutive crashes found
- `-s 3`: Skip 3 states randomly
- `-E`: Enable state-aware mode
- `-K`: Enable response collection
- `-R`: Enable region-level mutation
- `-m none`: Disable memory limits

3. Monitor the fuzzing progress in the terminal. AFL will display statistics about the fuzzing process, including:
- Total executions
- Unique crashes found
- Coverage information
- Queue state

The fuzzer will save any crashes it finds in the `out-dnp3/crashes` directory for further analysis.

