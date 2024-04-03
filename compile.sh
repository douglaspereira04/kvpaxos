git submodule update --init --recursive
git pull --recurse-submodules
cd deps/libpaxos
git checkout master
cd ../../
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. -DSCHEDULER=$1 -DTRACK_LENGTH=$2 -DQ_SIZE=$3 -DSCHEDULE_QUEUE_SIZE=$4
make -j16
