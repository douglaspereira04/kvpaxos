git submodule update --init --recursive
git pull --recurse-submodules
cd deps/libpaxos
git checkout master
cd ../../
mkdir build
cd build
cmake .. -DSTORAGE=$1 -DSCHEDULER=$2 -DPENDING_KEYS=$3
make
