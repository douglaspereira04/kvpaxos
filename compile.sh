git pull --recurse-submodules
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. -DINFO=ON -DANSWER=ON -DREPARTITIONING=ON -DTRACK_LENGTH=$1 -DQ_SIZE=$2 -DSCHEDULE_QUEUE_SIZE=$3
make -j$(nproc)
