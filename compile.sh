git pull --recurse-submodules
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug .. -DINFO=ON -DLINEARIZABLE=ON -DANSWER=ON -DREPARTITIONING=$1 -DTRACK_LENGTH=$2 -DQ_SIZE=$3
make -j$(nproc)
