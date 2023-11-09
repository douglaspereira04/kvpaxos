# KVPaxos

KVPaxos is a key-value distributed storage system that uses Paxos and Parallel State Machine Replication to ensure consistency among replicas. It's developed as a prototype to measure latency and throughput when using state partitioning and balanced graph partitioning to schedule requests among threads, it includes 4 graph repartition algorithms to be used during execution: METIS, KaHIP, FENNEL and ReFENNEL.

KVPaxos is a prototype and so it does not cover many corner and common cases, it should not be used as it is in a real deploy context, but can be used as a starting point to other projects.

## Build

The experiments where executed in Ubuntu 22.04 64 bits. To make sure you have all dependencies needed, run:
```
apt install make cmake libtbb-dev libmsgpack-dev libevent-dev mpich pip libboost-all-dev git -y
pip install conan==1.59
```
Also, install LibCDS 2.3.3 from [](https://github.com/khizmax/libcds.git), and make sure Conan executable path is present in PATH environment variable.

To compile, run compile_all.sh script.
After that, scripts for each experiment can be executed in build/bin foulder

CMake is used to build the project, along with Conan to control dependecies. Conan downloads the following dependencies:

* LibEvent 2.1.11
* toml11
* tbb 2020.1

Dependencies not present in Conan are added as submodules, so make sure to recursively clone submodules too when cloning the project. For some reason that I've not cracked, `libpaxos` in `deps/libpaxos` is often cloned in an older commit, a `git checkout master` in the directory may be necessary.

Conan only controls dependecies that are directly used by KVPaxos, dependecies used by submodules need to be installed separately. The submodules and their dependecies are:

## Usage

In build/bin execute one of the scripts. The script generates the workload and executes the tests of a given implementation under a given workload.

## Output

The build/bin/output directory will contain the results of the experiments, divided in subfolders. Each experiment creates two CSV files, one containing throughput information along the execution, and other, prefixed with "details_", containg information about the scheduler and partitioner.