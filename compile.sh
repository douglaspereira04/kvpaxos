git submodule update --init --recursive
git pull --recurse-submodules
cd deps/libpaxos
git checkout master
cd ../../
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. -DINFO=ON -DTRACK_LENGTH=$1 -DQ_SIZE=$2 -DSCHEDULE_QUEUE_SIZE=$3 -DMAX_SUCESSIVE_IMBALANCE=$4
make -j8
