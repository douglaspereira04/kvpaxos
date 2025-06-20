git pull --recurse-submodules
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. -DINFO=ON -DLINEARIZABLE=ON -DEDGES=ON -DENGINE=TKRZW -DANSWER=OFF -DREPARTITIONING=$1 -DTRACK_LENGTH=$2 -DQ_SIZE=$3
make -j$(nproc)
