# unifyfs-bug

## The bug comes in multi-node case only.

## Instructions

- update path of unifyfs on dependency/spack.yaml packages
- activate dependency spack folder
  ```bash
  spack activate -p dependency
  spack install
  ```
- update path of unifyfs install on line 3 of CMakeLists
- build code.
  ```bash
  cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=/usr/tce/packages/gcc/gcc-8.3.1/bin/gcc -DCMAKE_CXX_COMPILER=/usr/tce/packages/gcc/gcc-8.3.1/bin/g++ -G "CodeBlocks - Unix Makefiles" /g/g92/haridev/temp/unifyfs-bug
  cmake --build /g/g92/haridev/temp/unifyfs-bug/cmake-build-debug --target all -- -j 128
  ```
- Run Unifyfs server
  ```bash
  cd unifyfs-bug
  export UNIFYFS_LOG_VERBOSITY=3
  # SET ME
  export UNIFYFS_ROOT_DIR=/usr/workspace/iopp/software/tailorfs/dependency/.spack-env/view  
  export UNIFYFS_LOG_DIR=${HOME}/unifyfs/logs
  export pfs=/p/gpfs1/iopp
  export LD_LIBRARY_PATH=${PWD}/dependency/.spack-env/view/lib:${UNIFYFS_ROOT_DIR}/lib
  # ACTUAL RUN
  UNIFYFS_LOG_DIR=$UNIFYFS_LOG_DIR UNIFYFS_SERVER_CORES=8 ${UNIFYFS_ROOT_DIR}/bin/unifyfs start --share-dir=${pfs}/unifyfs/share-dir -d
  ```
- Run code
  ## Bug 1
  ```bash
  jsrun -r 1 -a 1 -c 1  -d packed $PWD/cmake-build-debug/unifyfs-bug 1
  ```
  Output
  ```bash
  Running transfer
  2023-03-27T09:36:22 tid=30501 @ forward_to_server() [margo_client.c:233] margo_forward_timed() failed - HG_TIMEOUT
  2023-03-27T09:36:22 tid=30501 @ invoke_client_transfer_rpc() [margo_client.c:614] forward of transfer rpc to server failed
  unifyfs-bug: /g/g92/haridev/project/unifyfs-bug/bug.cpp:137: int main(int, char**): Assertion `rc == UNIFYFS_SUCCESS' failed.
  [lassen1:30501] *** Process received signal ***
  [lassen1:30501] Signal: Aborted (6)
  [lassen1:30501] Signal code:  (-6)
```
  ## Bug 2

  ```bash
  jsrun -r 1 -a 1 -c 1  -d packed $PWD/cmake-build-debug/unifyfs-bug 2
  ```
