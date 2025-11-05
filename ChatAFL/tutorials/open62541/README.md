
# Open62541

This repository contains examples demonstrating the usage of the Open62541 OPC UA implementation.

## Prerequisites

Clone the repository and install cmake, make, etc.

```bash
git clone https://github.com/FreeOpcUa/freeopcua 
cd freeopcua
```

## Building and Installation

1. First, build and install Open62541:
```bash
cmake .
make -j8
sudo make install
```

2. Build the examples:
```bash
cd examples
cmake -DCMAKE_PREFIX_PATH=/usr/local .
make -j4
```

Note: Some build warnings may occur and can be ignored.

## Running the Examples

1. Go to the project root and start the server:
```bash
cd examples/bin/examples
./server_mainloop
```

2. In another terminal, go to the project root and run the client:
```bash
cd examples/bin/examples
./client_connect_loop
```