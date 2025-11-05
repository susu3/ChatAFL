# Fuzzing OpENer with AFLNet

This tutorial will guide you through the process of fuzzing OpENer, an EtherNet/IP stack implementation, using AFLNet.

## Building OpENer

1. Clone the OpENer repository:
```bash
git clone https://github.com/EIPStackGroup/OpENer.git
```

2. Install the required dependencies:
```bash
sudo apt-get update
sudo apt-get install cmake make gcc binutils
```

3. Build OpENer:
```bash
cd OpENer/bin/posix
./setup_posix_fuzz_afl.sh
make
```

If `setup_posix_fuzz_afl.sh` fails, you can try it again.

## Fuzzing

1. You need to make sure afl-fuzz is installed. For example, in `~/.bashrc`:

```bash
export PATH=$PATH:$HOME/aflnet-ICS
export AFL_PATH=$HOME/aflnet-ICS
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

2. Remove the AFL fuzzing code in `source/src/ports/POSIX/main.c` because we will use AFLNet to fuzz it.

```
diff --git a/source/src/ports/POSIX/main.c b/source/src/ports/POSIX/main.c
index 38a3bec51..bd4c0007d 100644
--- a/source/src/ports/POSIX/main.c
+++ b/source/src/ports/POSIX/main.c
@@ -111,10 +111,10 @@ int main(int argc,
   GetHostName(&g_tcpip.hostname);
 
   /* Fuzzing UDP/TCP handle packet flow */
-#ifdef FUZZING_AFL
-  fuzzHandlePacketFlow();
-  return EXIT_SUCCESS;
-#endif
+//#ifdef FUZZING_AFL
+//  fuzzHandlePacketFlow();
+//  return EXIT_SUCCESS;
+//#endif
```

2. Run AFL-Fuzz with the following command:
```bash
afl-fuzz -t 5000 -d \
         -i fuzz/inputs/ \
         -o out-opener \
         -N tcp://127.0.0.1/44818 \
         -P ETHERNETIP \
         -W 1000 \
         -q 3 \
         -s 3 \
         -E -K -R \
         -m none \
         ./bin/posix/src/ports/POSIX/OpENer lo
```

### Command Line Arguments Explained:
- `-t 5000`: Set timeout to 5 seconds
- `-d`: Skip deterministic fuzzing steps
- `-N tcp://127.0.0.1/20000`: Set target IP and port
- `-P ENIP`: Set protocol to EtherNet/IP
- `-W 1000`: Set wait time between requests to 1000ms
- `-q 3`: Set response wait time coefficient
- `-s 3`: Set response timeout coefficient
- `-E`: Enable state-aware mode
- `-K`: Enable response collection
- `-R`: Enable region-level mutation
- `-m none`: Disable memory limits
