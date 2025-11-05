# Tutorial - Fuzzing bacnet-stack server
This assumes that you have read the AFLNet README.md before reading this tutorial.

This tutorial was tested on Ubuntu 20.04.

## Step-1. Server compilation & setup
You can download the source code of bacnet-stack from the [bacnet-stack]([https://github.com/bacnet-stack/bacnet-stack]). Please use the following commands to compile and set up the Bacnet for fuzzing.

```bash
cd $WORKDIR
# Clone the bacnet-stack repository
git clone https://github.com/bacnet-stack/bacnet-stack.git
# Move to the folder
cd bacnet-stack
# Set the compiler, the afl-clang-fast compiler is located in the AFLNet directory
export CC=afl-clang-fast
# Compile source
make clean all
```

## Step-2. Fuzzing
```bash
cd bacnet-stack/apps/server
export BACNET_IFACE=lo
afl-fuzz -t 3000 -d -i aflnet/tutorials/bacnet/in-bacnet/ -o out-bacnet -N udp://127.0.0.1/47808 -P BACNETIP -W 500 -q 3 -s 3 -E -K -R -m none ./bacserv
```



 

